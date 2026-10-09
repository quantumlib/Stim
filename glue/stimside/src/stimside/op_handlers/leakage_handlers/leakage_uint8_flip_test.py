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

    def test_mpad_01_keys_collapse_and_read_true_z_value(self):
        """0/1-keyed MPAD[LEAKAGE_MEASUREMENT] Z-collapses targets and reads each shot's true Z value."""

        def records(text, seed=7, batch_size=4096):
            c = stim.Circuit(text)
            cti = LeakageUint8().compile_op_handler(circuit=c, batch_size=batch_size)
            fss = FlipsideSimulator(c, batch_size=batch_size, compiled_op_handler=cti, seed=seed)
            fss.run()
            return fss, np.asarray(fss.get_final_measurement_records(), dtype=bool)

        # (a) collapse: MPAD on one half of a Bell pair destroys the XX correlation.
        _, r = records("H 0\nCX 0 1\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0] 0\nMX 0 1")
        assert 0.45 < np.mean(r[:, 1] ^ r[:, 2]) < 0.55
        # Without 0/1 keys there is no collapse.
        _, r = records("H 0\nCX 0 1\nMPAD[LEAKAGE_MEASUREMENT: (0.9, 2) : 0] 0\nMX 0 1")
        assert not np.any(r[:, 1] ^ r[:, 2])

        # (b) readout agrees with a later M.
        _, r = records("H 0 1\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1), (0.9, 2) : 1 0] 0 1\nM 0 1")
        assert not np.any(r[:, 0] ^ r[:, 3])
        # Reference Z value 1 at the MPAD: the reference walk picks 1 for the earlier M of q1.
        fss, r = records("H 0 1\nM 0 1\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0 1] 0 0\nM 0 1")
        assert fss.ref_measurements[1]
        assert not np.any(r[:, 2] ^ r[:, 4]) and not np.any(r[:, 3] ^ r[:, 5])
        # Asymmetric readout: P(r=1 | Z) = p(Z), on that reference-1 qubit.
        _, r = records("H 0 1\nM 0 1\nMPAD[LEAKAGE_MEASUREMENT: (0.1, 0), (0.7, 1) : 0 1] 0 0\nM 0 1")
        assert abs(np.mean(r[r[:, 5] == 0, 3]) - 0.1) < 0.04
        assert abs(np.mean(r[r[:, 5] == 1, 3]) - 0.7) < 0.04

        # (c) mixed: q0 known |1>, q1 superposed.
        _, r = records("X 0\nH 1\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0 1] 0 0\nM 0 1")
        assert np.all(r[:, 0]) and not np.any(r[:, 1] ^ r[:, 3])

        # (d) a detector over MPAD + M records never fires without noise.
        for text in (
            "H 0\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0] 0\nM 0\nDETECTOR rec[-1] rec[-2]",
            "H 0\nCX 0 1\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0] 0\nM 1\nDETECTOR rec[-1] rec[-2]",
        ):
            fss, _ = records(text)
            assert not np.any(fss.get_detector_flips())

        # (e) detectors/observables are relative to stim's reference sample, as in Tableside/Cosetside:
        # R1D never fires (MPAD reads q1's earlier M value); D4 always fires (MPAD reads z0, M 1 reads
        # NOT z0, while stim's reference sample has both at 0).
        from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
        from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8 as LT
        from stimside.simulator_coset import CosetsideSimulator
        from stimside.simulator_tableau import TablesideSimulator

        r1d = "H 0 1\nM 0 1\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0 1] 0 0\nM 0 1\n"
        r1d += "DETECTOR rec[-1] rec[-3]\nOBSERVABLE_INCLUDE(0) rec[-1] rec[-3]"
        d4 = "H 0\nCX 0 1\nX 1\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0] 0\nM 1\n"
        d4 += "DETECTOR rec[-1] rec[-2]\nOBSERVABLE_INCLUDE(0) rec[-1] rec[-2]"
        for text, expected in ((r1d, False), (d4, True)):
            fss, r = records(text, batch_size=100)
            c = stim.Circuit(text)
            det, obs = fss.get_detector_flips(), fss.get_observable_flips()
            assert np.all(det == expected) and np.all(obs == expected)
            # Per shot: equal to stim's detection events of Flipside's own measurement records.
            d_exp, o_exp = c.compile_m2d_converter().convert(measurements=r, separate_observables=True)
            assert np.array_equal(det.T, d_exp) and np.array_equal(obs.T, o_exp)
            packed = np.packbits(d_exp, axis=1, bitorder="little")
            assert np.array_equal(fss.get_detector_flips(bit_packed=True), packed)
            for sim_cls, handler in ((TablesideSimulator, LT), (CosetsideSimulator, LeakageUint8Coset)):
                sim = sim_cls(
                    c, compiled_op_handler=handler().compile_op_handler(circuit=c, batch_size=16),
                    batch_size=16, seed=3,
                )
                sim.run()
                assert np.all(np.asarray(sim.get_detector_flips()) == expected)
                assert np.all(np.asarray(sim.get_observable_flips()) == expected)

        # (f) the reference sample resolves rec-controlled gates from the stim record (after resets,
        # inverted results and MPAD bits), so these deterministic detectors never fire.
        for text in (
            "H 0\nM 0\nRX 2\nCX rec[-1] 1\nM 1\nDETECTOR rec[-1] rec[-2]",
            "H 0\nM !0\nCX rec[-1] 1\nM 1\nDETECTOR rec[-1] rec[-2]",
            "H 0\nM 0\nMPAD 1\nCX rec[-2] 1\nM 1\nDETECTOR rec[-1] rec[-3]",
        ):
            fss, _ = records(text, batch_size=64)
            assert not np.any(fss.get_detector_flips())


