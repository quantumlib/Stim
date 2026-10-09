import numpy as np
import pytest
import stim # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
    CompiledLeakageUint8,
    LeakageUint8,
)
from stimside.simulator_tableau import TablesideSimulator


class TestCompiledLeakageUint8:

    op_handler_batch_size = 1
    simulator_batch_size = 64

    def _get_simulator_for_circuit(self, circuit, unconditional_condition_on_U=True):
        cti = LeakageUint8(unconditional_condition_on_U).compile_op_handler(
            circuit=circuit, batch_size=self.op_handler_batch_size
            )

        return TablesideSimulator(
            circuit=circuit, batch_size=self.simulator_batch_size, compiled_op_handler=cti
        )

    def test_clear(self):
        circuit = stim.Circuit("R 0")
        tss = self._get_simulator_for_circuit(circuit)

        # Manually set some state
        tss._compiled_op_handler.state[:] = 5
        assert np.any(tss._compiled_op_handler.state != 0)

        tss._compiled_op_handler.clear()
        assert np.all(tss._compiled_op_handler.state == 0)
    def test_make_target_mask(self):
        circuit = stim.Circuit("CX 0 1 2 3")
        op = circuit[0]

        # We don't need a full simulator for this, just the compiled op handler
        cti: CompiledLeakageUint8 = LeakageUint8().compile_op_handler(
            circuit=circuit, batch_size=self.op_handler_batch_size
        )

        mask = cti.make_target_mask(op)

        expected_mask = np.zeros(circuit.num_qubits, dtype=bool)
        expected_mask[0] = True
        expected_mask[1] = True
        expected_mask[2] = True
        expected_mask[3] = True

        assert np.array_equal(mask, expected_mask)

    def test_handle_op(self):
        import dataclasses

        @dataclasses.dataclass
        class FakeLeakageParams:
            name: str = "FAKE_LEAKAGE"
            from_tag: str = "FAKE"

        circuit = stim.Circuit("I 0")  # A dummy instruction
        op = circuit[0]

        tss = self._get_simulator_for_circuit(circuit)

        # Manually insert the fake params into the handler's dictionary
        tss._compiled_op_handler.ops_to_params[op] = FakeLeakageParams()
        tss._compiled_op_handler.claimed_ops_keys = set(
            tss._compiled_op_handler.ops_to_params.keys()
            )

        with pytest.raises(ValueError, match="Unrecognised LEAKAGE params"):
            tss._do_instruction(op)

    def test_leakage_transition_1(self):
        # Part a: Computational to Leaked
        circuit_a = stim.Circuit(
            """
            R 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 1
        """
        )
        tss_a = self._get_simulator_for_circuit(circuit_a)
        tss_a.interactive_do(circuit_a[0])
        assert np.all(tss_a._compiled_op_handler.state[:] == 0)
        tss_a.interactive_do(circuit_a[1])
        assert np.all(tss_a._compiled_op_handler.state[:] == 2)

        # Part b: Leaked to Leaked
        circuit_b = stim.Circuit("I[LEAKAGE_TRANSITION_1: (1.0, 2-->3)] 0 1")
        tss_b = self._get_simulator_for_circuit(circuit_b)
        tss_b._compiled_op_handler.state[:] = 2
        tss_b.interactive_do(circuit_b[0])
        assert np.all(tss_b._compiled_op_handler.state[:] == 3)

        # Part c: Leaked to Computational
        circuit_c = stim.Circuit(
            """
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 1
            M 0 1
            """
            )
        tss_c = self._get_simulator_for_circuit(circuit_c)
        tss_c._compiled_op_handler.state[:] = 2
        tss_c.run()
        records = tss_c.get_final_measurement_records()
        assert np.all(tss_c._compiled_op_handler.state[:] == 0)
        assert np.any(records[:, :] == 0)
        assert np.any(records[:, :] == 1)
        circuit_c = stim.Circuit(
            """
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 1
            MX 0 1
            """
            )
        tss_c = self._get_simulator_for_circuit(circuit_c)
        tss_c._compiled_op_handler.state[:] = 2
        tss_c.run()
        records = tss_c.get_final_measurement_records()
        assert np.all(tss_c._compiled_op_handler.state[:] == 0)
        assert np.any(records[:, :] == 0)
        assert np.any(records[:, :] == 1)

    def test_leakage_transition_2(self):
        # Test case: (U, U) -> (L2, L3)
        circuit = stim.Circuit(
            """
            R 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_3)] 0 1
        """
        )
        tss = self._get_simulator_for_circuit(circuit)
        tss.run()

        # Check leakage states
        assert np.all(tss._compiled_op_handler.state[0] == 2)
        assert np.all(tss._compiled_op_handler.state[1] == 3)

        # Test case: (L2, U) -> (L4, V)
        # This should depolarize qubit 1 (U->V) but not qubit 0 (L->L)
        circuit = stim.Circuit(
            """
            R 0 1
            H 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->4_V)] 0 1
            MX 0 1
        """
        )
        tss = self._get_simulator_for_circuit(circuit)
        tss._compiled_op_handler.state[0] = 2  # Manually set state of qubit 0
        tss.run()

        # Check leakage states
        assert np.all(tss._compiled_op_handler.state[0] == 4)
        assert np.all(tss._compiled_op_handler.state[1] == 0)  
        # V becomes 0 due to U->V (placeholder 0)

        # # Check depolarization
        # # Qubit 1 (U->V) should be depolarized
        records = tss.get_final_measurement_records()
        assert np.any(records[:, 1] == 0)
        assert np.any(records[:, 1] == 1)

        # Test case: (L2, U) -> (1, X)
        circuit = stim.Circuit(
            """
            R 0 1
            H 0 1
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->1_X)] 0 1
        """
        )
        tss = self._get_simulator_for_circuit(circuit)
        tss.interactive_do(circuit[0])
        tss.interactive_do(circuit[1])
        tss.interactive_do(circuit[2])
        assert np.all(tss._compiled_op_handler.state[0] == 2)  
        tss.interactive_do(circuit[3])
        assert np.all(tss.peek_z(0) == -1)
        assert np.all(tss._compiled_op_handler.state[1] == 0)  
        # X becomes 0 due to U->X (placeholder 0)

        circuit.append_from_stim_program_text("MX 0 1")
        tss.interactive_do(circuit[4])
        tss.finish_interactive_run()
        records = tss.get_final_measurement_records()
        assert np.all(records[:, 1] == 0)
        assert np.any(records[:, 0] == 0)
        assert np.any(records[:, 0] == 1)

    def test_leakage_projection_Z(self):
        circuit = stim.Circuit(
            """
            X 1
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->2)] 2
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->3)] 3
            M[LEAKAGE_PROJECTION_Z: (1.0, 0) (0.0, 1) (1.0, 2) (0.5, 3)] 0 1 2 3
        """
        )
        # the 'known state' for all qubits is 0, so meas_flip should correspond to outcome
        # readout errors are 'backwards'
        # 0 states look like 1s with 100% prob
        # 1 states look like 0s with 100% prob
        # 2s look like 1s with 100% probability
        # 3s look 50:50 random
        tss = self._get_simulator_for_circuit(circuit)

        tss.interactive_do(circuit[0])
        tss.interactive_do(circuit[1])
        tss.interactive_do(circuit[2])

        in_Z0 = tss.peek_z(0)
        in_Z1 = tss.peek_z(1)
        in_Z2 = tss._compiled_op_handler.state[2]
        in_Z3 = tss._compiled_op_handler.state[3]
        assert in_Z0 == 1 # q0 is in 0
        assert in_Z1 == -1  # q1 is in 1
        assert in_Z2 == 2  # q2 is leaked to 2
        assert in_Z3 == 3  # q3 is leaked to 3

        tss.interactive_do(circuit[3])  # finally, do the leakage projection instruction

        # measurement flips are what they're supposed to be
        tss.finish_interactive_run()
        records = tss.get_final_measurement_records()

        assert np.all(records[:, 0] == 1)  # qubit 0 was in 0, should read out as all 1s
        assert np.all(records[:, 1] == 0)  # qubit 1 was in 1, should read out as all 0s
        assert np.all(records[:, 2] == 1)  # qubit 2 was in 2, should read out as all 1s
        assert not np.all(records[:, 3] == 0)  # qubit 3 was in 3, should be 50:50
        assert not np.all(records[:, 3] == 1)  # we check it's not all 0s and not all 1s

        # leakage states haven't changed
        print(tss._compiled_op_handler.state)
        assert np.all(
            tss._compiled_op_handler.state
            == np.array([0, 0, 2, 3])
            )

    def test_leakage_measurement(self):
        circuit = stim.Circuit(
            """
            X 1
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->2)] 2
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->3)] 3
            MPAD[LEAKAGE_MEASUREMENT: (1.0, 0) (0.0, 1) (1.0, 2) (0.5, 3): 0 1 2 3] 0 0 0 0 
        """
        )
        # the 'known state' for all qubits is 0, so meas_flip should correspond to outcome
        # readout errors are 'backwards'
        # 0 states look like 1s with 100% prob
        # 1 states look like 0s with 100% prob
        # 2s look like 1s with 100% probability
        # 3s look 50:50 random
        tss = self._get_simulator_for_circuit(circuit)

        tss.interactive_do(circuit[0])
        tss.interactive_do(circuit[1])
        tss.interactive_do(circuit[2])

        in_Z0 = tss.peek_z(0)
        in_Z1 = tss.peek_z(1)
        in_Z2 = tss._compiled_op_handler.state[2]
        in_Z3 = tss._compiled_op_handler.state[3]
        assert in_Z0 == 1 # q0 is in 0
        assert in_Z1 == -1  # q1 is in 1
        assert in_Z2 == 2  # q2 is leaked to 2
        assert in_Z3 == 3  # q3 is leaked to 3

        tss.interactive_do(circuit[3])  # finally, do the leakage projection instruction

        # measurement flips are what they're supposed to be
        tss.finish_interactive_run()
        records = tss.get_final_measurement_records()

        print(tss._new_circuit)

        assert np.all(records[:, 0] == 1)  # qubit 0 was in 0, should read out as all 1s
        assert np.all(records[:, 1] == 0)  # qubit 1 was in 1, should read out as all 0s
        assert np.all(records[:, 2] == 1)  # qubit 2 was in 2, should read out as all 1s
        assert not np.all(records[:, 3] == 0)  # qubit 3 was in 3, should be 50:50
        assert not np.all(records[:, 3] == 1)  # we check it's not all 0s and not all 1s

        # leakage states haven't changed
        print(tss._compiled_op_handler.state)
        assert np.all(
            tss._compiled_op_handler.state
            == np.array([0, 0, 2, 3])
            )
        
    def test_conditional_ops(self):
       # Part a: Test universal conditioning on U
        circuit_a = stim.Circuit(
            """
            H 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            CZ 0 1
            H 0 1
            M 1
        """
        )
        tss_a = self._get_simulator_for_circuit(circuit_a)
        tss_a.run()
        records = tss_a.get_final_measurement_records()
        assert tss_a._compiled_op_handler.state[0] == 2
        assert np.all(records[:, 0] == 0)

        # Now turning universal conditioning on U off
        tss_a = self._get_simulator_for_circuit(circuit_a, unconditional_condition_on_U=False)
        tss_a.run()
        records = tss_a.get_final_measurement_records()
        assert tss_a._compiled_op_handler.state[0] == 2
        ## Check that now 1 is not deterministic
        assert np.any(records[:, 0] == 0)
        assert np.any(records[:, 0] == 1)

        # Part b: Test conditioning on pair (U, U) with universal conditioning on U off
        circuit_b = stim.Circuit(
            """
            H 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 1
            H 0 1
            M 1
        """
        )
        tss_b = self._get_simulator_for_circuit(circuit_b, unconditional_condition_on_U=False)
        tss_b.run()
        records = tss_b.get_final_measurement_records()
        assert tss_b._compiled_op_handler.state[0] == 2
        assert np.all(records[:, 0] == 0)

        # Part c: Test conditioning on other with universal conditioning on U off
        circuit_c = stim.Circuit(
            """
            R 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 2
            X[CONDITIONED_ON_OTHER: U: 0 2] 1 3
            M 1 3
        """
        )
        tss_c = self._get_simulator_for_circuit(circuit_c, unconditional_condition_on_U=False)
        tss_c.run()
        records = tss_c.get_final_measurement_records()
        assert tss_c._compiled_op_handler.state[0] == 2
        assert tss_c._compiled_op_handler.state[2] == 2
        assert np.all(records[:, :] == 0)

    def test_bell_entanglement_preserved_in_leakage_measurement_and_peek_z(self):
        circuit = stim.Circuit(
            """
            H 0
            CX 0 1
            M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0
            M 1
        """
        )
        tss = self._get_simulator_for_circuit(circuit)
        tss.run()
        records = tss.get_final_measurement_records()
        assert np.array_equal(records[:, 0], records[:, 1])

        # Also test batch_size=1 and _update_state_from_peek_Z on Bell state
        c_bell = stim.Circuit("H 0\nCX 0 1\nI[LEAKAGE_TRANSITION_1: (1.0, 1-->2)] 0 1")
        for seed in range(10):
            cti = LeakageUint8().compile_op_handler(circuit=c_bell, batch_size=1)
            tss_1 = TablesideSimulator(
                circuit=c_bell, batch_size=1, compiled_op_handler=cti, seed=seed
            )
            tss_1.run()
            # Either both qubits projected to |0> (state [0, 0]) or both projected to |1> and leaked to [2, 2]
            assert tss_1._compiled_op_handler.state[0] == tss_1._compiled_op_handler.state[1]

    def test_mpad_defaults_and_leaked_ancillas(self):
        circuit = stim.Circuit(
            """
            X 0
            H 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 2
            MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) : 0 1 2] 0 0 0
            MX 1
        """
        )
        tss = self._get_simulator_for_circuit(circuit)
        tss.run()
        records = tss.get_final_measurement_records()
        # q0 in |1> and q1 in |+> should read 0 (unspecified 1 defaults to 0.0, not 1.0)
        assert np.all(records[:, 0] == 0)
        assert np.all(records[:, 1] == 0)
        # q2 leaked in state 2 should read 1
        assert np.all(records[:, 2] == 1)
        # q1 in |+> was not collapsed by MPAD, so MX 1 is deterministically 0
        assert np.all(records[:, 3] == 0)

    def test_leakage_transition_1_no_chaining_and_no_stale_1(self):
        circuit = stim.Circuit(
            """
            R 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 1
            I[LEAKAGE_TRANSITION_1: (1.0, U<->2)] 0 1
        """
        )
        tss = self._get_simulator_for_circuit(circuit)
        tss.run()
        assert tss._compiled_op_handler.state[0] == 2
        assert tss._compiled_op_handler.state[1] == 0

        # Check stale 1s do not persist after X flips |1> back to |0>
        c_stale = stim.Circuit(
            """
            R 0
            X 0
            I[LEAKAGE_TRANSITION_1: (0.0, 1-->2)] 0
            X 0
            I[LEAKAGE_TRANSITION_1: (1.0, 1-->2)] 0
        """
        )
        tss_s = self._get_simulator_for_circuit(c_stale)
        tss_s.run()
        assert tss_s._compiled_op_handler.state[0] == 0

    def test_leakage_transition_2_regression_fixes(self):
        # 1. Computational tuple keys (0_1-->2_3) & non-ascending pair targets (1 0)
        c1 = stim.Circuit(
            """
            R 0 1
            X 0
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 0_1-->2_3)] 1 0
        """
        )
        tss1 = self._get_simulator_for_circuit(c1)
        tss1.run()
        assert tss1._compiled_op_handler.state[1] == 2
        assert tss1._compiled_op_handler.state[0] == 3

        # 2. Overlapping pairs (0 1 1 2) without ValueError
        c_overlap = stim.Circuit(
            """
            R 0 1 2
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->0_1)] 0 1 1 2
        """
        )
        tss_ov = self._get_simulator_for_circuit(c_overlap)
        tss_ov.run()
        assert tss_ov.peek_z(2) == -1

        # 3. 'V' depolarizes in Z basis as well as X basis (not deterministic |1>)
        c_v = stim.Circuit(
            """
            R 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->V_V)] 0 1
            M 0 1
        """
        )
        tss_v = self._get_simulator_for_circuit(c_v)
        tss_v.run()
        recs_v = tss_v.get_final_measurement_records()
        assert np.any(recs_v[:, 0] == 0) and np.any(recs_v[:, 0] == 1)

        # 4. Leaked input states to D and X/Y/Z
        c_lx = stim.Circuit(
            """
            R 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_2-->D_X)] 0 1
            M 0 1
        """
        )
        tss_lx = self._get_simulator_for_circuit(c_lx)
        tss_lx.run()
        recs_lx = tss_lx.get_final_measurement_records()
        assert np.all(tss_lx._compiled_op_handler.state == 0)
        assert np.any(recs_lx[:, 0] == 0) and np.any(recs_lx[:, 0] == 1)
        assert np.all(recs_lx[:, 1] == 1)

    def test_target_modifiers_and_correlated_error(self, capsys):
        circuit = stim.Circuit(
            """
            R 0 1 2
            X 0
            M !0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 2
            CX rec[-1] 1 rec[-1] 2
            E(1.0) X1 X2
            M 1 2
        """
        )
        tss = self._get_simulator_for_circuit(circuit)
        tss.run()
        captured = capsys.readouterr()
        assert "Not sure why this op" not in captured.out
        records = tss.get_final_measurement_records()
        # M !0 on |1> gives 0
        assert np.all(records[:, 0] == 0)
        # E(1.0) X1 X2 is skipped entirely because qubit 2 is leaked (like a 2q pair), so qubit 1 stays |0>
        assert np.all(records[:, 1] == 0)
        assert np.all(records[:, 2] == 0)

    def test_no_spurious_depolarize_or_reset_on_untriggered_transitions(self):
        # 0% probability transition from U, 0, or 1 must not depolarize or reset the qubit
        circuit = stim.Circuit(
            """
            R 0 1 2
            X 1
            H 2
            I[LEAKAGE_TRANSITION_1: (0.0, U-->2)] 0 1 2
            I[LEAKAGE_TRANSITION_1: (0.0, 0-->2) (0.0, 1-->2)] 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (0.0, 0_1-->2_2)] 0 1
            M 0 1
            MX 2
        """
        )
        tss = self._get_simulator_for_circuit(circuit)
        tss.run()
        records = tss.get_final_measurement_records()
        assert np.all(records[:, 0] == 0)
        assert np.all(records[:, 1] == 1)
        assert np.all(records[:, 2] == 0)

    def test_asymmetric_readout_bell_entanglement_and_chained_conditioned_pair_rec(self):
        # Asymmetric readout error (p0=0.0, p1=0.95) on a Bell state preserves entanglement
        # in both batch_size > 1 and batch_size == 1
        c_asym = stim.Circuit(
            """
            H 0
            CX 0 1
            M[LEAKAGE_PROJECTION_Z: (0.0, 0) (0.95, 1)] 0
            M 1
        """
        )
        tss_batch = self._get_simulator_for_circuit(c_asym)
        tss_batch.run()
        recs_b = tss_batch.get_final_measurement_records()
        # Whenever M 1 == 0, M 0 must be 0 (since p0 == 0.0 and q0/q1 are entangled)
        assert np.all(recs_b[recs_b[:, 1] == 0, 0] == 0)
        assert np.mean(recs_b[:, 0] == recs_b[:, 1]) > 0.85

        for seed in range(20):
            cti_1 = LeakageUint8().compile_op_handler(circuit=c_asym, batch_size=1)
            tss_1 = TablesideSimulator(
                circuit=c_asym, batch_size=1, compiled_op_handler=cti_1, seed=seed
            )
            tss_1.run()
            r1 = tss_1.get_final_measurement_records()[0]
            if r1[1] == 0:
                assert r1[0] == 0

        # Chained CONDITIONED_ON_PAIR with classical rec[-k] targets
        c_rec = stim.Circuit(
            """
            R 0 1 2
            X 0
            M 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 2
            CX[CONDITIONED_ON_PAIR: (U, U)] rec[-1] 1 rec[-1] 2
            CZ[CONDITIONED_ON_PAIR: (U, 1)] rec[-1] 1 rec[-1] 2
            M 1 2
        """
        )
        tss_rec = self._get_simulator_for_circuit(
            c_rec, unconditional_condition_on_U=False
        )
        tss_rec.run()
        recs_rec = tss_rec.get_final_measurement_records()
        assert np.all(recs_rec[:, 1] == 1)
        assert np.all(recs_rec[:, 2] == 0)

    def test_tableside_v2_cpp_and_python_kernels_statistical_consistency(self):
        circ = stim.Circuit(
            """
            R 0 1 2 3
            X_ERROR(0.3) 0 1 2 3
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 1 2 3
            II_ERROR[LEAKAGE_TRANSITION_2: (0.2, U_U-->2_D)] 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (0.25, 1-->2)] 0 1 2 3
            M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0 1 2 3
            R 0 1
            I[LEAKAGE_TRANSITION_1: (0.5, 2-->0) (0.5, 2-->U)] 0 1
            M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0 1 2 3
            DETECTOR rec[-4] rec[-8]
            OBSERVABLE_INCLUDE(0) rec[-1]
        """
        )
        det_means = {}
        leak_means = {}
        for use_cpp in (True, False):
            coh = LeakageUint8(unconditional_condition_on_U=True).compile_op_handler(
                circuit=circ, batch_size=1
            )
            tss = TablesideSimulator(
                circuit=circ,
                compiled_op_handler=coh,
                batch_size=1,
                seed=777,
                use_cpp_kernels=use_cpp,
            )
            dets = []
            leaks = []
            for _ in range(120):
                tss.clear()
                coh.clear()
                tss.run()
                dets.append(float(tss.get_detector_flips()[0, 0]))
                leaks.append(float(np.mean(coh.state >= 2)))
            det_means[use_cpp] = np.mean(dets)
            leak_means[use_cpp] = np.mean(leaks)

        assert abs(det_means[True] - det_means[False]) < 0.15
        assert abs(leak_means[True] - leak_means[False]) < 0.12
        assert leak_means[True] > 0.05

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_exotic_multilevel_leakage_hierarchy_and_output_tokens(self, use_cpp: bool):
        # 1. Cascaded multi-level heating & cooling chain: 0 -> 2 -> 3 -> 4 -> 5 -> 1 -> U
        c_cascade = stim.Circuit(
            """
            R 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->2)] 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, 2<->3)] 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, 3-->4)] 0
            I[LEAKAGE_TRANSITION_1: (1.0, 4-->5)] 0
            I[LEAKAGE_TRANSITION_1: (1.0, 5-->1)] 0
            I[LEAKAGE_TRANSITION_1: (1.0, 3-->1)] 1
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->1)] 2
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->U)] 3
            M 0 1 2 3
            """
        )
        coh = LeakageUint8(unconditional_condition_on_U=True).compile_op_handler(
            circuit=c_cascade, batch_size=1
        )
        tss = TablesideSimulator(
            circuit=c_cascade,
            compiled_op_handler=coh,
            batch_size=16,
            seed=42,
            use_cpp_kernels=use_cpp,
        )
        tss.run()
        recs = tss.get_final_measurement_records()
        assert list(coh.state) == [0, 0, 0, 0]
        # q0 transitioned 0 -> 2 -> 3 -> 4 -> 5 -> 1 -> |1>
        assert np.all(recs[:, 0] == 1)
        # q1 transitioned 0 -> 2 -> 3 -> 1 -> |1>
        assert np.all(recs[:, 1] == 1)
        # q2 transitioned 0 -> 1 -> |1>
        assert np.all(recs[:, 2] == 1)
        # q3 transitioned 0 -> U (depolarized)
        assert 0.1 < np.mean(recs[:, 3]) < 0.9

        # 2. LEAKAGE_TRANSITION_2 with U_3 --> V_Y, 1_2 --> X_Z, U_2 --> D_0
        c_t2 = stim.Circuit(
            """
            R 0 1 2 3 4 5
            X 2
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 3 5
            I[LEAKAGE_TRANSITION_1: (1.0, U-->3)] 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_3-->V_Y)] 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 1_2-->X_Z)] 2 3
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_2-->D_0)] 4 5
            M 0 1 2 3 4 5
            """
        )
        coh2 = LeakageUint8(unconditional_condition_on_U=True).compile_op_handler(
            circuit=c_t2, batch_size=1
        )
        tss2 = TablesideSimulator(
            circuit=c_t2,
            compiled_op_handler=coh2,
            batch_size=128,
            seed=123,
            use_cpp_kernels=use_cpp,
        )
        tss2.run()
        recs2 = tss2.get_final_measurement_records()
        assert list(coh2.state) == [0, 0, 0, 0, 0, 0]
        # q0 -> V: randomized 50/50
        assert 0.25 < np.mean(recs2[:, 0]) < 0.75
        # q1 -> Y from leaked state 3: reset to |0> then Y -> |1>
        assert np.all(recs2[:, 1] == 1)
        # q2 was |1>, transitioned to X -> X|1> = |0>
        assert np.all(recs2[:, 2] == 0)
        # q3 was leaked 2, transitioned to Z -> reset to |0> then Z -> |0>
        assert np.all(recs2[:, 3] == 0)
        # q4 -> D: depolarized 50/50
        assert 0.25 < np.mean(recs2[:, 4]) < 0.75
        # q5 -> 0: reset to |0>
        assert np.all(recs2[:, 5] == 0)

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_exotic_conditional_cliffords_and_other_targets(self, use_cpp: bool):
        circuit = stim.Circuit(
            """
            R 0 1 2 3 4 5
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->3)] 1
            X[CONDITIONED_ON_SELF: 2] 0 2
            X[CONDITIONED_ON_SELF: 2 3] 0 1 2
            SQRT_X[CONDITIONED_ON_OTHER: 2 : 0] 2
            SQRT_X[CONDITIONED_ON_OTHER: 3 : 1] 2
            C_XYZ[CONDITIONED_ON_OTHER: 2 3 : 0] 3
            C_ZYX[CONDITIONED_ON_OTHER: 2 : 0] 3
            CX[CONDITIONED_ON_PAIR: (U, U)] 2 3 0 4
            CX[CONDITIONED_ON_PAIR: (U, U)] 3 4
            ISWAP[CONDITIONED_ON_PAIR: (2, 3)] 0 1 2 3
            SQRT_XX[CONDITIONED_ON_PAIR: (U, U)] 2 3
            SQRT_XX_DAG[CONDITIONED_ON_PAIR: (U, U)] 2 3
            SQRT_YY[CONDITIONED_ON_PAIR: (U, U)] 2 3
            SQRT_YY_DAG[CONDITIONED_ON_PAIR: (U, U)] 2 3
            SQRT_ZZ[CONDITIONED_ON_PAIR: (U, U)] 2 3
            SQRT_ZZ_DAG[CONDITIONED_ON_PAIR: (U, U)] 2 3
            M 2 3 4
            """
        )
        coh = LeakageUint8(unconditional_condition_on_U=False).compile_op_handler(
            circuit=circuit, batch_size=1
        )
        tss = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=coh,
            batch_size=16,
            seed=77,
            use_cpp_kernels=use_cpp,
        )
        tss.run()
        recs = tss.get_final_measurement_records()
        # q2 had two SQRT_X gates (which equals X) -> |1>
        # CX[CONDITIONED_ON_SELF: U] 2 3 flipped q3 to |1>, while 0 4 was skipped because q0 is leaked (2)
        # CX[CONDITIONED_ON_OTHER: 2 : 0] 3 4 flipped q4 to |1> because q0 is in state 2
        assert np.all(recs[:, 0] == 1)
        assert np.all(recs[:, 1] == 1)
        assert np.all(recs[:, 2] == 1)

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_exotic_measurements_resets_repeat_blocks_and_noise(self, use_cpp: bool):
        circuit = stim.Circuit(
            """
            RX 0
            RY 1
            MX 0
            MY 1
            R 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (1.0, U-->3)] 2
            MPAD[LEAKAGE_MEASUREMENT: (0.0, 0) (0.0, 1) (1.0, 2) (1.0, 3) : 2 3] 0 0
            CX rec[-2] 0 rec[-1] 1
            M[LEAKAGE_PROJECTION_Z: (0.0, 0) (1.0, 1) (0.5, 2) (1.0, 3)] !2 3
            REPEAT 2 {
                H 0 1
                MXX 0 1
                MYY 0 1
                MZZ 0 1
                MPP X0*X1 Z0*Z1
                H 0 1
                SHIFT_COORDS(1, 0)
                DETECTOR(0, 0) rec[-1] rec[-4]
            }
            R 0 1
            CX rec[-14] 0 rec[-13] 1
            CORRELATED_ERROR(1.0) X2 Z0
            ELSE_CORRELATED_ERROR(1.0) X1
            CORRELATED_ERROR(0.0) X0
            ELSE_CORRELATED_ERROR(1.0) X3
            M 0 1 3
            """
        )
        coh = LeakageUint8(unconditional_condition_on_U=True).compile_op_handler(
            circuit=circuit, batch_size=1
        )
        tss = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=coh,
            batch_size=16,
            seed=99,
            use_cpp_kernels=use_cpp,
        )
        tss.run()
        recs = tss.get_final_measurement_records()
        # RX 0 -> MX 0 is 0; RY 1 -> MY 1 is 0
        assert np.all(recs[:, 0] == 0)
        assert np.all(recs[:, 1] == 0)
        # MPAD on q2 (state 3) -> 1, on q3 (state 0) -> 0
        assert np.all(recs[:, 2] == 1)
        assert np.all(recs[:, 3] == 0)
        # M[LEAKAGE_PROJECTION_Z: ... (1.0, 3)] !2 -> inverted 1 is 0!
        assert np.all(recs[:, 4] == 0)
        # Final M 0 1 3:
        # q0 was flipped to |1> by CX rec[-2] 0; q1 stayed |0> (ELSE_CORRELATED_ERROR(1.0) X1 did not fire because CORRELATED_ERROR(1.0) fired);
        # q3 was flipped to |1> by ELSE_CORRELATED_ERROR(1.0) X3 (since CORRELATED_ERROR(0.0) did not fire)
        assert np.all(recs[:, -3] == 1)
        assert np.all(recs[:, -2] == 0)
        assert np.all(recs[:, -1] == 1)

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_2q_single_group_conditioned_on_cartesian_and_sync_rng_other(
        self, use_cpp: bool
    ):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
            X 1 3
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->3)] 1
            # q0 is in state 2, q1 is in state 3 -> (2, 3) matches, so CX must fire and flip q1!
            CX[CONDITIONED_ON_PAIR: (2, 3) (3, 2)] 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->0) (1.0, 3-->0)] 0 1
            # Re-leak q0 -> 2, keep q2 in 0, and test CONDITIONED_ON_OTHER: U : 0 on X 2
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            X[CONDITIONED_ON_OTHER: U : 0] 2
            # Test 1-->3 followed by M[LEAKAGE_PROJECTION_Z: (1.0, 2)] on q3 (in |1>)
            I[LEAKAGE_TRANSITION_1: (1.0, 1-->3)] 3
            M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 3
            M 2
            """
        )
        for sync_rng in (False, True):
            coh = LeakageUint8(
                unconditional_condition_on_U=False
            ).compile_op_handler(circuit=circuit, batch_size=1)
            tss = TablesideSimulator(
                circuit=circuit,
                compiled_op_handler=coh,
                batch_size=8,
                seed=123,
                use_cpp_kernels=use_cpp,
                sync_tableside_rng=sync_rng,
            )
            tss.run()
            recs = tss.get_final_measurement_records()
            # q3 transitioned 1-->3, and M[LEAKAGE_PROJECTION_Z: (1.0, 2)] has prob 0.0 for state 3 (not fused with state 2!)
            assert np.all(recs[:, 0] == 0)
            # X[CONDITIONED_ON_OTHER: U : 0] 2 must be skipped because controller q0 is in state 2
            assert np.all(recs[:, 1] == 0)

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_mpad_01_readout_collapses_and_tracks_each_shot_at_batch_size_gt_1(self, use_cpp: bool):
        """Non-sync batch_size > 1: MPAD[LEAKAGE_MEASUREMENT] with 0/1 keys Z-collapses superposed targets and reads
        each shot's own Z value (incl. Pauli-frame flips), as batch_size == 1 and Cosetside do."""
        p0, p1 = 0.1, 0.7
        tag = "(0.1, 0), (0.7, 1), (0.9, 2)"

        def recs(text: str) -> np.ndarray:
            c = stim.Circuit(text)
            out = []
            for s in range(10):
                op = LeakageUint8().compile_op_handler(circuit=c, batch_size=400)
                sim = TablesideSimulator(
                    circuit=c, compiled_op_handler=op, batch_size=400, seed=7 + s, use_cpp_kernels=use_cpp
                )
                sim.run()
                out.append(sim.get_final_measurement_records())
            return np.vstack(out).astype(bool)

        # Bell pair, MPAD on q0: collapse => XX parity becomes random.
        r_x = recs(f"H 0\nCX 0 1\nMPAD[LEAKAGE_MEASUREMENT: {tag} : 0] 0\nH 0 1\nM 0 1")
        assert abs(np.mean(r_x[:, 1] ^ r_x[:, 2]) - 0.5) < 4.5 * 0.5 / np.sqrt(len(r_x))
        # Readout is p0/p1 noise on each shot's Z value: Bell partner, and a frame-only X_ERROR flip (inverted pad).
        for text, inv in (
            (f"H 0\nCX 0 1\nMPAD[LEAKAGE_MEASUREMENT: {tag} : 0] 0\nM 1", False),
            (f"X_ERROR(0.5) 0\nMPAD[LEAKAGE_MEASUREMENT: {tag} : 0] 1\nM 0", True),
        ):
            r_z = recs(text)
            r, m = r_z[:, 0] ^ inv, r_z[:, 1]
            for mask, p in ((~m, p0), (m, p1)):
                assert abs(np.mean(r[mask]) - p) < 4.5 * np.sqrt(p * (1 - p) / mask.sum())

    @pytest.mark.parametrize("use_cpp", [True, False])
    @pytest.mark.parametrize("gate", ["SPP", "SPP_DAG"])
    def test_spp_terms_on_leaked_qubits_are_skipped(self, use_cpp: bool, gate: str):
        # Like a 2q pair, an SPP/SPP_DAG Pauli product term is skipped iff it touches a leaked qubit; the other
        # terms still apply. Each term appears twice, so when applied it composes to its Pauli (X3 / X4).
        circuit = stim.Circuit(
            f"""
            R 1 3 4
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 1
            {gate} Z1*X3 X4 X4 Z1*X3
            M 3 4
            """
        )
        for batch_size, sync_rng in ((1, False), (16, False), (16, True)):
            coh = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=1)
            tss = TablesideSimulator(
                circuit=circuit,
                compiled_op_handler=coh,
                batch_size=batch_size,
                seed=5,
                use_cpp_kernels=use_cpp,
                sync_tableside_rng=sync_rng,
            )
            tss.run()
            recs = tss.get_final_measurement_records()
            assert np.all(recs[:, 0] == 0)
            assert np.all(recs[:, 1] == 1)

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_correlated_error_on_a_leaked_qubit_is_skipped_like_pauli_channel_2(self, use_cpp: bool):
        # E(1) X1 X3 is the same channel as PAULI_CHANNEL_2(XX=1) 1 3, so with q1 leaked it must not flip q3 either.
        # A skipped E still counts as fired, so the following ELSE_CORRELATED_ERROR stays blocked.
        pc2_xx = "PAULI_CHANNEL_2(0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) 1 3"
        for noise, expected in (
            ("E(1) X1 X3", [0, 0]),
            ("E(1) X3 X1", [0, 0]),
            (pc2_xx, [0, 0]),
            ("E(1) X1 X3\nELSE_CORRELATED_ERROR(1) X4", [0, 0]),
            ("E(1) X3 X4", [1, 1]),
        ):
            circuit = stim.Circuit(f"R 1 3 4\nI[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 1\n{noise}\nM 3 4")
            for batch_size, sync_rng in ((1, False), (16, False), (16, True)):
                coh = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=1)
                tss = TablesideSimulator(
                    circuit=circuit,
                    compiled_op_handler=coh,
                    batch_size=batch_size,
                    seed=5,
                    use_cpp_kernels=use_cpp,
                    sync_tableside_rng=sync_rng,
                )
                tss.run()
                assert np.all(tss.get_final_measurement_records() == expected), (noise, batch_size, sync_rng)

    @pytest.mark.parametrize(
        "op_text, expected, to_python",
        [
            ("SPP X1*X2 X1*X2", [1, 1], False),
            ("SPP_DAG Y1*Z2 Y1*Z2", [1, 0], False),
            ("E(1) X1*X2", [1, 1], False),
            ("CX sweep[0] 1", [0, 0], False),
            ("CX[CONDITIONED_ON_PAIR: (U, U)] sweep[0] 1", [0, 0], False),
            ("SPP X0*X1 X0*X1", [0, 0], True),
        ],
    )
    def test_cpp_engine_ignores_combiner_and_sweep_targets_when_checking_for_leaked_qubits(
        self, op_text: str, expected: list[int], to_python: bool, monkeypatch: pytest.MonkeyPatch
    ):
        # The C++ engine hands such an op to Python only if one of its qubits is leaked. A `*` combiner or sweep[0]
        # is not qubit 0, so the leaked q0 must not send the op to Python (unless q0 is a real target).
        circuit = stim.Circuit(f"R 0 1 2\nI[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0\n{op_text}\nM 1 2")
        handled: list[stim.CircuitInstruction] = []
        for batch_size in (1, 16):
            coh = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=1)

            def spy(op, sss, handle_op=coh.handle_op):
                handled.append(op)
                handle_op(op=op, sss=sss)

            monkeypatch.setattr(coh, "handle_op", spy)
            tss = TablesideSimulator(
                circuit=circuit,
                compiled_op_handler=coh,
                batch_size=batch_size,
                seed=5,
                use_cpp_kernels=True,
            )
            tss.run()
            assert np.all(tss.get_final_measurement_records() == expected), batch_size
        assert (len(handled) == 2) if to_python else (handled == [])