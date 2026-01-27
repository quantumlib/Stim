import numpy as np
import pytest
import stim # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    CompiledLeakageUint8,
    LeakageUint8,
)
from stimside.simulator_flip import FlipsideSimulator


class TestCompiledLeakageUint8:

    batch_size = 64
    # we're going to be checking for random behaviour by checking that
    # the x and z flip states have been modified somewhere in the batch.
    # this has a chance of not happening by accident that is 0.5**batch_size
    # we chose 64, so this is about the failure rate of 5E-20,
    # which is about where classical computers fail per operation
    # i.e. not going to happen to you

    def _get_simulator_for_circuit(self, circuit):
        cti = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=self.batch_size)

        return FlipsideSimulator(
            circuit=circuit, batch_size=self.batch_size, compiled_op_handler=cti
        )

    def test_clear(self):
        circuit = stim.Circuit("R 0")
        fss = self._get_simulator_for_circuit(circuit)

        # Manually set some state
        fss.compiled_op_handler.state[0, :] = 5
        assert np.any(fss.compiled_op_handler.state != 0)

        fss.compiled_op_handler.clear()
        assert np.all(fss.compiled_op_handler.state == 0)

    def test_make_target_mask(self):
        circuit = stim.Circuit("CX 0 1 2 3")
        op = circuit[0]

        # We don't need a full simulator for this, just the compiled op handler
        cti: CompiledLeakageUint8 = LeakageUint8().compile_op_handler(
            circuit=circuit, batch_size=self.batch_size
        )

        mask = cti.make_target_mask(op)

        expected_mask = np.zeros((circuit.num_qubits, self.batch_size), dtype=bool)
        expected_mask[0, :] = True
        expected_mask[1, :] = True
        expected_mask[2, :] = True
        expected_mask[3, :] = True

        assert np.array_equal(mask, expected_mask)

    def test_handle_op(self):
        import dataclasses

        @dataclasses.dataclass
        class FakeLeakageParams:
            name: str = "FAKE_LEAKAGE"
            from_tag: str = "FAKE"

        circuit = stim.Circuit("I 0")  # A dummy instruction
        op = circuit[0]

        fss = self._get_simulator_for_circuit(circuit)

        # Manually insert the fake params into the handler's dictionary
        fss.compiled_op_handler.ops_to_params[op] = FakeLeakageParams()

        with pytest.raises(ValueError, match="Unrecognised LEAKAGE params"):
            fss._do_instruction(op)

    def test_leakage_controlled_error(self):
        circuit = stim.Circuit(
            """
            R 0 1
            II_ERROR[LEAKAGE_CONTROLLED_ERROR: (1.0, 2-->X)] 0 1
        """
        )
        fss = self._get_simulator_for_circuit(circuit)

        fss.interactive_do(circuit[0])  # Reset operation

        # --- Test case 1: Control qubit is in the specified leakage state ---
        # Manually set the leakage state of the control qubit
        fss.compiled_op_handler.state[0, :] = 2
        fss.compiled_op_handler.state[1, :] = 0  # Target qubit is not leaked

        xs_before, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)

        fss.interactive_do(circuit[1])  # The LEAKAGE_CONTROLLED_ERROR operation

        xs_after, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)

        # The X error should be applied to qubit 1
        assert np.all(xs_after[1, :] != xs_before[1, :])
        # Qubit 0 should be unaffected
        assert np.all(xs_after[0, :] == xs_before[0, :])

        # --- Test case 2: Control qubit is NOT in the specified leakage state ---
        fss.clear()
        fss.interactive_do(circuit[0])

        # Set leakage state to something other than 2
        fss.compiled_op_handler.state[0, :] = 3

        xs_before, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)

        fss.interactive_do(circuit[1])

        xs_after, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)

        # No error should be applied
        assert np.all(xs_after[1, :] == xs_before[1, :])

    def test_leakage_transition_1(self):
        # Part a: Computational to Leaked
        circuit_a = stim.Circuit(
            """
            R 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        """
        )
        fss_a = self._get_simulator_for_circuit(circuit_a)
        fss_a.interactive_do(circuit_a[0])
        assert np.all(fss_a.compiled_op_handler.state[0, :] == 0)
        xs_before, zs_before, _, _, _ = fss_a._flip_simulator.to_numpy(
            output_xs=True, output_zs=True
        )
        fss_a.interactive_do(circuit_a[1])
        xs_after, zs_after, _, _, _ = fss_a._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        assert np.all(fss_a.compiled_op_handler.state[0, :] == 2)
        assert np.any(xs_before != xs_after)  # Check for depolarization
        assert np.any(zs_before != zs_after)

        # Part b: Leaked to Leaked
        circuit_b = stim.Circuit("I[LEAKAGE_TRANSITION_1: (1.0, 2-->3)] 0")
        fss_b = self._get_simulator_for_circuit(circuit_b)
        fss_b.compiled_op_handler.state[0, :] = 2
        fss_b.interactive_do(circuit_b[0])
        assert np.all(fss_b.compiled_op_handler.state[0, :] == 3)

        # Part c: Leaked to Computational
        circuit_c = stim.Circuit("I[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0")
        fss_c = self._get_simulator_for_circuit(circuit_c)
        fss_c.compiled_op_handler.state[0, :] = 2
        xs_before, zs_before, _, _, _ = fss_c._flip_simulator.to_numpy(
            output_xs=True, output_zs=True
        )
        fss_c.interactive_do(circuit_c[0])
        xs_after, zs_after, _, _, _ = fss_c._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        assert np.all(fss_c.compiled_op_handler.state[0, :] == 0)
        assert np.any(xs_before != xs_after)  # Check for depolarization
        assert np.any(zs_before != zs_after)

        # Part d: _depolarize_on_leak=False
        circuit_d = stim.Circuit(
            """
            R 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        """
        )
        fss_d = self._get_simulator_for_circuit(circuit_d)
        fss_d.compiled_op_handler._depolarize_on_leak = False
        fss_d.interactive_do(circuit_d[0])
        xs_before, zs_before, _, _, _ = fss_d._flip_simulator.to_numpy(
            output_xs=True, output_zs=True
        )
        fss_d.interactive_do(circuit_d[1])
        xs_after, zs_after, _, _, _ = fss_d._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        assert np.all(fss_d.compiled_op_handler.state[0, :] == 2)
        assert np.all(xs_before == xs_after)  # Check for NO depolarization
        assert np.all(zs_before == zs_after)

    def test_leakage_transition_Z(self):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
            X 1
            X_ERROR(1) 3
            I[LEAKAGE_TRANSITION_Z: (1.0, 0-->2) (1.0, 1-->3)] 0 1 2 3
            I[LEAKAGE_TRANSITION_Z: (1.0, 2-->0) (1.0, 3-->1)] 2 3
        """
        )
        fss = self._get_simulator_for_circuit(circuit)

        fss.interactive_do(circuit[0])
        fss.interactive_do(circuit[1])
        fss.interactive_do(circuit[2])
        # sanity check that we've prepared the state we think we've prepared
        xs_beforehand, zs_beforehand, _, _, _ = fss._flip_simulator.to_numpy(
            output_xs=True, output_zs=True
        )
        in_pZ, in_mZ = fss._get_current_known_state_masks(pauli="Z")
        assert np.all(in_pZ[0, :])  # q0 is in 0
        assert np.all(in_mZ[1, :])  # q1 is in 1
        assert np.all(in_pZ[2, :])  # q2 is in 0
        assert np.all(in_mZ[3, :])  # q3 is in 1 (via an error instruction)

        fss.interactive_do(circuit[3])  # do the first leakage transition instruction

        # check we're in the states we think we're in
        leakage_states = fss.compiled_op_handler.state
        assert np.all(leakage_states[0, :] == 2)
        assert np.all(leakage_states[1, :] == 3)
        assert np.all(leakage_states[2, :] == 2)
        assert np.all(leakage_states[3, :] == 3)

        # check we depolarize qubits when they leak
        xs_afterwards, zs_afterwards, _, _, _ = fss._flip_simulator.to_numpy(
            output_xs=True, output_zs=True
        )
        assert np.any(xs_afterwards[0, :] != xs_beforehand[0, :])
        assert np.any(xs_afterwards[1, :] != xs_beforehand[1, :])
        assert np.any(xs_afterwards[2, :] != xs_beforehand[2, :])
        assert np.any(xs_afterwards[3, :] != xs_beforehand[3, :])

        assert np.any(zs_afterwards[0, :] != zs_beforehand[0, :])
        assert np.any(zs_afterwards[1, :] != zs_beforehand[1, :])
        assert np.any(zs_afterwards[2, :] != zs_beforehand[2, :])
        assert np.any(zs_afterwards[3, :] != zs_beforehand[3, :])

        fss.interactive_do(circuit[4])  # unleak q2 and q3

        assert np.all(fss.compiled_op_handler.state[2, :] == 0)  # confirm the qubit was unleaked
        assert np.all(fss.compiled_op_handler.state[3, :] == 0)  # confirm the qubit was unleaked

        # Check we're in the right computational states
        # q2 is in a known_state of 0 and should be in |0), so x_flip should be false
        # q3 is in a known_state of 0 and should be in |1), so x_flip should be true
        xs_after, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        assert np.all(xs_after[2, :] == 0)
        assert np.all(xs_after[3, :] == 1)

    def test_leakage_transition_2(self):
        # Test case: (U, U) -> (L2, L3)
        circuit = stim.Circuit(
            """
            R 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_3)] 0 1
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.interactive_do(circuit[0])  # R 0 1

        xs_before, zs_before, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)

        fss.interactive_do(circuit[1])  # II_ERROR

        xs_after, zs_after, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)

        # Check leakage states
        assert np.all(fss.compiled_op_handler.state[0, :] == 2)
        assert np.all(fss.compiled_op_handler.state[1, :] == 3)

        # Check for depolarization
        assert np.any(xs_before != xs_after)
        assert np.any(zs_before != zs_after)

        # Test case: (L2, U) -> (L4, V)
        # This should depolarize qubit 1 (U->V) but not qubit 0 (L->L)
        circuit = stim.Circuit(
            """
            R 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->4_V)] 0 1
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.interactive_do(circuit[0])  # R 0 1
        fss.compiled_op_handler.state[0, :] = 2  # Manually set state of qubit 0

        xs_before, zs_before, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)

        fss.interactive_do(circuit[1])  # II_ERROR

        xs_after, zs_after, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)

        # Check leakage states
        assert np.all(fss.compiled_op_handler.state[0, :] == 4)
        assert np.all(fss.compiled_op_handler.state[1, :] == 0)  # V becomes 0

        # Check depolarization
        # Qubit 0 (L->L) should not be depolarized
        assert np.all(xs_before[0, :] == xs_after[0, :])
        assert np.all(zs_before[0, :] == zs_after[0, :])
        # Qubit 1 (U->V) should be depolarized
        assert np.any(xs_before[1, :] != xs_after[1, :])
        assert np.any(zs_before[1, :] != zs_after[1, :])

        # Test case with _depolarize_on_leak = False
        circuit = stim.Circuit(
            """
            R 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_3)] 0 1
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.compiled_op_handler._depolarize_on_leak = False
        fss.interactive_do(circuit[0])  # R 0 1

        xs_before, zs_before, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)

        fss.interactive_do(circuit[1])  # II_ERROR

        xs_after, zs_after, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)

        # Check leakage states
        assert np.all(fss.compiled_op_handler.state[0, :] == 2)
        assert np.all(fss.compiled_op_handler.state[1, :] == 3)

        # Check for NO depolarization
        assert np.all(xs_before == xs_after)
        assert np.all(zs_before == zs_after)

    def test_leakage_projection_Z(self):
        circuit = stim.Circuit(
            """
            X 1
            I[LEAKAGE_TRANSITION_Z: (1.0, 0-->2)] 2
            I[LEAKAGE_TRANSITION_Z: (1.0, 0-->3)] 3
            M[LEAKAGE_PROJECTION_Z: (1.0, 0) (0.0, 1) (1.0, 2) (0.5, 3)] 0 1 2 3
        """
        )
        # the 'known state' for all qubits is 0, so meas_flip should correspond to outcome
        # readout errors are 'backwards'
        # 0 states look like 1s with 100% prob
        # 1 states look like 0s with 100% prob
        # 2s look like 1s with 100% probability
        # 3s look 50:50 random
        fss = self._get_simulator_for_circuit(circuit)

        fss.interactive_do(circuit[0])
        fss.interactive_do(circuit[1])
        fss.interactive_do(circuit[2])
        # sanity check that we've prepared the state we think we've prepared
        xs_beforehand, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        in_pZ, in_mZ = fss._get_current_known_state_masks(pauli="Z")
        assert np.all(in_pZ[0, :])  # q0 is in 0
        assert np.all(in_mZ[1, :])  # q1 is in 1
        assert not np.all(in_pZ[2, :])  # q2 is leaked, should have been depolarized
        assert not np.all(in_mZ[2, :])
        assert not np.all(in_pZ[3, :])  # q3 is leaked, should have been depolarized
        assert not np.all(in_mZ[3, :])

        fss.interactive_do(circuit[3])  # finally, do the leakage projection instruction

        # measurement flips are what they're supposed to be
        meas_flips = fss._flip_simulator.get_measurement_flips()

        assert np.all(meas_flips[0, :] == 1)  # qubit 0 was in 0, should read out as all 1s
        assert np.all(meas_flips[1, :] == 0)  # qubit 1 was in 1, should read out as all 0s
        assert np.all(meas_flips[2, :] == 1)  # qubit 2 was in 2, should read out as all 1s
        assert not np.all(meas_flips[3, :] == 0)  # qubit 3 was in 3, should be 50:50
        assert not np.all(meas_flips[3, :] == 1)  # we check it's not all 0s and not all 1s

        # leakage states haven't changed
        assert np.all(
            fss.compiled_op_handler.state
            == np.array(
                [
                    [0] * self.batch_size,  # qubit 0 is in a computational state
                    [0] * self.batch_size,  # qubit 1 is in a computational state
                    [2] * self.batch_size,  # qubit 2 is in the 2 state
                    [3] * self.batch_size,  # qubit 3 is in the 3 state
                ]
            )
        )

        # computational X flips have been appropriately randomized
        xs_afterwards, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        assert not np.all(xs_beforehand == xs_afterwards)

    def test_error_handling(self):
        with pytest.raises(ValueError, match="targets aren't in known Z states"):
            circuit = stim.Circuit(
                """
                H 0
                I[LEAKAGE_TRANSITION_Z: (1.0, 0-->2)] 0
            """
            )
            fss = self._get_simulator_for_circuit(circuit)
            fss.interactive_do(circuit)

        with pytest.raises(ValueError, match="targets aren't in known Z states"):
            circuit = stim.Circuit(
                """
                H 0
                M[LEAKAGE_PROJECTION_Z: (1.0, 0)] 0
            """
            )
            fss = self._get_simulator_for_circuit(circuit)
            fss.interactive_do(circuit)