# 0/1 input and output states of LEAKAGE_TRANSITION_1/2 (Tableside/Cosetside semantics).
_LEAK2 = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) : 0] 0"
_LT1 = "I[LEAKAGE_TRANSITION_1: {}] {}"
_LT2 = "II[LEAKAGE_TRANSITION_2: {}] {}"


def _flip_run(text, batch_size=256, seed=3, **kwargs):
    circuit = stim.Circuit(text)
    cti = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=batch_size)
    fss = FlipsideSimulator(
        circuit, compiled_op_handler=cti, batch_size=batch_size, seed=seed, **kwargs
    )
    fss.run()
    return fss, np.asarray(fss.get_final_measurement_records(), dtype=bool)


@pytest.mark.parametrize(
    "prep, tag, expected",
    [
        ("X 0", "(1.0, 1-->2)", True),  # 1 used to never match
        ("", "(1.0, 1-->2)", False),
        ("X 0", "(1.0, 0-->2)", False),  # 0 used to match every unleaked qubit
        ("", "(1.0, 0-->2)", True),
    ],
)
def test_lt1_01_inputs_match_z_value(prep, tag, expected):
    _, r = _flip_run(f"{prep}\n{_LT1.format(tag, 0)}\n{_LEAK2}")
    assert np.all(r[:, 0] == expected)


def test_lt1_01_input_follows_x_frame():
    _, r = _flip_run(f"X_ERROR(0.5) 0\nM 0\n{_LT1.format('(1.0, 1-->2)', 0)}\n{_LEAK2}")
    assert 0 < r[:, 0].sum() < len(r)
    np.testing.assert_array_equal(r[:, 1], r[:, 0])


def test_lt2_01_input_legs():
    _, r = _flip_run(
        f"X 0\n{_LT2.format('(1.0, 1_0-->2_3)', '0 1')}\n{_LEAK2}\n"
        "MPAD[LEAKAGE_MEASUREMENT: (1.0, 3) : 1] 0"
    )
    assert np.all(r)


def test_lt1_01_input_collapses_superposed_target():
    # The Bell partner collapses with the target.
    _, r = _flip_run(f"H 0\nCX 0 1\n{_LT1.format('(1.0, 1-->2)', 0)}\n{_LEAK2}\nM 1", batch_size=2000)
    assert 0.4 < r[:, 0].mean() < 0.6
    np.testing.assert_array_equal(r[:, 1], r[:, 0])
    # Even a p=0 0/1-keyed op dephases |+> (H M after it is random, not always 0).
    _, r = _flip_run(f"H 0\n{_LT1.format('(0.0, 1-->2)', 0)}\nH 0\nM 0", batch_size=2000)
    assert 0.4 < r[:, 0].mean() < 0.6


