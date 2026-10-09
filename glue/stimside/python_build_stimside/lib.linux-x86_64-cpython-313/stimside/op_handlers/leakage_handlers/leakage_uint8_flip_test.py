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
            X_ERROR(1) 1
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

        # computational X flips have been appropriately randomized on leaked qubits
        xs_afterwards, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        assert not np.all(xs_beforehand == xs_afterwards)
        # Unleaked qubits 0 and 1 should NOT have their Z eigenstates scrambled by X flips
        assert np.all(xs_afterwards[0, :] == xs_beforehand[0, :])
        assert np.all(xs_afterwards[1, :] == xs_beforehand[1, :])

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

    def test_untagged_gate_not_executed_twice(self):
        """Regression test for Bug 1: untagged M/R/MR must only execute once."""
        circuit = stim.Circuit(
            """
            R 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            M 0 1
            DETECTOR rec[-1]
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.run()
        # Only 2 measurements should exist (not 4 from double execution of M 0 1)
        assert fss.get_measurement_flips().shape == (2, self.batch_size)
        # Qubit 1 is unleaked in |0>, so rec[-1] must have 0 detector flips
        assert np.all(fss.get_detector_flips() == 0)

    def test_controlled_error_pauli_and_multi_control(self):
        """Regression test for Bug 2: Z/Y Pauli branches and multiple controls targeting same qubit."""
        circuit = stim.Circuit(
            """
            R 0 1 2
            H 2
            II_ERROR[LEAKAGE_CONTROLLED_ERROR: (1.0, 2-->Z) (0.5, 3-->X) (0.5, 3-->Y)] 0 2 1 2
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.interactive_do(circuit[0])
        fss.interactive_do(circuit[1])
        xs_before, zs_before, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        # Qubit 0 is leaked (state 2), Qubit 1 is unleaked (state 0); both target Qubit 2!
        fss.compiled_op_handler.state[0, :] = 2
        fss.compiled_op_handler.state[1, :] = 0
        fss.interactive_do(circuit[2])
        xs, zs, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        # Qubit 2 should receive a Z flip (not X flip, and not overwritten by pair (1, 2))
        assert np.all(zs[2, :] == (zs_before[2, :] ^ 1))
        assert np.all(xs[2, :] == xs_before[2, :])

    def test_transition_1_no_false_depolarization_or_chaining(self):
        """Regression test for Bug 3: U->U stays untouched and U->2 / 2->U do not chain."""
        circuit = stim.Circuit(
            """
            R 0 1
            I[LEAKAGE_TRANSITION_1: (0.0, U-->2) (1.0, 2-->U)] 0 1
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.interactive_do(circuit[0])
        xs_before, zs_before, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        fss.interactive_do(circuit[1])
        xs, zs, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        assert np.all(xs == xs_before)
        assert np.all(zs == zs_before)

        circuit_chain = stim.Circuit(
            """
            R 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2) (1.0, 2-->U)] 0
        """
        )
        fss_chain = self._get_simulator_for_circuit(circuit_chain)
        fss_chain.run()
        assert np.all(fss_chain.compiled_op_handler.state[0, :] == 2)

    def test_transition_2_out_of_order_and_pauli_outputs(self):
        """Regression test for Bug 4: out-of-order targets, even/odd depolarization, and D/X/Y/Z outputs."""
        circuit = stim.Circuit(
            """
            R 0 1 2 3
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_U)] 3 1 2 0
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->2_Z)] 3 1
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.interactive_do(circuit[0])
        xs_before, zs_before, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        fss.interactive_do(circuit[1])
        # Even targets are 3 and 2 -> should be in state 2 and depolarized
        # Odd targets are 1 and 0 -> should stay in state 0 and NOT be depolarized!
        assert np.all(fss.compiled_op_handler.state[3, :] == 2)
        assert np.all(fss.compiled_op_handler.state[2, :] == 2)
        assert np.all(fss.compiled_op_handler.state[1, :] == 0)
        assert np.all(fss.compiled_op_handler.state[0, :] == 0)
        xs, zs, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        assert np.all(xs[1, :] == xs_before[1, :]) and np.all(zs[1, :] == zs_before[1, :])
        assert np.all(xs[0, :] == xs_before[0, :]) and np.all(zs[0, :] == zs_before[0, :])

        # Now apply 2_U --> 2_Z on (3, 1): qubit 1 should get a deterministic Z error
        fss.interactive_do(circuit[2])
        xs, zs, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True, output_zs=True)
        assert np.all(zs[1, :] == (zs_before[1, :] ^ 1))
        assert np.all(xs[1, :] == xs_before[1, :])

    def test_transition_Z_preserves_leaked_and_supports_reset(self):
        """Regression test for Bug 5: already-leaked qubits are not unleaked by 0-->2, and R works."""
        circuit = stim.Circuit(
            """
            R 0
            I[LEAKAGE_TRANSITION_Z: (1.0, 0-->3)] 0
            I[LEAKAGE_TRANSITION_Z: (0.0, 0-->2)] 0
            R[LEAKAGE_TRANSITION_Z: (1.0, 3-->1)] 0
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.interactive_do(circuit[0])
        fss.interactive_do(circuit[1])
        assert np.all(fss.compiled_op_handler.state[0, :] == 3)
        # (0.0, 0-->2) must NOT reset state 3 back to 0!
        fss.interactive_do(circuit[2])
        assert np.all(fss.compiled_op_handler.state[0, :] == 3)
        # R[LEAKAGE_TRANSITION_Z: (1.0, 3-->1)] must unleak 3 -> 1 and leave qubit 0 in |1>
        fss.interactive_do(circuit[3])
        assert np.all(fss.compiled_op_handler.state[0, :] == 0)
        xs, _, _, _, _ = fss._flip_simulator.to_numpy(output_xs=True)
        assert np.all(xs[0, :] == 1)

    def test_projection_Z_clean_minus_and_omitted_states(self):
        """Regression test for Bug 6: clean |1> state produces 0 detector flips and omitted states default properly."""
        circuit = stim.Circuit(
            """
            R 0 1
            X 1
            M[LEAKAGE_PROJECTION_Z: (0.0, 2)] 0 1
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        """
        )
        fss = self._get_simulator_for_circuit(circuit)
        fss.run()
        # States 0 and 1 were omitted from tag -> default to p0=0.0, p1=1.0
        # Clean state of q1 is |1>, and it reads out as 1 -> 0 measurement flips and 0 detector flips!
        assert np.all(fss.get_detector_flips() == 0)

    def test_surface_code_flipside_vs_tableside_equivalence(self):
        """Cross-validate FlipsideSimulator (ControlledError and 2_U->2_D) vs TablesideSimulator on d=3 surface code."""
        from stimside.simulator_tableau import TablesideSimulator
        from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
            LeakageUint8 as LeakageUint8Tableau,
        )

        base = stim.Circuit.generated(
            "surface_code:rotated_memory_z",
            distance=3,
            rounds=3,
            after_clifford_depolarization=0.001,
            after_reset_flip_probability=0.001,
            before_measure_flip_probability=0.001,
            before_round_data_depolarization=0.001,
        ).flattened()

        c_flip = stim.Circuit()
        c_tab = stim.Circuit()
        for op in base:
            if op.name in ("R", "RZ"):
                c_flip.append(op)
                c_tab.append(op)
                targets = [t.value for t in op.targets_copy()]
                c_flip.append("I", targets, [], tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)")
                c_tab.append("I", targets, [], tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)")
            elif op.name in ("M", "MZ"):
                targets = [t.value for t in op.targets_copy()]
                c_flip.append(op.name, targets, op.gate_args_copy(), tag="LEAKAGE_PROJECTION_Z: (1.0, 2)")
                c_tab.append(op.name, targets, op.gate_args_copy(), tag="LEAKAGE_PROJECTION_Z: (1.0, 2)")
            elif op.name in ("CX", "CZ"):
                c_flip.append(op)
                c_tab.append(op)
                targets = [t.value for t in op.targets_copy()]
                trans_tag = "LEAKAGE_TRANSITION_2: (0.002, U_U-->2_D) (0.01, 2_U-->U_D) (0.01, U_2-->D_U)"
                c_flip.append("II_ERROR", targets, [], tag=trans_tag)
                c_tab.append("II_ERROR", targets, [], tag=trans_tag)
                swapped = []
                for i in range(0, len(targets), 2):
                    swapped.extend([targets[i], targets[i + 1], targets[i + 1], targets[i]])
                c_flip.append(
                    "II_ERROR",
                    swapped,
                    [],
                    tag="LEAKAGE_CONTROLLED_ERROR: (0.25, 2-->X) (0.25, 2-->Y) (0.25, 2-->Z)",
                )
                c_tab.append(
                    "II_ERROR",
                    targets,
                    [],
                    tag="LEAKAGE_TRANSITION_2: (1.0, 2_U-->2_D) (1.0, U_2-->D_2)",
                )
                c_tab.append("DEPOLARIZE1", targets, [0.75], tag="CONDITIONED_ON_SELF: 2")
            else:
                c_flip.append(op)
                c_tab.append(op)

        fss = FlipsideSimulator(
            c_flip,
            compiled_op_handler=LeakageUint8().compile_op_handler(circuit=c_flip, batch_size=4096),
            batch_size=4096,
            seed=42,
        )
        fss.run()
        det_flip = fss.get_detector_flips(bit_packed=False).mean()

        tss = TablesideSimulator(
            c_tab,
            compiled_op_handler=LeakageUint8Tableau().compile_op_handler(circuit=c_tab, batch_size=1),
            batch_size=1,
            running_tableau=True,
            seed=42,
        )
        det_tab_list = []
        for _ in range(400):
            tss.clear()
            tss.run()
            det_tab_list.append(tss.get_detector_flips()[0])
        det_tab = np.mean(det_tab_list)

        assert abs(det_flip - det_tab) < 0.005

    def test_projection_Z_symmetric_float_readout_and_mx_mrx_recovery(self):
        """Test symmetric float readout (p0=0.01, p1=0.99) on superposition and MX/MRX+H before LEAKAGE_TRANSITION_Z."""
        # 1. p0=0.01, p1=0.99 has 1.0 - 0.99 != 0.01 in IEEE-754 float64; np.isclose must allow superposition
        c_sym = stim.Circuit(
            """
            R 0 1
            H 0
            CX 0 1
            M[LEAKAGE_PROJECTION_Z: (0.01, 0) (0.99, 1) (1.0, 2)] 0 1
        """
        )
        fss_sym = self._get_simulator_for_circuit(c_sym)
        fss_sym.run()
        assert fss_sym.get_measurement_flips().shape == (2, self.batch_size)

        # 2. Mid-circuit MX/MRX followed by H recovers deterministic Z eigenstate for LEAKAGE_TRANSITION_Z
        c_mx = stim.Circuit(
            """
            R 0
            H 0
            MX 0
            H 0
            I[LEAKAGE_TRANSITION_Z: (1.0, 0-->2)] 0
            MRX 1
            H 1
            I[LEAKAGE_TRANSITION_Z: (1.0, 0-->3)] 1
        """
        )
        fss_mx = self._get_simulator_for_circuit(c_mx)
        fss_mx.run()
        assert np.all(fss_mx.compiled_op_handler.state[0, :] == 2)
        assert np.all(fss_mx.compiled_op_handler.state[1, :] == 3)

    def test_transition_2_shared_targets_and_pauli_xor(self):
        """Test multi-pair LEAKAGE_TRANSITION_2 preserves earlier pair leakage and XORs duplicate Pauli flips."""
        # 1. Two pairs (0, 2) and (1, 2) both applying X to q2 must cancel (X * X = I)
        c_xor = stim.Circuit(
            """
            R 0 1 2
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->2_X)] 0 2 1 2
            M 2
            DETECTOR rec[-1]
        """
        )
        fss_xor = self._get_simulator_for_circuit(c_xor)
        fss_xor.run()
        assert np.all(fss_xor.get_detector_flips() == 0)

        # 2. Shared qubit 0 across pairs (0, 1) and (0, 2) with (0.5, U_U-->2_U) (0.5, U_U-->U_2)
        # Whenever pair (0, 1) samples 2_U, q0 must end up in state 2 even if pair (0, 2) samples U_2!
        c_shared = stim.Circuit(
            """
            R 0 1 2
            II_ERROR[LEAKAGE_TRANSITION_2: (0.5, U_U-->2_U) (0.5, U_U-->U_2)] 0 1 0 2
        """
        )
        fss_shared = self._get_simulator_for_circuit(c_shared)
        fss_shared.run()
        st = fss_shared.compiled_op_handler.state
        # In every shot, pair (0, 1) either sets q0=2 or q1=2; if q1==0, pair (0, 1) MUST have set q0=2!
        assert np.all((st[0, :] == 2) | (st[1, :] == 2))

    @pytest.mark.parametrize(
        "distance,rounds,p2_readout",
        [(3, 3, 0.5), (3, 4, 1.0), (5, 3, 0.5)],
    )
    def test_synced_surface_code_2q_loss_exact_measurement_records(
        self, distance: int, rounds: int, p2_readout: float
    ):
        """Verify 100% shot-for-shot measurement record, detector, observable, and state equality on surface code with 2Q loss."""
        from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
            LeakageUint8 as LeakageTab,
        )
        from stimside.simulator_tableau import TablesideSimulator

        p_leak = 0.025
        p_depol = 0.005
        base = stim.Circuit.generated(
            "surface_code:rotated_memory_z",
            distance=distance,
            rounds=rounds,
            after_clifford_depolarization=p_depol,
            before_measure_flip_probability=p_depol,
            after_reset_flip_probability=p_depol,
        ).flattened()

        c_flip = stim.Circuit()
        c_tab = stim.Circuit()
        c_unified = stim.Circuit()
        for op in base:
            targs = op.targets_copy()
            args = op.gate_args_copy()
            if op.name == "CX":
                bidir_targs = []
                for i in range(0, len(targs), 2):
                    bidir_targs.extend(
                        [targs[i], targs[i + 1], targs[i + 1], targs[i]]
                    )
                c_flip.append(op)
                c_flip.append(
                    stim.CircuitInstruction(
                        "I_ERROR",
                        targs,
                        [],
                        tag=f"LEAKAGE_TRANSITION_1: ({p_leak}, U-->2)",
                    )
                )
                c_flip.append(
                    stim.CircuitInstruction(
                        "II_ERROR",
                        bidir_targs,
                        [],
                        tag="LEAKAGE_CONTROLLED_ERROR: (0.25, 2-->X), (0.25, 2-->Y), (0.25, 2-->Z)",
                    )
                )
                c_tab.append(op)
                c_tab.append(
                    stim.CircuitInstruction(
                        "I_ERROR",
                        targs,
                        [],
                        tag=f"LEAKAGE_TRANSITION_1: ({p_leak}, U-->2)",
                    )
                )
                c_tab.append(
                    stim.CircuitInstruction(
                        "II_ERROR",
                        targs,
                        [],
                        tag="LEAKAGE_TRANSITION_2: (1.0, 2_U-->2_D), (1.0, U_2-->D_2)",
                    )
                )
                c_unified.append(op)
                c_unified.append(
                    stim.CircuitInstruction(
                        "II_ERROR",
                        targs,
                        [],
                        tag=f"LEAKAGE_TRANSITION_2: ({p_leak * 0.5}, U_U-->2_D), ({p_leak * 0.5}, U_U-->D_2), (1.0, 2_U-->2_D), (1.0, U_2-->D_2)",
                    )
                )
            elif op.name == "MR":
                m_tag = f"LEAKAGE_PROJECTION_Z: (0.0, 0), (1.0, 1), ({p2_readout}, 2)"
                r_tag = "LEAKAGE_TRANSITION_1: (1.0, 2-->0)"
                for circ in (c_flip, c_tab, c_unified):
                    circ.append(
                        stim.CircuitInstruction("M", targs, args, tag=m_tag)
                    )
                    circ.append(stim.CircuitInstruction("R", targs, []))
                    circ.append(
                        stim.CircuitInstruction(
                            "I_ERROR", targs, [], tag=r_tag
                        )
                    )
            elif op.name == "R":
                c_flip.append(op)
                c_flip.append(
                    stim.CircuitInstruction(
                        "I_ERROR", targs, [], tag="LEAKAGE_TRANSITION_1: (1.0, 2-->0)"
                    )
                )
                c_tab.append(op)
                c_tab.append(
                    stim.CircuitInstruction(
                        "I_ERROR", targs, [], tag="LEAKAGE_TRANSITION_1: (1.0, 2-->0)"
                    )
                )
                c_unified.append(op)
                c_unified.append(
                    stim.CircuitInstruction(
                        "I_ERROR", targs, [], tag="LEAKAGE_TRANSITION_1: (1.0, 2-->0)"
                    )
                )
            elif op.name == "M":
                m_tag = f"LEAKAGE_PROJECTION_Z: (0.0, 0), (1.0, 1), ({p2_readout}, 2)"
                c_flip.append(
                    stim.CircuitInstruction("M", targs, args, tag=m_tag)
                )
                c_tab.append(
                    stim.CircuitInstruction("M", targs, args, tag=m_tag)
                )
                c_unified.append(
                    stim.CircuitInstruction("M", targs, args, tag=m_tag)
                )
            else:
                c_flip.append(op)
                c_tab.append(op)
                c_unified.append(op)

        for cf, ct in [(c_flip, c_tab), (c_unified, c_unified)]:
            num_shots = 16
            seed = 98765
            fh = LeakageUint8().compile_op_handler(
                circuit=cf, batch_size=num_shots
            )
            fss = FlipsideSimulator(
                cf,
                compiled_op_handler=fh,
                batch_size=num_shots,
                seed=seed,
                sync_tableside_rng=True,
            )
            th = LeakageTab().compile_op_handler(circuit=ct, batch_size=1)
            tss = TablesideSimulator(
                ct,
                compiled_op_handler=th,
                batch_size=1,
                seed=seed,
                sync_flipside_rng=True,
            )

            for batch_idx in range(2):
                if batch_idx > 0:
                    fss.clear()
                fss.run()
                f_meas = fss.get_final_measurement_records()
                f_det = fss.get_detector_flips()
                f_obs = fss.get_observable_flips()

                t_meas_list, t_det_list, t_obs_list, t_state_list = (
                    [],
                    [],
                    [],
                    [],
                )
                for b in range(num_shots):
                    if batch_idx > 0 or b > 0:
                        tss.clear()
                    tss.run()
                    t_meas_list.append(tss.get_final_measurement_records()[0])
                    t_det_list.append(tss.get_detector_flips()[0])
                    t_obs_list.append(tss.get_observable_flips()[0])
                    t_state_list.append(th.state.copy())

                t_meas = np.stack(t_meas_list, axis=0)
                t_det = np.stack(t_det_list, axis=1)
                t_obs = np.stack(t_obs_list, axis=1)
                t_state = np.stack(t_state_list, axis=1)

                assert np.array_equal(f_meas, t_meas)
                assert np.array_equal(f_det, t_det)
                assert np.array_equal(f_obs, t_obs)
                assert np.array_equal(fh.state, t_state)
                assert np.any(fh.state >= 2)
                assert np.any(f_det)

    def test_synced_edge_cases_mpad_partial_lce_and_transition_z(self):
        """Verify synced equivalence on MPAD[LEAKAGE_MEASUREMENT], inverted targets, partial/biased LEAKAGE_CONTROLLED_ERROR, and LEAKAGE_TRANSITION_Z."""
        from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
            LeakageUint8 as LeakageTab,
        )
        from stimside.simulator_tableau import TablesideSimulator

        cf = stim.Circuit(
            """
            X 1
            I_ERROR[LEAKAGE_TRANSITION_Z: (0.4, 0-->2), (0.6, 1-->2)] 0 1
            CX 0 2 1 3
            II_ERROR[LEAKAGE_CONTROLLED_ERROR: (0.1, 2-->X), (0.1, 2-->Y), (0.1, 2-->Z)] 0 2 2 0
            II_ERROR[LEAKAGE_CONTROLLED_ERROR: (0.35, 2-->X), (0.15, 2-->Z)] 1 3 3 1
            MPAD[LEAKAGE_MEASUREMENT: (0.05, 0), (0.05, 1), (0.9, 2) : 0 1 2 3] 0 1 0 1
            I_ERROR[LEAKAGE_TRANSITION_1: (0.5, 2-->0)] 0 1
            M[LEAKAGE_PROJECTION_Z: (0.02, 0), (0.98, 1), (0.5, 2)] !0 1 !2 3
            """
        )
        ct = stim.Circuit(
            """
            X 1
            I_ERROR[LEAKAGE_TRANSITION_1: (0.4, 0-->2), (0.6, 1-->2)] 0 1
            CX 0 2 1 3
            II_ERROR[LEAKAGE_TRANSITION_2: (0.4, 2_U-->2_D), (0.4, U_2-->D_2)] 0 2
            II_ERROR[LEAKAGE_TRANSITION_2: (0.35, 2_U-->2_X), (0.15, 2_U-->2_Z), (0.35, U_2-->X_2), (0.15, U_2-->Z_2)] 1 3
            MPAD[LEAKAGE_MEASUREMENT: (0.05, 0), (0.05, 1), (0.9, 2) : 0 1 2 3] 0 1 0 1
            I_ERROR[LEAKAGE_TRANSITION_1: (0.5, 2-->0)] 0 1
            M[LEAKAGE_PROJECTION_Z: (0.02, 0), (0.98, 1), (0.5, 2)] !0 1 !2 3
            """
        )
        num_shots = 32
        seed = 13579
        fh = LeakageUint8().compile_op_handler(circuit=cf, batch_size=num_shots)
        fss = FlipsideSimulator(
            cf,
            compiled_op_handler=fh,
            batch_size=num_shots,
            seed=seed,
            sync_tableside_rng=True,
        )
        fss.run()
        f_meas = fss.get_final_measurement_records()

        th = LeakageTab().compile_op_handler(circuit=ct, batch_size=1)
        tss = TablesideSimulator(
            ct,
            compiled_op_handler=th,
            batch_size=1,
            seed=seed,
            sync_flipside_rng=True,
        )
        t_meas_list, t_state_list = [], []
        for b in range(num_shots):
            if b > 0:
                tss.clear()
            tss.run()
            t_meas_list.append(tss.get_final_measurement_records()[0])
            t_state_list.append(th.state.copy())

        t_meas = np.stack(t_meas_list, axis=0)
        t_state = np.stack(t_state_list, axis=1)
        assert np.array_equal(f_meas, t_meas)
        assert np.array_equal(fh.state, t_state)

