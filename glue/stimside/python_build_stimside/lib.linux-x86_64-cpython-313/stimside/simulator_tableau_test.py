import numpy as np
import pytest
import stim # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler
from stimside.simulator_tableau import TablesideSimulator


class MockCompiledOpHandler(CompiledOpHandler[TablesideSimulator]):
    def claim_ops(self) -> set[str]:
        return {"MOCK"}

    def handle_op(self, op: stim.CircuitInstruction, sss: "TablesideSimulator") -> None:
        if op in self.claim_ops():
            raise NotImplementedError(f"mock handle_op called and somehow matched")
        else:
            sss._do_bare_instruction(op)
            return

    def clear(self) -> None:
        raise NotImplementedError(f"mock clear called")


class TestFlipsideSimulator:

    batch_size = 128
    coh = MockCompiledOpHandler()

    def make_simulator(self, circuit):
        return TablesideSimulator(
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
        tss = self.make_simulator(circuit)
        tss.interactive_do(circuit[0])  # QUBIT_COORDS(0,0) 0
        tss.interactive_do(circuit[1])  # QUBIT_COORDS(1,0) 1
        assert tss.qubit_coords == {0: [0, 0], 1: [1, 0]}
        tss.interactive_do(circuit[2])  # SHIFT_COORDS(5, 5)
        assert tss.qubit_coords == {0: [0, 0], 1: [1, 0]}
        assert tss.coords_shifts == [5, 5]
        tss.interactive_do(circuit[3])  # QUBIT_COORDS(0) 5
        tss.interactive_do(circuit[4])  # QUBIT_COORDS(1, 0) 6
        assert tss.qubit_coords == {0: [0, 0], 1: [1, 0], 5: [5], 6: [6, 5]}
        tss.interactive_do(circuit[5])  # SHIFT_COORDS(5)
        assert tss.coords_shifts == [10, 5]
        tss.interactive_do(circuit[6])  # SHIFT_COORDS(5,5,5)
        assert tss.coords_shifts == [15, 10, 5]
        tss.interactive_do(circuit[7])  # QUBIT_COORDS[QUBIT_TAG](0, 0)
        assert tss.qubit_coords == {0: [0, 0], 1: [1, 0], 5: [5], 6: [6, 5], 10: [15, 10]}

    def test_run_and_clear(self):
        circuit = stim.Circuit(
            """
            R 0 1 2 3
        """
        )
        tss = self.make_simulator(circuit)

        tss.run()

        with pytest.raises(ValueError, match=r"prepare the simulator for a new run"):
            tss.run()

        with pytest.raises(NotImplementedError, match="mock clear called"):
            tss.clear()

        assert tss.qubit_coords == {}
        assert tss.qubit_tags == {}
        assert tss.coords_shifts == []

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

        tss = self.make_simulator(circuit)
        tss.run()

        assert tss.batch_size == self.batch_size
        assert tss.num_qubits == circuit_num_qubits

        assert tss.get_detector_flips().shape == (self.batch_size, circuit_num_detectors)
        assert tss.get_final_measurement_records().shape == (self.batch_size, circuit_num_measurements)
        assert tss.get_observable_flips().shape == (self.batch_size, circuit_num_observables)

    def test_batch_size_1_detector_and_observable_flips(self):
        circuit = stim.Circuit(
            """
            X_ERROR(1.0) 0 4
            M 0 1 2 3 4 5 6 7
            DETECTOR(0) rec[-8]
            DETECTOR(1) rec[-7]
            OBSERVABLE_INCLUDE(0) rec[-4] rec[-3]
        """
        )
        from stimside.op_handlers.abstract_op_handler import _TrivialOpHandler

        coh = _TrivialOpHandler().compile_op_handler(circuit=circuit, batch_size=1)
        tss = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=coh,
            batch_size=1,
            seed=42,
            construct_reference_circuit=True,
        )
        tss.run()
        det_flips = tss.get_detector_flips()
        obs_flips = tss.get_observable_flips()
        assert det_flips.shape == (1, 2)
        assert obs_flips.shape == (1, 1)
        assert det_flips[0, 0] == True
        assert det_flips[0, 1] == False
        assert obs_flips[0, 0] == True

        tss.clear()
        assert tss._detector_flips is None
        assert tss._observable_flips is None
        assert tss._finished_running_circuit is False
        assert len(tss._new_reference_circuit) == 0

    def test_deterministic_seeding_and_reference_coords(self):
        circuit = stim.Circuit(
            """
            QUBIT_COORDS(1, 2) 0
            SHIFT_COORDS(3, 4)
            H 0
            M 0
            DETECTOR(0) rec[-1]
        """
        )
        from stimside.op_handlers.abstract_op_handler import _TrivialOpHandler

        coh1 = _TrivialOpHandler().compile_op_handler(circuit=circuit, batch_size=1)
        coh2 = _TrivialOpHandler().compile_op_handler(circuit=circuit, batch_size=1)
        tss1 = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=coh1,
            batch_size=1,
            seed=999,
            construct_reference_circuit=True,
        )
        tss2 = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=coh2,
            batch_size=1,
            seed=999,
            construct_reference_circuit=True,
        )
        outs1, outs2 = [], []
        for _ in range(10):
            tss1.clear()
            tss1.run()
            outs1.append(bool(tss1.get_detector_flips()[0, 0]))

            tss2.clear()
            tss2.run()
            outs2.append(bool(tss2.get_detector_flips()[0, 0]))

        assert outs1 == outs2
        ref_names = [inst.name for inst in tss1._new_reference_circuit]
        assert "QUBIT_COORDS" in ref_names
        assert "SHIFT_COORDS" in ref_names

    def test_cpp_vs_python_kernels_mode_a_and_mode_b_up_to_d15(self):
        from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
            LeakageUint8,
        )

        for d, mode_b in [(5, False), (5, True), (15, False), (15, True)]:
            base = stim.Circuit.generated(
                "surface_code:rotated_memory_z",
                distance=d,
                rounds=d,
                after_clifford_depolarization=0.001,
                before_measure_flip_probability=0.001,
                after_reset_flip_probability=0.001,
            ).flattened()
            mr_qubits = {
                t.value
                for inst in base
                if inst.name == "MR"
                for t in inst.targets_copy()
            }
            data_qubits = sorted(set(range(base.num_qubits)) - mr_qubits)
            circ = stim.Circuit()
            for inst in base:
                if inst.name == "CX":
                    targets = inst.targets_copy()
                    t_qs = [targets[i] for i in range(1, len(targets), 2)]
                    circ.append("H", t_qs)
                    circ.append(
                        "CZ", targets, [], tag="CONDITIONED_ON_PAIR: (U, U)"
                    )
                    circ.append(
                        "II_ERROR",
                        targets,
                        [0.0],
                        tag="LEAKAGE_TRANSITION_2: (0.002, U_U-->2_D)",
                    )
                    circ.append("H", t_qs)
                elif inst.name == "MR":
                    targets = inst.targets_copy()
                    if mode_b:
                        circ.append(
                            "I",
                            targets,
                            [],
                            tag="LEAKAGE_TRANSITION_1: (0.01, 1-->2)",
                        )
                    circ.append(
                        "M",
                        targets,
                        inst.gate_args_copy(),
                        tag="LEAKAGE_PROJECTION_Z: (1.0, 2)",
                    )
                    circ.append("R", targets)
                    circ.append(
                        "I",
                        targets,
                        [],
                        tag="LEAKAGE_TRANSITION_1: (0.5, 2-->0) (0.5, 2-->U)",
                    )
                    circ.append(
                        "I",
                        data_qubits,
                        [],
                        tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)",
                    )
                elif inst.name == "M":
                    circ.append(
                        "M",
                        inst.targets_copy(),
                        inst.gate_args_copy(),
                        tag="LEAKAGE_PROJECTION_Z: (1.0, 2)",
                    )
                else:
                    circ.append(inst)

            for use_cpp in (True, False):
                coh = LeakageUint8(
                    unconditional_condition_on_U=True
                ).compile_op_handler(circuit=circ, batch_size=1)
                tss = TablesideSimulator(
                    circuit=circ,
                    compiled_op_handler=coh,
                    batch_size=1,
                    seed=1234,
                    use_cpp_kernels=use_cpp,
                    construct_reference_circuit=True,
                )
                assert tss._running_tableau is mode_b
                tss.run()
                recs = tss.get_final_measurement_records()
                dets = tss.get_detector_flips()
                obs = tss.get_observable_flips()
                assert recs.shape == (1, circ.num_measurements)
                assert dets.shape == (1, circ.num_detectors)
                assert obs.shape == (1, circ.num_observables)
                assert len(tss._new_circuit) > 0
                assert len(coh.state) == circ.num_qubits