@pytest.mark.parametrize(
    "text, expected",
    [
        (f"{_LT1.format('(1.0, 0-->1)', 0)}\nM 0", True),
        (f"X 0\n{_LT1.format('(1.0, 1-->0)', 0)}\nM 0", False),
        (f"X 0\n{_LT1.format('(1.0, U-->0)', 0)}\nM 0", False),
        (f"{_LT1.format('(1.0, U-->1)', 0)}\nM 0", True),
        # leaked --> 0/1 resets without depolarizing (also after an R while leaked)
        (f"{_LT1.format('(1.0, U-->2)', 0)}\n{_LT1.format('(1.0, 2-->0)', 0)}\nM 0", False),
        (f"{_LT1.format('(1.0, U-->2)', 0)}\n{_LT1.format('(1.0, 2-->1)', 0)}\nM 0", True),
        (f"{_LT1.format('(1.0, U-->2)', 0)}\nR 0\n{_LT1.format('(1.0, 2-->0)', 0)}\nM 0", False),
        (f"{_LT1.format('(1.0, U-->2)', 0)}\n{_LT2.format('(1.0, 2_U-->0_U)', '0 1')}\nM 0", False),
        (f"{_LT2.format('(1.0, U_U-->1_U)', '0 1')}\nM 0", True),
        # leaked --> X/Y/Z resets to |0> before the Pauli
        (f"{_LT1.format('(1.0, U-->2)', 0)}\n{_LT2.format('(1.0, 2_U-->X_U)', '0 1')}\nM 0", True),
        (f"{_LT1.format('(1.0, U-->2)', 0)}\n{_LT2.format('(1.0, 2_U-->Y_U)', '0 1')}\nM 0", True),
        (f"{_LT1.format('(1.0, U-->2)', 0)}\n{_LT2.format('(1.0, 2_U-->Z_U)', '0 1')}\nM 0", False),
    ],
)
def test_01_outputs_reset_to_z_eigenstate(text, expected):
    for depolarize_on_leak in (True, False):
        circuit = stim.Circuit(text)
        cti = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=256)
        cti._depolarize_on_leak = depolarize_on_leak
        fss = FlipsideSimulator(circuit, compiled_op_handler=cti, batch_size=256, seed=5)
        fss.run()
        r = np.asarray(fss.get_final_measurement_records(), dtype=bool)
        assert np.all(r[:, -1] == expected)
        assert np.all(cti.state == 0)


def test_01_output_reset_randomizes_phase_and_follows_frames():
    # A reset to |1> is a Z eigenstate with a random phase: H M is random, H H M is 1.
    _, r = _flip_run(f"{_LT1.format('(1.0, U-->1)', 0)}\nH 0\nM 0", batch_size=2000)
    assert 0.4 < r[:, 0].mean() < 0.6
    _, r = _flip_run(f"{_LT1.format('(1.0, U-->1)', 0)}\nH 0\nH 0\nM 0")
    assert np.all(r[:, 0])
    # Reset of a Bell half after a 0/1-keyed input collapse: q0 is 0, q1 is random and uncorrelated.
    _, r = _flip_run(f"H 0\nCX 0 1\n{_LT1.format('(1.0, 1-->0)', 0)}\nM 0 1", batch_size=2000)
    assert not np.any(r[:, 0]) and 0.4 < r[:, 1].mean() < 0.6
    # p < 1: only the shots that fire are reset.
    _, r = _flip_run(f"{_LT1.format('(0.3, U-->1)', '0 1')}\nM 0 1", batch_size=4000)
    assert np.all(np.abs(r.mean(axis=0) - 0.3) < 0.04)


@pytest.mark.parametrize(
    "text",
    [
        f"H 0\n{_LT1.format('(1.0, U-->0)', 0)}\nM 0",
        f"H 0\n{_LT1.format('(0.5, U-->2)', 0)}\n{_LT1.format('(1.0, 2-->0)', 0)}\nM 0",
        f"H 1\n{_LT2.format('(1.0, U_U-->U_1)', '0 1')}\nM 1",
    ],
)
def test_01_output_on_superposed_reference_raises(text):
    # Resetting only the shots that fire isn't representable in a Pauli frame over a superposed
    # reference (Tableside/Cosetside can), so FlipsideSimulator raises instead of being wrong.
    with pytest.raises(ValueError, match="Z value is definite in the noiseless circuit"):
        _flip_run(text)


def test_01_states_with_disable_stabilizer_randomization():
    # As for 0/1-keyed MPAD and plain M: a superposed target's collapse takes the reference branch.
    kw = {"disable_stabilizer_randomization": True}
    _, r_lt = _flip_run(f"H 0\n{_LT1.format('(1.0, 1-->2)', 0)}\n{_LEAK2}\nM 0", **kw)
    _, r_mpad = _flip_run("H 0\nMPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0] 0\nM 0", **kw)
    _, r_m = _flip_run("H 0\nM 0\nM 0", **kw)
    assert not np.any(r_lt) and not np.any(r_mpad) and not np.any(r_m)
    _, r = _flip_run(f"H 0\n{_LT1.format('(1.0, 0-->2)', 0)}\n{_LEAK2}", **kw)
    assert np.all(r[:, 0])
    _, r = _flip_run(f"{_LT1.format('(1.0, U-->2)', 0)}\n{_LT1.format('(1.0, 2-->1)', 0)}\nM 0", **kw)
    assert np.all(r[:, 0])


