import numpy as np
import pytest
import stim # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler
from stimside.simulator_flip import FlipsideSimulator


class MockCompiledOpHandler(CompiledOpHandler[FlipsideSimulator]):
    def claim_ops(self) -> set[str]:
        return {"MOCK"}

    def handle_op(self, op: stim.CircuitInstruction, sss: "FlipsideSimulator") -> None:
        if op in self.claim_ops():
            raise NotImplementedError(f"mock handle_op called and somehow matched")
        else:
            sss.do_on_flip_simulator(op)
            return

    def clear(self) -> None:
        raise NotImplementedError(f"mock clear called")


class TestFlipsideSimulator:

    batch_size = 128
    coh = MockCompiledOpHandler()

    def make_simulator(self, circuit):
        return FlipsideSimulator(
            circuit=circuit, compiled_op_handler=self.coh, batch_size=self.batch_size
        )

    def test_coords_tracking(self):
        """test qubit_coords, qubit_tags, coords_shifts."""
        circuit = stim.Circuit(
            """
        QUBIT_COORDS(0,0) 0
        QUBIT_COORDS(1,0) 1
        SHIFT_COORDS(5, 5)
        QUBIT_COORDS(0) 5
        QUBIT_COORDS(1, 0) 6
        SHIFT_COORDS(5)
        SHIFT_COORDS(5,5,5)
        QUBIT_COORDS[QUBIT_TAG](0,0) 10
        """
        )
        fss = self.make_simulator(circuit)
        fss.interactive_do(circuit[0])  # QUBIT_COORDS(0,0) 0
        fss.interactive_do(circuit[1])  # QUBIT_COORDS(1,0) 1
        assert fss.qubit_coords == {0: [0, 0], 1: [1, 0]}
        fss.interactive_do(circuit[2])  # SHIFT_COORDS(5, 5)
        assert fss.qubit_coords == {0: [0, 0], 1: [1, 0]}
        assert fss.coords_shifts == [5, 5]
        fss.interactive_do(circuit[3])  # QUBIT_COORDS(0) 5
        fss.interactive_do(circuit[4])  # QUBIT_COORDS(1, 0) 6
        assert fss.qubit_coords == {0: [0, 0], 1: [1, 0], 5: [5], 6: [6, 5]}
        fss.interactive_do(circuit[5])  # SHIFT_COORDS(5)
        assert fss.coords_shifts == [10, 5]
        fss.interactive_do(circuit[6])  # SHIFT_COORDS(5,5,5)
        assert fss.coords_shifts == [15, 10, 5]
        fss.interactive_do(circuit[7])  # QUBIT_COORDS[QUBIT_TAG](0, 0)
        assert fss.qubit_coords == {0: [0, 0], 1: [1, 0], 5: [5], 6: [6, 5], 10: [15, 10]}

    def test_run_and_clear(self):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
        """
        )
        fss = self.make_simulator(circuit)

        fss.run()

        with pytest.raises(ValueError, match=r"prepare the simulator for a new run"):
            fss.run()

        with pytest.raises(NotImplementedError, match="mock clear called"):
            fss.clear()

        assert fss.qubit_coords == {}
        assert fss.qubit_tags == {}
        assert fss.coords_shifts == []

    def test_flip_simulator_lookthroughs(self):
        circuit = stim.Circuit(
            """
            M 0 1 2 3 4 5 6 7
            DETECTOR(0) rec[-8]
            DETECTOR(1) rec[-7]
            DETECTOR(2) rec[-6]
            DETECTOR(3) rec[-5]
            OBSERVABLE_INCLUDE(0) rec[-4] rec[-3] rec[-2] rec[-1]
        """
        )
        circuit_num_qubits = 8
        circuit_num_measurements = 8
        circuit_num_detectors = 4
        circuit_num_observables = 1

        fss = self.make_simulator(circuit)
        fss.run()

        assert fss.batch_size == self.batch_size
        assert fss.num_qubits == circuit_num_qubits

        assert fss.get_detector_flips().shape == (circuit_num_detectors, self.batch_size)
        assert fss.get_measurement_flips().shape == (circuit_num_measurements, self.batch_size)
        assert fss.get_observable_flips().shape == (circuit_num_observables, self.batch_size)

    def test_random_mask_funcs(self):
        circuit = stim.Circuit()
        fss = self.make_simulator(circuit)

        array = fss._make_random_mask(p=1, shape=(4, 12))
        assert array.shape == (4, 12)
        assert np.all(array != 0)

        array = np.zeros((4, 12), dtype=bool)
        fss._fill_random_mask(p=1, array=array)
        assert array.shape == (4, 12)
        assert np.all(array != 0)

        array = np.zeros((4, 12), dtype=bool)
        non_reshapeable_array_thing = array[1:2, :]
        with pytest.raises(ValueError, match="cannot reshape"):
            fss._fill_random_mask(p=1, array=non_reshapeable_array_thing)

    def test_broadcast_pauli_errors(self):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
        """
        )
        circuit_num_qubits = 4
        fss = self.make_simulator(circuit)
        fss.run()

        mask = np.zeros((circuit_num_qubits, self.batch_size), dtype=bool)
        mask[1, :] = True
        mask[3, :] = True
        before_xs, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        fss.broadcast_pauli_errors(error_mask=mask, p=1, pauli="X")
        after_xs, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        assert np.all(before_xs[0] == after_xs[0])
        assert np.all(before_xs[1] == np.logical_not(after_xs[1]))
        assert np.all(before_xs[2] == after_xs[2])
        assert np.all(before_xs[3] == np.logical_not(after_xs[3]))

        # jankily check probability
        mask = np.zeros((circuit_num_qubits, self.batch_size), dtype=bool)
        mask[1, :] = True
        mask[3, :] = True
        before_xs, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        fss.broadcast_pauli_errors(error_mask=mask, p=0.5, pauli="X")
        after_xs, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        assert np.all(before_xs[0] == after_xs[0])
        assert np.all(before_xs[2] == after_xs[2])

        # our batch size is massive, so we check that  the pauli error has done _something_
        # this should fail accidentally around 0.5**batch_size, which is vanishing
        assert np.any(before_xs[1] != after_xs[1])
        assert np.any(before_xs[3] != after_xs[3])
        # but also check it's not just been inverted unconditionally
        assert np.any(before_xs[1] != np.logical_not(before_xs[1]))
        assert np.any(before_xs[3] != np.logical_not(before_xs[3]))

    def test_get_all_clean_known_state(self):
        # for actual tests of the known state behaviour, see known_states_util_test
        circuit = stim.Circuit(
            """
            R 0 1 2 3
            X 1
            H 2 3
            M 3
        """
        )
        fss = self.make_simulator(circuit)
        fss.run()

        pX, mX, pY, mY, pZ, mZ = fss._get_all_clean_known_state(circuit_time=4)

        assert pX.tolist() == [False, False, True, False]
        assert mX.tolist() == [False, False, False, False]
        assert pY.tolist() == [False, False, False, False]
        assert mY.tolist() == [False, False, False, False]
        assert pZ.tolist() == [True, False, False, False]
        assert mZ.tolist() == [False, True, False, False]

    def test_get_clean_known_states(self):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
            X 1
            H 2 3
            M 3
        """
        )
        fss = self.make_simulator(circuit)
        fss.run()

        pX, mX = fss._get_clean_known_states(pauli="X", circuit_time=4)
        assert pX.tolist() == [False, False, True, False]
        assert mX.tolist() == [False, False, False, False]

        pZ, mZ = fss._get_clean_known_states(pauli="Z")  # circuit time should default to 4
        assert pZ.tolist() == [True, False, False, False]
        assert mZ.tolist() == [False, True, False, False]

    def test_all_targets_in_known_state(self):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
            X 1
            H 2 3
            M 3
        """
        )
        fss = self.make_simulator(circuit)
        fss.run()

        assert fss._all_targets_in_known_state(targets=[0, 1], pauli="Z")
        assert not fss._all_targets_in_known_state(targets=[0, 1, 2], pauli="Z")
        assert not fss._all_targets_in_known_state(targets=[0, 1, 3], pauli="Z", circuit_time=4)
        assert fss._all_targets_in_known_state(targets=[2], pauli="X", circuit_time=4)

    def test_get_current_known_state_masks(self):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
            X 1
            H 2 3
            M 3
        """
        )
        fss = self.make_simulator(circuit)
        fss.run()

        # No errors, no flips
        plus_mask, minus_mask = fss._get_current_known_state_masks(pauli="Z")
        assert plus_mask.shape == (fss.num_qubits, fss.batch_size)
        assert minus_mask.shape == (fss.num_qubits, fss.batch_size)

        # qubit 0 is in the 0 state, so in +Z
        assert np.all(plus_mask[0, :])
        assert not np.any(minus_mask[0, :])

        # qubit 1 is in the 1 state, so in Z
        assert np.all(minus_mask[1, :])
        assert not np.any(plus_mask[1, :])

        # qubit 2 is in +X, so shouldn't be in +Z or -Z
        assert not np.any(plus_mask[2, :])
        assert not np.any(minus_mask[2, :])

        # qubit 3 is randomly measured into +Z or -Z, so it is not in a known state
        assert not np.any(plus_mask[3, :])
        assert not np.any(minus_mask[3, :])

        # With errors, everyone gets a flip
        mask = np.ones((fss.num_qubits, fss.batch_size), dtype=bool)
        fss.broadcast_pauli_errors(mask, p=1, pauli="X")
        plus_mask, minus_mask = fss._get_current_known_state_masks(pauli="Z")
        # qubit 0 is in the 1 state, so in -Z
        assert np.all(minus_mask[0, :])
        assert not np.any(plus_mask[0, :])

        # qubit 1 is in the 0 state, so in +Z
        assert np.all(plus_mask[1, :])
        assert not np.any(minus_mask[1, :])

        # qubit 2 is in +X, so shouldn't be in +Z or -Z
        assert not np.any(plus_mask[2, :])
        assert not np.any(minus_mask[2, :])

        # qubit 3 is randomly measured into +Z or -Z, so it is not in a known state
        assert not np.any(plus_mask[3, :])
        assert not np.any(minus_mask[3, :])

    def test_circuit_time_zero_and_shift_coords_sync(self):
        """Regression tests for Bugs 8, 9, 10 and Optimization 4."""
        circuit = stim.Circuit(
            """
            R 0 1
            SHIFT_COORDS(0, 0, 1)
            X 0
            REPEAT 2 {
                SHIFT_COORDS(0, 0, 1)
                I 0 1
            }
            M 0 1
            DETECTOR rec[-2]
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-1]
        """
        )
        fss = self.make_simulator(circuit)
        fss.run()
        # Bug 9: circuit_time=0 must return initial known state (+Z on both qubits), not final state (-Z on q0)
        pZ_0, mZ_0 = fss._get_clean_known_states("Z", circuit_time=0)
        assert pZ_0.tolist() == [True, True]
        assert mZ_0.tolist() == [False, False]
        pZ_end, mZ_end = fss._get_clean_known_states("Z")
        assert pZ_end.tolist() == [False, True]
        assert mZ_end.tolist() == [True, False]

        # Optimization 4: bit_packed=True matches packbits(det_flips.T, bitorder="little", axis=1)
        det_bool = fss.get_detector_flips(bit_packed=False)
        det_packed = fss.get_detector_flips(bit_packed=True)
        assert np.array_equal(
            det_packed, np.packbits(det_bool.T, axis=1, bitorder="little")
        )

    def test_interactive_do_must_replay_the_circuit(self):
        circuit = stim.Circuit("X 0\nREPEAT 2 {\n    M 0\n}\nM 1")
        fss = self.make_simulator(circuit)
        fss.interactive_do(circuit[0])
        fss.interactive_do(circuit[1])  # a REPEAT block replays as its unrolled ops
        fss.interactive_do(stim.Circuit("M 1"))  # an equal op from another circuit object is fine
        assert fss.get_final_measurement_records().tolist() == [[True, True, False]] * self.batch_size
        with pytest.raises(ValueError, match="ends after 4 operations"):
            fss.interactive_do(stim.Circuit("M 0"))
        # Any other op raises, even one that only changes the reference sample (I 0 for X 0 used to
        # give M 0 = 1, the construction circuit's reference value).
        for other in ["I 0\nM 0", "X 1", "X 0\nM 1", "X 0\nM !0"]:
            with pytest.raises(ValueError, match="doesn't match the circuit"):
                self.make_simulator(circuit).interactive_do(stim.Circuit(other))
        # flattened() drops the SHIFT_COORDS (operation 2 here), so it doesn't replay the circuit.
        rounds = stim.Circuit("REPEAT 2 {\n    M 0\n    DETECTOR(0) rec[-1]\n    SHIFT_COORDS(1)\n}")
        with pytest.raises(ValueError, match=r"not `circuit\.flattened\(\)` or `circuit \+ more`"):
            self.make_simulator(rounds).interactive_do(rounds.flattened())

    def test_interactive_do_noise_strength_may_differ(self):
        # Only an untagged noisy gate's strength may differ: it is applied live, and everything
        # precomputed from the construction circuit is noiseless.
        fss = self.make_simulator(stim.Circuit("X_ERROR(0) 0\nM(0) 0 1"))
        fss.interactive_do(stim.Circuit("X_ERROR(1) 0\nM(1) 0 1"))
        assert fss.get_final_measurement_records().tolist() == [[False, True]] * self.batch_size
        for a, b in [
            ("X_ERROR(0.1) 0", "Z_ERROR(0.1) 0"),
            ("X_ERROR[t](0.1) 0", "X_ERROR[t](0.2) 0"),
            ("M 0\nOBSERVABLE_INCLUDE(0) rec[-1]", "M 0\nOBSERVABLE_INCLUDE(1) rec[-1]"),
        ]:
            with pytest.raises(ValueError, match="doesn't match the circuit"):
                self.make_simulator(stim.Circuit(a)).interactive_do(stim.Circuit(b))

    def test_final_measurement_records_after_partial_replay(self):
        circuit = stim.Circuit("X 0\nM 0\nI 0\nM 0 1")
        fss = self.make_simulator(circuit)
        fss.interactive_do(circuit[:2])  # used to broadcast the 3 reference bits over 1 measurement
        assert fss.get_final_measurement_records().tolist() == [[True]] * self.batch_size
        fss.interactive_do(circuit[2:])
        assert fss.get_final_measurement_records().tolist() == [[True, True, False]] * self.batch_size