def test_concurrent_simulators_and_fused_mode_b_new_circuit():
    from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
        LeakageUint8,
    )

    circ = stim.Circuit(
        """
        X 0
        I[LEAKAGE_TRANSITION_1: (1.0, 1-->2)] 0 1
        M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0 1
        DETECTOR rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-1]
        """
    )
    coh1 = LeakageUint8(unconditional_condition_on_U=True).compile_op_handler(
        circuit=circ, batch_size=1
    )
    coh2 = LeakageUint8(unconditional_condition_on_U=True).compile_op_handler(
        circuit=circ, batch_size=1
    )
    sim1 = TablesideSimulator(
        circuit=circ, compiled_op_handler=coh1, batch_size=8, seed=101, use_cpp_kernels=True
    )
    sim2 = TablesideSimulator(
        circuit=circ, compiled_op_handler=coh2, batch_size=8, seed=202, use_cpp_kernels=True
    )
    # Concurrent instances on the same circuit must not share state buffers
    assert coh1.state is not coh2.state
    sim1.run()
    assert list(coh1.state) == [2, 0]
    assert list(coh2.state) == [0, 0]
    sim2.run()
    assert list(coh1.state) == [2, 0]
    assert list(coh2.state) == [2, 0]

    # Verify fused Mode B injects R and X_ERROR(1) BEFORE M in _new_circuit so all batch_size=8 shots measure [1, 0]
    for use_cpp in (True, False):
        coh = LeakageUint8(unconditional_condition_on_U=True).compile_op_handler(
            circuit=circ, batch_size=1
        )
        sim = TablesideSimulator(
            circuit=circ,
            compiled_op_handler=coh,
            batch_size=8,
            seed=303,
            use_cpp_kernels=use_cpp,
        )
        sim.run()
        recs = sim.get_final_measurement_records()
        assert recs.shape == (8, 2)
        assert np.all(recs[:, 0] == True)
        assert np.all(recs[:, 1] == False)
        assert str(sim.peek_bloch(1)) == "+Z"