def test_01_states_without_known_states():
    kw = {"compute_known_states": False}
    # A plain (1.0, 2) MPAD readout itself needs known states (pre-existing); the 0/1-keyed one doesn't.
    leak_01 = "MPAD[LEAKAGE_MEASUREMENT: (0, 0), (0, 1), (1.0, 2) : 0] 0"
    _, r = _flip_run(f"X 0\n{_LT1.format('(1.0, 1-->2)', 0)}\n{leak_01}", **kw)
    assert np.all(r[:, 0])
    _, r = _flip_run(f"{_LT1.format('(1.0, U-->2)', 0)}\n{_LT1.format('(1.0, 2-->1)', 0)}\nM 0", **kw)
    assert np.all(r[:, 0])


def test_01_states_fused_duplicate_targets_and_repeat():
    # stim fuses identical lines into one op with duplicate targets; each copy re-reads the Z value.
    text = f"X 0\n{_LT1.format('(0.5, 1-->2)', 0)}\n{_LT1.format('(0.5, 1-->2)', 0)}\n{_LEAK2}"
    assert len(stim.Circuit(text)) == 3
    _, r = _flip_run(text, batch_size=4000)
    assert abs(r[:, 0].mean() - 0.75) < 0.03
    _, r = _flip_run(f"{_LT1.format('(1.0, 0-->1)', '0 0')}\nM 0")  # 2nd copy sees |1>
    assert np.all(r[:, 0])
    rounds = "REPEAT 3 {\nX 0\n" + _LT1.format("(1.0, 1-->2)", 0) + f"\n{_LEAK2}\n"
    rounds += _LT1.format("(1.0, 2-->0)", 0) + "\n}\nM 0"
    _, r = _flip_run("R 0\n" + rounds)
    assert np.all(r[:, :3]) and not np.any(r[:, 3])


def test_lt2_same_qubit_in_both_legs_rejected_by_stim():
    with pytest.raises(ValueError, match="same target"):
        stim.Circuit(_LT2.format("(1.0, 1_U-->2_U)", "0 0"))


def test_01_states_interactive_do():
    text = f"X 0\n{_LT1.format('(1.0, 1-->2)', 0)}\n{_LEAK2}\n{_LT1.format('(1.0, 2-->0)', 0)}\nM 0"
    circuit = stim.Circuit(text)
    cti = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=8)
    fss = FlipsideSimulator(circuit, compiled_op_handler=cti, batch_size=8, seed=1)
    fss.interactive_do(circuit)  # same op sequence as the circuit: works
    r = np.asarray(fss.get_final_measurement_records(), dtype=bool)
    assert np.all(r[:, 0]) and not np.any(r[:, 1])
    fss.clear()
    with pytest.raises(ValueError, match="doesn't match"):
        fss.interactive_do(stim.Circuit(text.splitlines()[1]))  # op at the wrong circuit time
    fss.clear()
    with pytest.raises(ValueError, match="doesn't match"):
        # Same ops at the same indices except the first: the 1-->2 op's reference Z value would
        # be the construction circuit's (1, after X 0), so q0 would leak although it is |0>.
        fss.interactive_do(stim.Circuit(text.replace("X 0", "I 0", 1)))


def test_01_states_flipside_sampler_end_to_end():
    import sinter

    from stimside.dem_generators.leakage_decoder import BaseDecoder
    from stimside.sampler_flip import FlipsideSampler

    # Every shot leaks q0 (|1> input) and resets it to |0> (output); the trailing X then gives 1 where
    # the noiseless reference gives 0, so the observable always flips and the DEM can't correct it.
    circuit = stim.Circuit(
        f"R 0 1\nX_ERROR(0.1) 1\nX 0\n{_LT1.format('(1.0, 1-->2)', 0)}\n"
        f"{_LT1.format('(1.0, 2-->0)', 0)}\nX 0\nM 0 1\nDETECTOR rec[-1]\nOBSERVABLE_INCLUDE(0) rec[-2]"
    )
    sampler = FlipsideSampler(op_handler=LeakageUint8(), batch_size=64, seed=2, dem_decoder=BaseDecoder())
    stats = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=circuit, decoder="pymatching")
    ).sample(suggested_shots=128)
    assert stats.shots >= 128
    assert stats.errors == stats.shots


