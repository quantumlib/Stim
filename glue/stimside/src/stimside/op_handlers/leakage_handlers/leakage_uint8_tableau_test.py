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