def test_exotic_circuit_end_to_end_tableside_vs_cosetside_cross_validation():
    from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import (
        LeakageUint8Coset,
    )
    from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
        LeakageUint8 as LeakageUint8Tableau,
    )
    from stimside.simulator_coset import CosetsideSimulator

    circ = stim.Circuit(
        """
        R 0 1 2 3
        REPEAT 2 {
            X 0
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 1 2 3
            II_ERROR[LEAKAGE_TRANSITION_2: (0.25, U_U-->2_3) (0.25, 2_3-->U_Y)] 0 1
            I[LEAKAGE_TRANSITION_1: (0.3, 1-->2) (0.4, 2-->U) (0.4, 3-->0)] 0 1 2 3
            PAULI_CHANNEL_1(0.05, 0.05, 0.05) 0 1 2 3
            PAULI_CHANNEL_2(0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01) 0 1 2 3
            CORRELATED_ERROR(0.2) X0 Z1
            ELSE_CORRELATED_ERROR(0.3) Y2 X3
            MPAD[LEAKAGE_MEASUREMENT: (0.0, 0) (0.0, 1) (1.0, 2) (1.0, 3) : 0 1] 0 0
            CX rec[-2] 2 rec[-1] 3
            M[LEAKAGE_PROJECTION_Z: (1.0, 2) (1.0, 3)] 0 1 2 3
            SHIFT_COORDS(0, 1)
            DETECTOR(0, 0) rec[-4] rec[-3]
        }
        OBSERVABLE_INCLUDE(0) rec[-2] rec[-1]
        """
    )

    for seed in (10, 20, 30):
        for use_cpp in (True, False):
            coh_t = LeakageUint8Tableau(
                unconditional_condition_on_U=True
            ).compile_op_handler(circuit=circ, batch_size=1)
            tss = TablesideSimulator(
                circuit=circ,
                compiled_op_handler=coh_t,
                batch_size=1,
                seed=seed,
                use_cpp_kernels=use_cpp,
            )
            tss.run()
            assert tss.get_final_measurement_records().shape == (1, circ.num_measurements)
            assert tss.get_detector_flips().shape == (1, circ.num_detectors)
            assert tss.get_observable_flips().shape == (1, circ.num_observables)

        coh_t_sync = LeakageUint8Tableau(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=circ, batch_size=1)
        tss_sync = TablesideSimulator(
            circuit=circ,
            compiled_op_handler=coh_t_sync,
            batch_size=1,
            seed=seed,
            sync_tableside_rng=True,
        )
        tss_sync.run()

        coh_c_sync = LeakageUint8Coset(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=circ, batch_size=1)
        css_sync = CosetsideSimulator(
            circuit=circ,
            compiled_op_handler=coh_c_sync,
            batch_size=1,
            seed=seed,
            sync_tableside_rng=True,
        )
        css_sync.run()

        np.testing.assert_array_equal(
            tss_sync.get_final_measurement_records(),
            css_sync.get_final_measurement_records(),
        )
        np.testing.assert_array_equal(
            tss_sync.get_detector_flips(),
            css_sync.get_detector_flips(),
        )
        np.testing.assert_array_equal(
            tss_sync.get_observable_flips(),
            css_sync.get_observable_flips(),
        )
        np.testing.assert_array_equal(
            coh_t_sync.state.reshape(-1),
            coh_c_sync.state.reshape(-1),
        )