def test_lt2_overlapping_pairs_read_after_reset():
    # Pairs (0,1) then (1,2) run in order: the first resets q1 to |0>, so the second (1_U) doesn't fire.
    _, r = _flip_run(
        f"X 0 1\n{_LT2.format('(1.0, 1_U-->2_0)', '0 1 1 2')}\n{_LEAK2}\n"
        "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) : 1] 0\nM 1 2"
    )
    assert np.all(r[:, 0]) and not np.any(r[:, 1:])


@pytest.mark.parametrize("p", [0.0, 1.0])
def test_01_detectors_and_observables_after_collapse_and_reset(p):
    # The reference walk collapses the Bell pair; detectors/observables must stay relative to stim's
    # reference sample. p=0: collapse only, M0 == M1. p=1: q0 is reset to |0>, M1 stays random.
    circuit = stim.Circuit(
        f"R 0 1\nH 0\nCX 0 1\n{_LT1.format(f'({p}, 1-->0)', 0)}\nM 0 1\n"
        "DETECTOR rec[-1] rec[-2]\nOBSERVABLE_INCLUDE(0) rec[-1]"
    )
    cti = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=2048)
    fss = FlipsideSimulator(circuit, compiled_op_handler=cti, batch_size=2048, seed=4)
    fss.run()
    det = np.asarray(fss.get_detector_flips(), dtype=bool)[0]
    obs = np.asarray(fss.get_observable_flips(), dtype=bool)[0]
    assert 0.4 < obs.mean() < 0.6
    if p == 0.0:
        assert not np.any(det)
    else:
        np.testing.assert_array_equal(det, obs)


@pytest.mark.parametrize("meas", ["MZZ 1 3", "MPP Z1*Z3", "MXX 1 3", "CX 1 2 3 2\nMR 2"])
def test_product_measurements_do_not_rescramble_leaked_qubits(meas):
    # Unlike M, a product or pair measurement doesn't re-scramble the leaked q1, so repeating it gives the same
    # outcome, just as with its decomposition into CXs and an ancilla measurement (last case).
    _, r = _flip_run(f"H 1\n{_LT1.format('(1.0, U-->2)', 1)}\n{meas}\nTICK\n{meas}")
    assert 0 < r[:, -1].sum() < len(r)
    np.testing.assert_array_equal(r[:, -2], r[:, -1])


def test_leakage_projection_z_depolarizes_leaked_targets_with_depolarize_on_leak_off():
    # The noiseless reference walk collapses q0 at the M, so a leaked q0 that isn't depolarized there would repeat
    # the reference's MX value in every shot.
    circuit = stim.Circuit(f"H 0\n{_LT1.format('(1.0, U-->2)', 0)}\nM[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0\nMX 0")
    cti = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=2000)
    cti._depolarize_on_leak = False
    fss = FlipsideSimulator(circuit, compiled_op_handler=cti, batch_size=2000, seed=3)
    fss.run()
    r = np.asarray(fss.get_final_measurement_records(), dtype=bool)
    assert np.all(r[:, 0]) and 0.4 < r[:, 1].mean() < 0.6


@pytest.mark.parametrize("op", ["M", "MX", "MY", "MR", "MRX", "MRY"])
def test_leaked_repeated_target_is_rescrambled_after_each_occurrence(op):
    # stim fuses identical lines (`OP 0` twice is `OP 0 0`). As with a TICK in between or a REPEAT, the
    # leaked q0 must be re-scrambled after its first occurrence (repeated targets run in order).
    prep = f"H 0\n{_LT1.format('(1.0, U-->2)', 0)}\n"
    forms = [f"{op} 0\n{op} 0", f"{op} 0 0", f"{op} 0\nTICK\n{op} 0", f"REPEAT 2 {{\n{op} 0\n}}"]
    assert stim.Circuit(prep + forms[0]) == stim.Circuit(prep + forms[1])
    for form in forms:
        _, r = _flip_run(prep + form, batch_size=4000)
        # M family: the two readouts differ half the time; MR family: the 2nd reads the scrambled qubit.
        stat = r[:, -1] if op.startswith("MR") else r[:, -2] != r[:, -1]
        assert abs(stat.mean() - 0.5) < 0.04, form
