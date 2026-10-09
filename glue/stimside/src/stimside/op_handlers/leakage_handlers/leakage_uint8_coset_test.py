import dataclasses
import time

import numpy as np
import pytest
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import (
    CompiledLeakageUint8Coset,
    LeakageUint8Coset,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
    LeakageUint8 as LeakageUint8Tableau,
)
from stimside.dem_generators.leakage_decoder import BaseDecoder
from stimside.sampler_coset import CosetsideSampler
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_tableau import TablesideSimulator


class TestCompiledLeakageUint8Coset:
    """Test suite porting all tests from leakage_uint8_tableau_test.py plus batched coset tests."""

    op_handler_batch_size = 1
    simulator_batch_size = 64

    def _get_simulator_for_circuit(
        self,
        circuit: stim.Circuit,
        unconditional_condition_on_U: bool = True,
        op_batch_size: int | None = None,
        sim_batch_size: int | None = None,
    ) -> CosetsideSimulator:
        obs = (
            self.op_handler_batch_size
            if op_batch_size is None
            else op_batch_size
        )
        sbs = (
            self.simulator_batch_size
            if sim_batch_size is None
            else sim_batch_size
        )
        cti = LeakageUint8Coset(
            unconditional_condition_on_U
        ).compile_op_handler(circuit=circuit, batch_size=obs)
        return CosetsideSimulator(
            circuit=circuit,
            batch_size=sbs,
            compiled_op_handler=cti,
            seed=42,
        )

    def test_clear(self) -> None:
        circuit = stim.Circuit("R 0")
        tss = self._get_simulator_for_circuit(circuit)

        tss._compiled_op_handler.state[:] = 5
        assert np.any(tss._compiled_op_handler.state != 0)

        tss._compiled_op_handler.clear()
        assert np.all(tss._compiled_op_handler.state == 0)

    def test_make_target_mask(self) -> None:
        circuit = stim.Circuit("CX 0 1 2 3")
        op = circuit[0]

        cti: CompiledLeakageUint8Coset = LeakageUint8Coset().compile_op_handler(
            circuit=circuit, batch_size=1
        )
        mask = cti.make_target_mask(op)
        expected_mask = np.zeros(circuit.num_qubits, dtype=bool)
        expected_mask[[0, 1, 2, 3]] = True
        assert np.array_equal(mask, expected_mask)

        cti_batched = LeakageUint8Coset().compile_op_handler(
            circuit=circuit, batch_size=16
        )
        mask_2d = cti_batched.make_target_mask(op)
        assert mask_2d.shape == (4, 16)
        assert np.all(mask_2d)

    def test_handle_op(self) -> None:
        @dataclasses.dataclass
        class FakeLeakageParams:
            name: str = "FAKE_LEAKAGE"
            from_tag: str = "FAKE"

        circuit = stim.Circuit("I 0")
        op = circuit[0]
        tss = self._get_simulator_for_circuit(circuit)

        tss._compiled_op_handler.ops_to_params[op] = FakeLeakageParams()  # type: ignore[assignment]
        tss._compiled_op_handler.claimed_ops_keys = set(
            tss._compiled_op_handler.ops_to_params.keys()
        )

        with pytest.raises(ValueError, match="Unrecognised LEAKAGE params"):
            tss._do_instruction(op)

    @pytest.mark.parametrize("op_batch", [1, 64])
    def test_leakage_transition_1(self, op_batch: int) -> None:
        # Part a: Computational to Leaked
        circuit_a = stim.Circuit(
            """
            R 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 1
        """
        )
        tss_a = self._get_simulator_for_circuit(
            circuit_a, op_batch_size=op_batch
        )
        tss_a.interactive_do(circuit_a[0])
        assert np.all(tss_a._compiled_op_handler.state[:] == 0)
        tss_a.interactive_do(circuit_a[1])
        assert np.all(tss_a._compiled_op_handler.state[:] == 2)

        # Part b: Leaked to Leaked
        circuit_b = stim.Circuit("I[LEAKAGE_TRANSITION_1: (1.0, 2-->3)] 0 1")
        tss_b = self._get_simulator_for_circuit(
            circuit_b, op_batch_size=op_batch
        )
        tss_b._compiled_op_handler.state[:] = 2
        tss_b.interactive_do(circuit_b[0])
        assert np.all(tss_b._compiled_op_handler.state[:] == 3)

        # Part c: Leaked to Computational (Z basis and X basis)
        circuit_c = stim.Circuit(
            """
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 1
            M 0 1
            """
        )
        tss_c = self._get_simulator_for_circuit(
            circuit_c, op_batch_size=op_batch
        )
        tss_c._compiled_op_handler.state[:] = 2
        tss_c.run()
        records = tss_c.get_final_measurement_records()
        assert np.all(tss_c._compiled_op_handler.state[:] == 0)
        assert np.any(records[:, :] == 0)
        assert np.any(records[:, :] == 1)

        circuit_cx = stim.Circuit(
            """
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 1
            MX 0 1
            """
        )
        tss_cx = self._get_simulator_for_circuit(
            circuit_cx, op_batch_size=op_batch
        )
        tss_cx._compiled_op_handler.state[:] = 2
        tss_cx.run()
        records_x = tss_cx.get_final_measurement_records()
        assert np.all(tss_cx._compiled_op_handler.state[:] == 0)
        assert np.any(records_x[:, :] == 0)
        assert np.any(records_x[:, :] == 1)

    @pytest.mark.parametrize("op_batch", [1, 64])
    def test_leakage_transition_2(self, op_batch: int) -> None:
        # Test case: (U, U) -> (L2, L3)
        circuit = stim.Circuit(
            """
            R 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_3)] 0 1
        """
        )
        tss = self._get_simulator_for_circuit(circuit, op_batch_size=op_batch)
        tss.run()
        assert np.all(tss._compiled_op_handler.state[0] == 2)
        assert np.all(tss._compiled_op_handler.state[1] == 3)

        # Test case: (L2, U) -> (L4, V)
        circuit = stim.Circuit(
            """
            R 0 1
            H 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->4_V)] 0 1
            MX 0 1
        """
        )
        tss = self._get_simulator_for_circuit(circuit, op_batch_size=op_batch)
        tss._compiled_op_handler.state[0] = 2
        tss.run()
        assert np.all(tss._compiled_op_handler.state[0] == 4)
        assert np.all(tss._compiled_op_handler.state[1] == 0)
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
        tss = self._get_simulator_for_circuit(circuit, op_batch_size=op_batch)
        tss.interactive_do(circuit[0])
        tss.interactive_do(circuit[1])
        tss.interactive_do(circuit[2])
        assert np.all(tss._compiled_op_handler.state[0] == 2)
        tss.interactive_do(circuit[3])
        assert np.all(tss.peek_z(0) == -1)
        assert np.all(tss._compiled_op_handler.state[1] == 0)

        circuit.append_from_stim_program_text("MX 0 1")
        tss.interactive_do(circuit[4])
        tss.finish_interactive_run()
        records = tss.get_final_measurement_records()
        assert np.all(records[:, 1] == 0)
        assert np.any(records[:, 0] == 0)
        assert np.any(records[:, 0] == 1)

    @pytest.mark.parametrize("op_batch", [1, 64])
    def test_leakage_projection_Z(self, op_batch: int) -> None:
        circuit = stim.Circuit(
            """
            X 1
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->2)] 2
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->3)] 3
            M[LEAKAGE_PROJECTION_Z: (1.0, 0) (0.0, 1) (1.0, 2) (0.5, 3)] 0 1 2 3
        """
        )
        tss = self._get_simulator_for_circuit(circuit, op_batch_size=op_batch)
        tss.interactive_do(circuit[0])
        tss.interactive_do(circuit[1])
        tss.interactive_do(circuit[2])

        assert tss.peek_z(0) == 1
        assert tss.peek_z(1) == -1
        assert tss._compiled_op_handler.state[2] == 2
        assert tss._compiled_op_handler.state[3] == 3

        tss.interactive_do(circuit[3])
        tss.finish_interactive_run()
        records = tss.get_final_measurement_records()

        assert np.all(records[:, 0] == 1)
        assert np.all(records[:, 1] == 0)
        assert np.all(records[:, 2] == 1)
        assert not np.all(records[:, 3] == 0)
        assert not np.all(records[:, 3] == 1)
        assert np.all(
            tss._compiled_op_handler.state == np.array([0, 0, 2, 3])
        )

    @pytest.mark.parametrize("op_batch", [1, 64])
    def test_leakage_measurement(self, op_batch: int) -> None:
        circuit = stim.Circuit(
            """
            X 1
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->2)] 2
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->3)] 3
            MPAD[LEAKAGE_MEASUREMENT: (1.0, 0) (0.0, 1) (1.0, 2) (0.5, 3): 0 1 2 3] 0 0 0 0
        """
        )
        tss = self._get_simulator_for_circuit(circuit, op_batch_size=op_batch)
        tss.interactive_do(circuit[0])
        tss.interactive_do(circuit[1])
        tss.interactive_do(circuit[2])

        assert tss.peek_z(0) == 1
        assert tss.peek_z(1) == -1
        assert tss._compiled_op_handler.state[2] == 2
        assert tss._compiled_op_handler.state[3] == 3

        tss.interactive_do(circuit[3])
        tss.finish_interactive_run()
        records = tss.get_final_measurement_records()

        assert np.all(records[:, 0] == 1)
        assert np.all(records[:, 1] == 0)
        assert np.all(records[:, 2] == 1)
        assert not np.all(records[:, 3] == 0)
        assert not np.all(records[:, 3] == 1)
        assert np.all(
            tss._compiled_op_handler.state == np.array([0, 0, 2, 3])
        )

    @pytest.mark.parametrize("op_batch", [1, 64])
    def test_conditional_ops(self, op_batch: int) -> None:
        # Part a: Universal conditioning on U suppresses CZ 0 1 and H 0 when qubit 0 is leaked
        circuit_a = stim.Circuit(
            """
            H 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            CZ 0 1
            H 0 1
            M 1
        """
        )
        tss_a = self._get_simulator_for_circuit(
            circuit_a, unconditional_condition_on_U=True, op_batch_size=op_batch
        )
        tss_a.run()
        records = tss_a.get_final_measurement_records()
        assert tss_a._compiled_op_handler.state[0] == 2
        assert np.all(records[:, 0] == 0)

        # Turning universal conditioning on U off allows CZ 0 1 to entangle 0 and 1
        tss_a_off = self._get_simulator_for_circuit(
            circuit_a,
            unconditional_condition_on_U=False,
            op_batch_size=op_batch,
        )
        tss_a_off.run()
        records_off = tss_a_off.get_final_measurement_records()
        assert tss_a_off._compiled_op_handler.state[0] == 2
        assert np.any(records_off[:, 0] == 0)
        assert np.any(records_off[:, 0] == 1)

        # Part b: Explicit CONDITIONED_ON_PAIR: (U, U) suppresses CZ 0 1 via inverse Clifford cocycle E = CZ^dagger
        circuit_b = stim.Circuit(
            """
            H 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 1
            H 0 1
            M 1
        """
        )
        tss_b = self._get_simulator_for_circuit(
            circuit_b,
            unconditional_condition_on_U=False,
            op_batch_size=op_batch,
        )
        tss_b.run()
        records_b = tss_b.get_final_measurement_records()
        assert tss_b._compiled_op_handler.state[0] == 2
        assert np.all(records_b[:, 0] == 0)

        # Part c: CONDITIONED_ON_OTHER
        circuit_c = stim.Circuit(
            """
            R 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 2
            X[CONDITIONED_ON_OTHER: U: 0 2] 1 3
            M 1 3
        """
        )
        tss_c = self._get_simulator_for_circuit(
            circuit_c,
            unconditional_condition_on_U=False,
            op_batch_size=op_batch,
        )
        tss_c.run()
        records_c = tss_c.get_final_measurement_records()
        assert tss_c._compiled_op_handler.state[0] == 2
        assert tss_c._compiled_op_handler.state[2] == 2
        assert np.all(records_c[:, :] == 0)

    def test_surface_code_statistical_equivalence_and_speedup(self) -> None:
        """Compare detector and observable flip statistics of CosetsideSimulator vs TablesideSimulator on a surface code circuit with leakage."""
        raw_circuit = stim.Circuit.generated(
            code_task="surface_code:rotated_memory_x",
            distance=3,
            rounds=2,
            after_clifford_depolarization=2e-3,
            before_round_data_depolarization=2e-3,
            before_measure_flip_probability=2e-3,
            after_reset_flip_probability=2e-3,
        )

        p = 5e-3

        def _mod_circuit(circ: stim.Circuit) -> stim.Circuit:
            out = stim.Circuit()
            for op in circ:
                if isinstance(op, stim.CircuitInstruction):
                    if op.name == "CX":
                        out.append(
                            stim.CircuitInstruction(
                                "H", op.targets_copy()[1::2]
                            )
                        )
                        out.append(
                            stim.CircuitInstruction(
                                "CZ",
                                op.targets_copy(),
                                tag="CONDITIONED_ON_PAIR: (U, U)",
                            )
                        )
                        out.append(
                            stim.CircuitInstruction(
                                "II_ERROR",
                                op.targets_copy(),
                                tag=(
                                    f"LEAKAGE_TRANSITION_2: ({p/6}, U_U-->U_2) "
                                    f"({p/6}, U_U-->2_U) ({p/6}, U_U-->2_2) "
                                    f"({p/2}, 2_U-->2_2) ({p/2}, U_2-->2_2)"
                                ),
                            )
                        )
                        out.append(
                            stim.CircuitInstruction(
                                "H", op.targets_copy()[1::2]
                            )
                        )
                    elif stim.gate_data(op.name).produces_measurements:
                        if op.name == "MX":
                            out.append(
                                stim.CircuitInstruction("H", op.targets_copy())
                            )
                            out.append(
                                stim.CircuitInstruction(
                                    "I_ERROR",
                                    op.targets_copy(),
                                    tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)",
                                )
                            )
                            out.append(
                                stim.CircuitInstruction("M", op.targets_copy())
                            )
                        elif op.name in ("MR", "M"):
                            out.append(
                                stim.CircuitInstruction(
                                    "I_ERROR",
                                    op.targets_copy(),
                                    tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)",
                                )
                            )
                            out.append(op)
                        else:
                            out.append(op)
                    else:
                        out.append(op)
                elif isinstance(op, stim.CircuitRepeatBlock):
                    body = op.body_copy()
                    for _ in range(op.repeat_count):
                        out.append(_mod_circuit(body))
            return out

        mod_circ = _mod_circuit(raw_circuit)
        num_shots = 256

        # Run CosetsideSimulator in a single batch of 256 shots
        op_coset = LeakageUint8Coset(
            unconditional_condition_on_U=False
        ).compile_op_handler(circuit=mod_circ, batch_size=num_shots)
        coset_sim = CosetsideSimulator(
            circuit=mod_circ,
            compiled_op_handler=op_coset,
            batch_size=num_shots,
            seed=1234,
        )
        t0 = time.perf_counter()
        coset_sim.run()
        coset_dets = coset_sim.get_detector_flips()
        coset_obs = coset_sim.get_observable_flips()
        coset_time = time.perf_counter() - t0

        # Run TablesideSimulator for 64 independent shots (1 shot per run)
        tab_shots = 64
        op_tab = LeakageUint8Tableau(
            unconditional_condition_on_U=False
        ).compile_op_handler(circuit=mod_circ, batch_size=1)
        tab_sim = TablesideSimulator(
            circuit=mod_circ,
            compiled_op_handler=op_tab,
            batch_size=1,
            seed=1234,
        )
        tab_dets_list = []
        tab_obs_list = []
        t1 = time.perf_counter()
        for _ in range(tab_shots):
            tab_sim.clear()
            tab_sim._detector_flips = None
            tab_sim._observable_flips = None
            tab_sim._finished_running_circuit = False
            tab_sim.run()
            tab_dets_list.append(tab_sim.get_detector_flips()[0])
            tab_obs_list.append(tab_sim.get_observable_flips()[0])
        tab_time = (time.perf_counter() - t1) * (num_shots / tab_shots)

        tab_dets = np.array(tab_dets_list)
        assert coset_dets.shape == (num_shots, mod_circ.num_detectors)
        assert coset_obs.shape == (num_shots, mod_circ.num_observables)

        # Verify detector flip rates agree within statistical uncertainty
        coset_det_rate = float(np.mean(coset_dets))
        tab_det_rate = float(np.mean(tab_dets))
        assert coset_det_rate > 0.01
        assert tab_det_rate > 0.01
        assert abs(coset_det_rate - tab_det_rate) < 0.025
        assert coset_time < tab_time

    def test_sinter_sampler_coset(self) -> None:
        circuit = stim.Circuit(
            """
            R 0 1 2
            X_ERROR(0.01) 0 1
            H 2
            I_ERROR[LEAKAGE_TRANSITION_1: (0.02, U-->2)] 0
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 2
            CZ[CONDITIONED_ON_PAIR: (U, U)] 1 2
            H 2
            M 0 1 2
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-2]
        """
        )
        task = sinter.Task(
            circuit=circuit,
            detector_error_model=circuit.detector_error_model(),
        )
        sampler = CosetsideSampler(
            op_handler=LeakageUint8Coset(unconditional_condition_on_U=False),
            dem_decoder=BaseDecoder(),
            batch_size=64,
        )
        compiled_sampler = sampler.compiled_sampler_for_task(task)
        stats = compiled_sampler.sample(suggested_shots=128)
        assert stats.shots == 128

    def test_coset_rank_drops_and_self_healing(self) -> None:
        """Verify Theorem 2.5 (Phase 3), Theorem 2.7 (Phase 4 Case A / B-I / B-II), and Theorem 2.8 (Self-Healing)."""
        # Skipped CZ on |+>|+> creates r_0 = 2^2 = 4 orthogonal frames (Remark 2.3), followed by measurements that self-heal r_b -> 1
        circuit = stim.Circuit(
            """
            R 0 1 2
            H 0 1 2
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 1
            TICK
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 2
            H 1 2
            M 1 2
            R 0 1 2
            M 0 1 2
        """
        )
        sim = self._get_simulator_for_circuit(
            circuit,
            unconditional_condition_on_U=False,
            op_batch_size=16,
            sim_batch_size=16,
        )
        # Step through instructions to verify active_shots creation and self-healing eviction:
        sim.interactive_do(circuit[0])  # R 0 1 2
        sim.interactive_do(circuit[1])  # H 0 1 2
        sim.interactive_do(circuit[2])  # leak 0 -> 2
        assert len(sim.active_shots) == 0
        sim.interactive_do(circuit[3])  # skipped CZ 0 1 -> injects E = CZ^dag (d_0 = 2, r_0 = 4)
        assert len(sim.active_shots) == 16
        for coset in sim.active_shots.values():
            assert coset.d == 2
            assert coset.num_frames == 4
            coset.check_invariants()
        sim.interactive_do(circuit[4])  # TICK
        sim.interactive_do(circuit[5])  # skipped CZ 0 2
        sim.interactive_do(circuit[6])  # H 1 2
        sim.interactive_do(circuit[7])  # M 1 2 -> collapses cosets on qubits 1, 2
        sim.interactive_do(circuit[8])  # R 0 1 2 -> self-heals all remaining cosets to r_b = 1
        assert len(sim.active_shots) == 0
        sim.interactive_do(circuit[9])  # M 0 1 2
        sim.finish_interactive_run()
        records = sim.get_final_measurement_records()
        assert records.shape == (16, 5)
        assert np.all(records == 0)

    def test_overlapping_conditional_clifford_target_groups(self) -> None:
        """Verify multi-target instructions with overlapping target groups (e.g. CX 0 1 1 2) match TablesideSimulator."""
        circuit = stim.Circuit(
            """
            X 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            CX 0 1 1 2
            M 1 2
        """
        )
        op_tab = LeakageUint8Tableau(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=circuit, batch_size=1)
        tab_sim = TablesideSimulator(
            circuit=circuit, compiled_op_handler=op_tab, batch_size=1, seed=1
        )
        tab_sim.run()

        cos_sim = self._get_simulator_for_circuit(
            circuit,
            unconditional_condition_on_U=True,
            op_batch_size=16,
            sim_batch_size=16,
        )
        cos_sim.run()
        assert np.all(
            cos_sim.get_final_measurement_records()
            == tab_sim.get_final_measurement_records()[0]
        )

    def test_entangled_bell_reset_transitions(self) -> None:
        """Verify simultaneous unmatched resets (2-->0 and 2-->1) on entangled Bell pairs."""
        circuit = stim.Circuit(
            """
            H 0 2
            CX 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->0)] 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->1)] 2 3
            M 0 1 2 3
        """
        )
        cos_sim = self._get_simulator_for_circuit(
            circuit,
            unconditional_condition_on_U=True,
            op_batch_size=64,
            sim_batch_size=64,
        )
        cos_sim.run()
        recs = cos_sim.get_final_measurement_records()
        assert np.all(recs[:, 0] == 0)
        assert np.all(recs[:, 1] == 0)
        assert np.all(recs[:, 2] == 1)
        assert np.all(recs[:, 3] == 1)

    def test_leakage_transition_2_non_ascending_pairs_and_tuple_keys(self) -> None:
        """Verify correlated pair transitions on non-ascending target pairs and computational tuple keys (0_1-->2_3)."""
        c_pairs = stim.Circuit(
            """
            R 0 1 2 3
            II_ERROR[LEAKAGE_TRANSITION_2: (0.5, U_U-->2_3)] 2 1 0 3
        """
        )
        sim_pairs = CosetsideSimulator(
            circuit=c_pairs,
            compiled_op_handler=LeakageUint8Coset(
                unconditional_condition_on_U=False
            ).compile_op_handler(circuit=c_pairs, batch_size=64),
            seed=None,
            batch_size=64,
        )
        sim_pairs.run()
        st = sim_pairs._compiled_op_handler.state
        for b in range(64):
            assert (int(st[2, b]), int(st[1, b])) in ((0, 0), (2, 3))
            assert (int(st[0, b]), int(st[3, b])) in ((0, 0), (2, 3))

        # Also verify exact match with TablesideSimulator when seeded
        sim_seeded = CosetsideSimulator(
            circuit=c_pairs,
            compiled_op_handler=LeakageUint8Coset(
                unconditional_condition_on_U=False
            ).compile_op_handler(circuit=c_pairs, batch_size=1),
            seed=42,
            batch_size=1,
        )
        sim_seeded.run()
        tab_seeded = TablesideSimulator(
            circuit=c_pairs,
            compiled_op_handler=LeakageUint8Tableau(
                unconditional_condition_on_U=False
            ).compile_op_handler(circuit=c_pairs, batch_size=1),
            seed=42,
            batch_size=1,
            sync_tableside_rng=True,
        )
        tab_seeded.run()
        np.testing.assert_array_equal(
            sim_seeded._compiled_op_handler.state,
            tab_seeded._compiled_op_handler.state,
        )

        c_tuple = stim.Circuit(
            """
            R 0 1
            X 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 0_1-->2_3)] 0 1
        """
        )
        for bs in (1, 8):
            sim_tuple = self._get_simulator_for_circuit(
                c_tuple,
                unconditional_condition_on_U=False,
                op_batch_size=bs,
                sim_batch_size=bs,
            )
            sim_tuple.run()
            assert np.all(
                sim_tuple._compiled_op_handler.state == np.array([2, 3])
            )

        # Verify non-ascending pair targets (3 2 1 0) with asymmetric output states (0_1) in batched mode
        c_rev = stim.Circuit(
            """
            R 0 1 2 3
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->0_1)] 3 2 1 0
            M 0 1 2 3
        """
        )
        sim_rev = self._get_simulator_for_circuit(
            c_rev,
            unconditional_condition_on_U=False,
            op_batch_size=8,
            sim_batch_size=8,
        )
        sim_rev.run()
        recs_rev = sim_rev.get_final_measurement_records()
        assert np.all(recs_rev[:, 0] == 1)
        assert np.all(recs_rev[:, 1] == 0)
        assert np.all(recs_rev[:, 2] == 1)
        assert np.all(recs_rev[:, 3] == 0)

    def test_inverted_targets_and_multi_pauli_measurements(self) -> None:
        """Verify M !q, MZZ, MXX, MYY, and MPP instructions with and without active cosets."""
        circuit = stim.Circuit(
            """
            R 0 1 2 3
            H 0 2
            CX 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 2
            CZ[CONDITIONED_ON_PAIR: (U, U)] 2 3
            MZZ 0 1
            MXX 0 1
            MYY 0 1
            MPP !Z0*Z1 !X0*X1 Y0*Y1
            M !0 !1
        """
        )
        sim = self._get_simulator_for_circuit(
            circuit,
            unconditional_condition_on_U=False,
            op_batch_size=32,
            sim_batch_size=32,
        )
        sim.run()
        recs = sim.get_final_measurement_records()
        assert recs.shape == (32, 8)
        assert np.all(recs[:, 0] == 0)  # Z0*Z1 == +1
        assert np.all(recs[:, 1] == 0)  # X0*X1 == +1
        assert np.all(recs[:, 2] == 1)  # Y0*Y1 == -1
        assert np.all(recs[:, 3] == 1)  # !Z0*Z1
        assert np.all(recs[:, 4] == 1)  # !X0*X1
        assert np.all(recs[:, 5] == 1)  # Y0*Y1 == -1
        assert np.all(recs[:, 6] == recs[:, 7])

    def test_high_rank_cosets_16_and_64_frames(self) -> None:
        """Verify multiple simultaneous uncollapsed skipped 2-qubit Clifford gates driving r_b(t) to 16 and 64."""
        circuit = stim.Circuit(
            """
            R 0 1 2 3 4 5
            H 0 1 2 3 4 5
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 2 4
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 1 2 3 4 5
            H 0 1 2 3 4 5
            M 0 1 2 3 4 5
        """
        )
        sim = self._get_simulator_for_circuit(
            circuit,
            unconditional_condition_on_U=False,
            op_batch_size=8,
            sim_batch_size=8,
        )
        sim.interactive_do(circuit[0])
        sim.interactive_do(circuit[1])
        sim.interactive_do(circuit[2])
        sim.interactive_do(circuit[3])  # 3 disjoint skipped CZs on |++> -> d_b = 6 (r_b = 64)
        for coset in sim.active_shots.values():
            assert coset.d == 6
            assert coset.num_frames == 64
            coset.check_invariants()
        sim.interactive_do(circuit[4])
        sim.interactive_do(circuit[5])  # M collapses all 64 frames back to d_b = 0 (r_b = 1)
        assert len(sim.active_shots) == 0
        sim.finish_interactive_run()
        recs = sim.get_final_measurement_records()
        assert np.all(recs == 0)

    def test_mrqf_primitives_s1_to_s5_p1_to_p3_q1_and_collisions(self) -> None:
        """Verify MRQF routines S1-S5, P1-P3, PCP, Q1, collision branches, multi-word (>64q) invariants, and conditional gates."""
        from stimside.simulator_coset import MRQFState

        # 1. Example 14.1: S|+> measured in Y (d: 1 -> 0, deterministic m=0, rho=1.0)
        st = MRQFState(num_qubits=1)
        a1 = np.array([1], dtype=np.uint64)
        b0 = np.array([0], dtype=np.uint64)
        b1 = np.array([1], dtype=np.uint64)
        st.p2_apply_quarter_turn(a1, b0, kappa=2)
        assert st.d == 1
        st.check_invariants()
        assert st.q1_pauli_expectation(a1, b1, kappa=3) == 1
        st.p2_apply_quarter_turn(a1, b0, kappa=0)
        m, rho = st.p3_measure_diagonal(b1, sigma=0)
        assert m == 0 and rho == 1.0 and st.d == 0
        st.check_invariants()

        # 2. Example 14.4: CZ|++> via direct 2-column PCP injection, measured in X1 X2 (d: 2 -> 0 via S5 even-c collision branch!)
        st2 = MRQFState(num_qubits=2)
        st2.p2_apply_controlled_pauli(
            np.array([1], dtype=np.uint64),
            b0,
            0,
            np.array([2], dtype=np.uint64),
            b0,
            0,
        )
        assert st2.d == 2
        st2.check_invariants()
        assert st2.q1_pauli_expectation(np.array([3], dtype=np.uint64), b0, kappa=0) == 0
        st2.p2_apply_quarter_turn(np.array([3], dtype=np.uint64), b1, kappa=1)
        m2, rho2 = st2.p3_measure_diagonal(b1, sigma=0, requested_bit=0)
        assert m2 == 0 and rho2 == 0.5 and st2.d == 0
        st2.check_invariants()

        # 3. Direct S5.4 branches (c=0, lam=0 -> eta=2; c=2, lam=0 -> FAIL, eta=0) and C.5 postselection failure
        st_s5 = MRQFState(num_qubits=2)
        st_s5.A_words = np.array([[0]], dtype=np.uint64)
        st_s5.ell = np.array([0], dtype=np.uint8)
        st_s5.Gamma = np.zeros((1, 1), dtype=np.uint8)
        st_s5.P = [-1]
        ok0, eta0 = st_s5.s5_eliminate_zero(0)
        assert ok0 and eta0 == 2

        st_s5_fail = MRQFState(num_qubits=2)
        st_s5_fail.A_words = np.array([[0]], dtype=np.uint64)
        st_s5_fail.ell = np.array([2], dtype=np.uint8)
        st_s5_fail.Gamma = np.zeros((1, 1), dtype=np.uint8)
        st_s5_fail.P = [-1]
        ok2, eta2 = st_s5_fail.s5_eliminate_zero(0)
        assert not ok2 and eta2 == 0

        _, rho_fail = st.p3_measure_diagonal(b1, sigma=0, requested_bit=1)
        assert rho_fail == 0.0

        # 4. Multi-word (n = 150 > 128, n_words = 3) P1, P2, PCP, S5 inverse cancellation
        rng = np.random.default_rng(2026)
        st_mw = MRQFState(num_qubits=150)
        for _ in range(6):
            aw = rng.integers(0, 1 << 63, size=3, dtype=np.uint64)
            aw[2] &= np.uint64((1 << 22) - 1)
            aw[0] |= np.uint64(1)
            bw = rng.integers(0, 1 << 63, size=3, dtype=np.uint64)
            bw[2] &= np.uint64((1 << 22) - 1)
            kap = int(np.bitwise_count(aw & bw).sum()) & 1
            st_mw.p2_apply_quarter_turn(aw, bw, kap)
            st_mw.check_invariants()
        d_before = st_mw.d
        st_mw.p1_apply_pauli(aw, bw, kap)
        st_mw.p1_apply_pauli(aw, bw, kap)
        st_mw.p2_apply_quarter_turn(aw, bw, kap)
        st_mw.p2_apply_quarter_turn(aw, bw, (kap + 2) & 3)
        assert st_mw.d == d_before
        st_mw.check_invariants()

        # 5. Conditional inverse Clifford gates (including C_XYZ, C_ZYX, H_XY, H_YZ, SQRT_XX/YY/ZZ, YCX/YCY/YCZ, SWAP, ISWAP)
        cond_circ = stim.Circuit(
            """
            R 0 1 2 3 4
            H 0 1 2 3 4
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            S 0 1
            SQRT_X 0 2
            C_XYZ 0 1
            C_ZYX 0 2
            H_XY 0 3
            H_YZ 0 4
            CX 0 1 2 3
            CZ 0 2 1 3
            YCX 0 3 1 4
            YCY 0 4 2 3
            YCZ 0 1 3 4
            SQRT_XX 0 2 1 3
            SQRT_YY 0 3 2 4
            SQRT_ZZ 0 4 1 2
            SWAP 0 3
            ISWAP 0 4 1 2
            ISWAP_DAG 0 1 3 4
            H 0 1 2 3 4
            M 1 2 3 4
        """
        )
        op_tab = LeakageUint8Tableau(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=cond_circ, batch_size=1)
        tsim = TablesideSimulator(
            circuit=cond_circ, compiled_op_handler=op_tab, batch_size=1, seed=99,
            sync_tableside_rng=True,
        )
        tsim.run()
        op_cos = LeakageUint8Coset(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=cond_circ, batch_size=1)
        csim = CosetsideSimulator(
            circuit=cond_circ,
            compiled_op_handler=op_cos,
            batch_size=1,
            seed=99,
            sync_tableside_rng=True,
        )
        csim.run()
        np.testing.assert_array_equal(
            csim.get_final_measurement_records(),
            tsim.get_final_measurement_records(),
        )

        # 6. Dynamic buffer doubling beyond _cap = 16 -> 32 -> 64 -> 128 (d_b = 70)
        st_big = MRQFState(num_qubits=150)
        for q_i in range(70):
            aw = np.zeros(3, dtype=np.uint64)
            aw[q_i >> 6] = np.uint64(1 << (q_i & 63))
            bw = np.zeros(3, dtype=np.uint64)
            st_big.p2_apply_quarter_turn(aw, bw, 0)
        assert st_big.d == 70
        assert st_big._cap >= 128
        st_big.check_invariants()
        for q_i in range(70):
            bw = np.zeros(3, dtype=np.uint64)
            bw[q_i >> 6] = np.uint64(1 << (q_i & 63))
            st_big.p3_measure_diagonal(bw, 0, requested_bit=q_i & 1)
        assert st_big.d == 0
        st_big.check_invariants()

    def test_reference_chp_peek_pauli_expectation_readonly(self) -> None:
        """Verify ReferenceCHPTableau.peek_pauli_expectation is 100% read-only."""
        from stimside.util.reference_chp import ReferenceCHPTableau

        chp = ReferenceCHPTableau(6)
        chp.do_unitary("H", [0, 2])
        chp.do_unitary("CX", [0, 1, 2, 3, 3, 4])
        chp.do_unitary("S", [1, 4])
        chp.do_unitary("X", [5])

        table_snapshot = chp.snapshot_packed()
        res_1q = [chp.peek_pauli_expectation([(q, "Z")])[0] for q in range(6)]
        for a, b in zip(chp.snapshot_packed(), table_snapshot):
            assert np.array_equal(a, b)
        assert res_1q[5] == -1
        assert res_1q[0] == 0

        res_bell = chp.peek_pauli_expectation([(0, "Z"), (1, "Z")])
        res_ghz = chp.peek_pauli_expectation([(2, "X"), (3, "X"), (4, "Y")])
        res_multi = chp.peek_pauli_expectation(
            [(0, "Z"), (1, "Z"), (2, "X"), (3, "X"), (4, "Y"), (5, "Z")]
        )
        res_anticomm = chp.peek_pauli_expectation([(0, "X"), (5, "Z")])
        for a, b in zip(chp.snapshot_packed(), table_snapshot):
            assert np.array_equal(a, b)
        assert res_bell == [1]
        assert res_ghz == [1]
        assert res_multi == [-1]
        assert res_anticomm == [0]

    @pytest.mark.parametrize("batch_size", [1, 64])
    def test_cpp_vs_python_bit_for_bit_equivalence(self, batch_size: int) -> None:
        """Verify use_cpp_kernels=True and use_cpp_kernels=False are bit-for-bit identical."""
        raw = stim.Circuit.generated(
            "surface_code:rotated_memory_z",
            distance=5,
            rounds=4,
            after_clifford_depolarization=0.002,
            before_round_data_depolarization=0.002,
            before_measure_flip_probability=0.002,
            after_reset_flip_probability=0.002,
        )

        def _build_leakage_circuit(circ: stim.Circuit) -> stim.Circuit:
            out = stim.Circuit()
            p = 0.015
            for op in circ:
                if isinstance(op, stim.CircuitInstruction):
                    if op.name == "CX":
                        t = op.targets_copy()
                        out.append(stim.CircuitInstruction("H", t[1::2]))
                        out.append(
                            stim.CircuitInstruction(
                                "CZ", t, tag="CONDITIONED_ON_PAIR: (U, U)"
                            )
                        )
                        out.append(
                            stim.CircuitInstruction(
                                "II_ERROR",
                                t,
                                tag=(
                                    f"LEAKAGE_TRANSITION_2: ({p/4}, U_U-->U_2) "
                                    f"({p/4}, U_U-->2_U) ({p/4}, U_U-->2_2)"
                                ),
                            )
                        )
                        out.append(stim.CircuitInstruction("H", t[1::2]))
                    elif op.name in ("M", "MR"):
                        t = op.targets_copy()
                        out.append(
                            stim.CircuitInstruction(
                                "I_ERROR",
                                t,
                                tag="LEAKAGE_TRANSITION_1: (0.02, 1-->2) (0.8, 2-->0) (0.2, 2-->1)",
                            )
                        )
                        out.append(op)
                    else:
                        out.append(op)
                elif isinstance(op, stim.CircuitRepeatBlock):
                    body = _build_leakage_circuit(op.body_copy())
                    for _ in range(op.repeat_count):
                        out += body
            return out

        mod_circ = _build_leakage_circuit(raw)
        for seed in (42, 314159):
            op_cpp = LeakageUint8Coset(
                unconditional_condition_on_U=True
            ).compile_op_handler(circuit=mod_circ, batch_size=batch_size)
            sim_cpp = CosetsideSimulator(
                circuit=mod_circ,
                compiled_op_handler=op_cpp,
                batch_size=batch_size,
                seed=seed,
                use_cpp_kernels=True,
            )
            sim_cpp.run()

            op_py = LeakageUint8Coset(
                unconditional_condition_on_U=True
            ).compile_op_handler(circuit=mod_circ, batch_size=batch_size)
            sim_py = CosetsideSimulator(
                circuit=mod_circ,
                compiled_op_handler=op_py,
                batch_size=batch_size,
                seed=seed,
                use_cpp_kernels=False,
            )
            sim_py.run()

            np.testing.assert_array_equal(
                sim_cpp.get_final_measurement_records(),
                sim_py.get_final_measurement_records(),
            )
            np.testing.assert_array_equal(
                sim_cpp.get_detector_flips(),
                sim_py.get_detector_flips(),
            )
            np.testing.assert_array_equal(
                sim_cpp.get_observable_flips(),
                sim_py.get_observable_flips(),
            )
            np.testing.assert_array_equal(op_cpp.state, op_py.state)

    def test_cpp_vs_python_multiword_over_512_qubits_and_active_shots_sync(
        self,
    ) -> None:
        """Verify >512 qubits (W=9 words) and bidirectional active_shots synchronization."""
        circuit = stim.Circuit(
            """
            R 0 1 63 64 511 512 528 529
            H 0 63 511 528
            CX 0 1 63 64 511 512 528 529
            I[LEAKAGE_TRANSITION_1: (0.4, U-->2)] 0 63 511 528
            CZ[CONDITIONED_ON_PAIR: (U, U)] 0 64 63 512 511 529 528 1
            I[LEAKAGE_TRANSITION_1: (0.5, 1-->2) (0.7, 2-->0) (0.3, 2-->1)] 0 1 63 64 511 512 528 529
            CX[CONDITIONED_ON_PAIR: (U, U)] 1 529 64 512
            M 0 1 63 64 511 512 528 529
            DETECTOR rec[-8] rec[-7]
            OBSERVABLE_INCLUDE(0) rec[-2] rec[-1]
        """
        )
        assert circuit.num_qubits == 530
        batch_size = 48
        seed = 20260920

        op_cpp = LeakageUint8Coset(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=circuit, batch_size=batch_size)
        sim_cpp = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=op_cpp,
            batch_size=batch_size,
            seed=seed,
            use_cpp_kernels=True,
        )

        op_py = LeakageUint8Coset(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=circuit, batch_size=batch_size)
        sim_py = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=op_py,
            batch_size=batch_size,
            seed=seed,
            use_cpp_kernels=False,
        )

        for idx, inst in enumerate(circuit):
            sim_cpp.interactive_do(inst)
            sim_py.interactive_do(inst)
            if idx == 4:
                # Inspect active_shots mid-circuit to trigger _sync_cpp_to_py and _sync_py_to_cpp
                assert set(sim_cpp.active_shots.keys()) == set(
                    sim_py.active_shots.keys()
                )
                for b_idx in sim_py.active_shots:
                    assert (
                        sim_cpp.active_shots[b_idx].d
                        == sim_py.active_shots[b_idx].d
                    )
                    np.testing.assert_array_equal(
                        sim_cpp.active_shots[b_idx].h_words,
                        sim_py.active_shots[b_idx].h_words,
                    )
                if sim_py.active_shots:
                    b0 = sorted(sim_py.active_shots.keys())[0]
                    st_cpp = sim_cpp.active_shots[b0]
                    st_py = sim_py.active_shots[b0]
                    # Trigger a read-only peek_z and then mutate the held MRQFState in-place
                    _ = sim_cpp.peek_z(0)
                    _ = sim_py.peek_z(0)
                    st_cpp.h_words[0] ^= np.uint64(1)
                    st_py.h_words[0] ^= np.uint64(1)
                    st_cpp.A_words[0, 0] ^= np.uint64(1)
                    st_py.A_words[0, 0] ^= np.uint64(1)

        sim_cpp.finish_interactive_run()
        sim_py.finish_interactive_run()

        np.testing.assert_array_equal(
            sim_cpp.get_final_measurement_records(),
            sim_py.get_final_measurement_records(),
        )
        np.testing.assert_array_equal(
            sim_cpp.get_detector_flips(),
            sim_py.get_detector_flips(),
        )
        np.testing.assert_array_equal(
            sim_cpp.get_observable_flips(),
            sim_py.get_observable_flips(),
        )
        np.testing.assert_array_equal(op_cpp.state, op_py.state)

    def test_exotic_multilevel_transitions_and_v_d_xyz_tokens_cross_validation(
        self,
    ) -> None:
        """Verify multi-level leakage (states 2..9) and output tokens (0, 1, U, D, V, X, Y, Z) in CosetsideSimulator."""
        circuit = stim.Circuit(
            """
            R 0 1 2 3 4 5 6 7
            X 2
            I[LEAKAGE_TRANSITION_1: (1.0, 0-->2)] 1 3 5 6
            I[LEAKAGE_TRANSITION_1: (1.0, 2<->3)] 1
            I[LEAKAGE_TRANSITION_1: (1.0, 3-->4)] 1
            I[LEAKAGE_TRANSITION_1: (1.0, 4-->3)] 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_3-->V_Y)] 0 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 1_2-->X_Z)] 2 3
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_2-->D_0)] 4 5
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 6
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->U_V)] 6 7
            M 0 1 2 3 4 5 6 7
            """
        )
        batch_size = 256
        for use_cpp in (True, False):
            coh = LeakageUint8Coset(
                unconditional_condition_on_U=True
            ).compile_op_handler(circuit=circuit, batch_size=batch_size)
            sim = CosetsideSimulator(
                circuit=circuit,
                compiled_op_handler=coh,
                batch_size=batch_size,
                seed=2026,
                use_cpp_kernels=use_cpp,
            )
            sim.run()
            recs = sim.get_final_measurement_records()
            assert np.all(coh.state == 0)
            # q0 (from U->V), q4 (from U->D), q6 (from 2->U), q7 (from U->V) must be 50/50 randomized, NOT deterministic 1!
            for q_rand in (0, 4, 6, 7):
                assert 0.35 < np.mean(recs[:, q_rand]) < 0.65
            # q1 (from 3->Y) -> |1>
            assert np.all(recs[:, 1] == 1)
            # q2 (from 1->X) -> |0>, q3 (from 2->Z) -> |0>, q5 (from 2->0) -> |0>
            assert np.all(recs[:, 2] == 0)
            assert np.all(recs[:, 3] == 0)
            assert np.all(recs[:, 5] == 0)

        # Shot-for-shot equivalence with TablesideSimulator when sync_tableside_rng=True
        for seed in (11, 22, 33):
            op_tab = LeakageUint8Tableau(
                unconditional_condition_on_U=True
            ).compile_op_handler(circuit=circuit, batch_size=1)
            tsim = TablesideSimulator(
                circuit=circuit,
                compiled_op_handler=op_tab,
                batch_size=1,
                seed=seed,
            )
            tsim.run()

            op_cos = LeakageUint8Coset(
                unconditional_condition_on_U=True
            ).compile_op_handler(circuit=circuit, batch_size=1)
            csim = CosetsideSimulator(
                circuit=circuit,
                compiled_op_handler=op_cos,
                batch_size=1,
                seed=seed,
                sync_tableside_rng=True,
            )
            csim.run()
            np.testing.assert_array_equal(
                csim.get_final_measurement_records(),
                tsim.get_final_measurement_records(),
            )
            np.testing.assert_array_equal(
                op_cos.state.reshape(-1), op_tab.state.reshape(-1)
            )

    def test_exotic_conditional_cliffords_classical_feedback_and_correlated_errors(
        self,
    ) -> None:
        """Verify CONDITIONED_ON_SELF/OTHER/PAIR on higher leakage levels, classical feedback, and CORRELATED_ERROR chains."""
        circuit = stim.Circuit(
            """
            RX 0
            RY 1
            MX 0
            MY 1
            R 0 1 2 3 4
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->3)] 1
            SQRT_X[CONDITIONED_ON_SELF: 2] 0 2
            SQRT_X[CONDITIONED_ON_OTHER: 2 3 : 0] 2
            SQRT_X[CONDITIONED_ON_OTHER: 2 3 : 1] 2
            CX[CONDITIONED_ON_PAIR: (U, U)] 2 3 0 4
            CX[CONDITIONED_ON_PAIR: (U, U)] 3 4
            MPAD[LEAKAGE_MEASUREMENT: (0.0, 0) (0.0, 1) (1.0, 2) (1.0, 3) : 0 2] 0 0
            CZ rec[-2] 2
            CX rec[-2] 4 rec[-1] 3
            M[LEAKAGE_PROJECTION_Z: (0.0, 0) (1.0, 1) (1.0, 2) (1.0, 3)] !0 !1 2 3 4
            CORRELATED_ERROR(1.0) X0 X2
            ELSE_CORRELATED_ERROR(1.0) X3
            CORRELATED_ERROR(0.0) X2
            ELSE_CORRELATED_ERROR(1.0) X4
            M 2 3 4
            OBSERVABLE_INCLUDE(0) rec[-3] rec[-2] rec[-1]
            REPEAT 2 {
                H 2 3
                SQRT_XX[CONDITIONED_ON_PAIR: (U, U)] 2 3
                SQRT_XX_DAG[CONDITIONED_ON_PAIR: (U, U)] 2 3
                MXX 2 3
                MPP Z2*Z3
                H 2 3
                SHIFT_COORDS(1, 1)
                DETECTOR(0, 0) rec[-1]
            }
            """
        )
        batch_size = 32
        op_cpp = LeakageUint8Coset(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=circuit, batch_size=batch_size)
        sim_cpp = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=op_cpp,
            batch_size=batch_size,
            seed=555,
            use_cpp_kernels=True,
        )
        sim_cpp.run()

        op_py = LeakageUint8Coset(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=circuit, batch_size=batch_size)
        sim_py = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=op_py,
            batch_size=batch_size,
            seed=555,
            use_cpp_kernels=False,
        )
        sim_py.run()

        recs = sim_cpp.get_final_measurement_records()
        np.testing.assert_array_equal(
            recs, sim_py.get_final_measurement_records()
        )
        np.testing.assert_array_equal(
            sim_cpp.get_detector_flips(), sim_py.get_detector_flips()
        )
        np.testing.assert_array_equal(
            sim_cpp.get_observable_flips(), sim_py.get_observable_flips()
        )
        # Verify MX 0 == 0, MY 1 == 0
        assert np.all(recs[:, 0] == 0)
        assert np.all(recs[:, 1] == 0)
        # MPAD on q0 (state 2) -> 1, on q2 (state 0) -> 0
        assert np.all(recs[:, 2] == 1)
        assert np.all(recs[:, 3] == 0)
        # M[LEAKAGE_PROJECTION_Z: ... (1.0, 2) (1.0, 3)] !0 !1 -> inverted 1 is 0!
        assert np.all(recs[:, 4] == 0)
        assert np.all(recs[:, 5] == 0)
        # q2 had two SQRT_X gates -> |1>; q3 had CX from q2 -> |1>; q4 had CX from q3 -> |1> then CX rec[-2] 4 -> |0>
        assert np.all(recs[:, 6] == 1)
        assert np.all(recs[:, 7] == 1)
        assert np.all(recs[:, 8] == 0)
        # After CORRELATED_ERROR(1.0) X0 X2 (skipped entirely because q0 is leaked, so q2 stays |1>; ELSE X3 skipped) and CORRELATED_ERROR(0.0) + ELSE_CORRELATED_ERROR(1.0) X4 (flips q4 to |1>):
        # M 2 3 4 at indices 9, 10, 11 must be [1, 1, 1]
        assert np.all(recs[:, 9] == 1)
        assert np.all(recs[:, 10] == 1)
        assert np.all(recs[:, 11] == 1)

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_bell_state_peek_z_projection_and_readout_noise_shot_for_shot_lockstep(
        self, use_cpp: bool
    ) -> None:
        """Verify Bell-pair collapse under mid-circuit 1-->2 transitions, classical readout noise non-backaction, and shot-for-shot lockstep."""
        # 1. Mid-circuit 1-->2 transition on an entangled Bell pair must collapse Z0=Z1 coherently
        # so either both qubits are in |0> (and neither leaks) or both are in |1> (and both leak to 2).
        bell_leak_circuit = stim.Circuit(
            """
            H 0
            CX 0 1
            I[LEAKAGE_TRANSITION_1: (1.0, 1-->2)] 0 1
            MPAD[LEAKAGE_MEASUREMENT: (0.0, 0) (0.0, 1) (1.0, 2) : 0 1] 0 0
            M[LEAKAGE_PROJECTION_Z: (0.0, 0) (1.0, 1) (0.0, 2)] 0 1
            """
        )
        batch_size = 64
        op_cos = LeakageUint8Coset(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=bell_leak_circuit, batch_size=batch_size)
        sim_cos = CosetsideSimulator(
            circuit=bell_leak_circuit,
            compiled_op_handler=op_cos,
            batch_size=batch_size,
            seed=777,
            use_cpp_kernels=use_cpp,
            sync_tableside_rng=True,
        )
        sim_cos.run()
        cos_recs = sim_cos.get_final_measurement_records()
        # Both qubits in each Bell pair must have identical leakage status (rec[0] == rec[1])
        np.testing.assert_array_equal(cos_recs[:, 0], cos_recs[:, 1])

        # Verify 100% shot-for-shot agreement with TablesideSimulator(sync_tableside_rng=True)
        op_tab = LeakageUint8Tableau(
            unconditional_condition_on_U=True
        ).compile_op_handler(circuit=bell_leak_circuit, batch_size=batch_size)
        sim_tab = TablesideSimulator(
            circuit=bell_leak_circuit,
            compiled_op_handler=op_tab,
            batch_size=batch_size,
            seed=777,
            sync_tableside_rng=True,
        )
        sim_tab.run()
        np.testing.assert_array_equal(cos_recs, sim_tab.get_final_measurement_records())

        # 2. Verify classical readout flip (p0=1.0, p1=1.0) flips measurement record to 1 without flipping physical qubit |0> to |1>
        readout_non_backaction = stim.Circuit(
            """
            R 0
            M[LEAKAGE_PROJECTION_Z: (1.0, 0) (1.0, 1)] 0
            M 0
            """
        )
        for sync_mode in (False, True):
            op_nb = LeakageUint8Coset(
                unconditional_condition_on_U=True
            ).compile_op_handler(circuit=readout_non_backaction, batch_size=16)
            sim_nb = CosetsideSimulator(
                circuit=readout_non_backaction,
                compiled_op_handler=op_nb,
                batch_size=16,
                seed=42,
                use_cpp_kernels=use_cpp,
                sync_tableside_rng=sync_mode,
            )
            sim_nb.run()
            nb_recs = sim_nb.get_final_measurement_records()
            assert np.all(nb_recs[:, 0] == 1)
            assert np.all(nb_recs[:, 1] == 0)

        # 3. Verify shot-for-shot lockstep on symmetric and asymmetric M[LEAKAGE_PROJECTION_Z] + 2Q single-group CONDITIONED_ON: (2, 3)
        lockstep_circuit = stim.Circuit(
            """
            H 0 2
            CX 0 1 2 3
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 4
            I[LEAKAGE_TRANSITION_1: (1.0, U-->3)] 5
            CX[CONDITIONED_ON_PAIR: (2, 3) (3, 2)] 4 5
            X[CONDITIONED_ON_OTHER: U : 4] 6
            M[LEAKAGE_PROJECTION_Z: (0.15, 0) (0.85, 1) (0.75, 2)] 0 1 4
            M[LEAKAGE_PROJECTION_Z: (0.10, 0) (0.75, 1) (0.60, 3)] 2 3 5
            M 6
            """
        )
        n_shots = 32
        op_ls_cos = LeakageUint8Coset(
            unconditional_condition_on_U=False
        ).compile_op_handler(circuit=lockstep_circuit, batch_size=n_shots)
        sim_ls_cos = CosetsideSimulator(
            circuit=lockstep_circuit,
            compiled_op_handler=op_ls_cos,
            batch_size=n_shots,
            seed=900,
            use_cpp_kernels=use_cpp,
            sync_tableside_rng=True,
        )
        sim_ls_cos.run()
        ls_cos_recs = sim_ls_cos.get_final_measurement_records()

        op_ls_tab = LeakageUint8Tableau(
            unconditional_condition_on_U=False
        ).compile_op_handler(circuit=lockstep_circuit, batch_size=n_shots)
        sim_ls_tab = TablesideSimulator(
            circuit=lockstep_circuit,
            compiled_op_handler=op_ls_tab,
            batch_size=n_shots,
            seed=900,
            sync_tableside_rng=True,
        )
        sim_ls_tab.run()
        np.testing.assert_array_equal(ls_cos_recs, sim_ls_tab.get_final_measurement_records())

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_sync_tableside_rng_mpad_equal_01_readout_lockstep(self, use_cpp: bool):
        """MPAD[LEAKAGE_MEASUREMENT] with explicit equal (p, 0), (p, 1) must match Tableside shot-for-shot."""
        circuit = stim.Circuit(
            """
            X 0
            H 1
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 2
            MPAD[LEAKAGE_MEASUREMENT: (0.2, 0), (0.2, 1), (0.6, 2) : 0 1 2 3] 0 1 0 0
            M 0 1 3
            """
        )
        n_shots = 32
        op_cos = LeakageUint8Coset(unconditional_condition_on_U=False).compile_op_handler(
            circuit=circuit, batch_size=n_shots
        )
        sim_cos = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=op_cos,
            batch_size=n_shots,
            seed=900,
            use_cpp_kernels=use_cpp,
            sync_tableside_rng=True,
        )
        sim_cos.run()
        op_tab = LeakageUint8Tableau(unconditional_condition_on_U=False).compile_op_handler(
            circuit=circuit, batch_size=n_shots
        )
        sim_tab = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=op_tab,
            batch_size=n_shots,
            seed=900,
            sync_tableside_rng=True,
        )
        sim_tab.run()
        np.testing.assert_array_equal(
            sim_cos.get_final_measurement_records(), sim_tab.get_final_measurement_records()
        )

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_mpad_01_readout_collapses_superposition_like_tableside(self, use_cpp: bool):
        """Non-sync Coset: MPAD[LEAKAGE_MEASUREMENT] with 0/1 keys Z-collapses a superposed qubit, as Tableside bs=1 does."""
        p0, p1 = 0.1, 0.7
        tag = "(0.1, 0), (0.7, 1), (0.9, 2) : 0"
        # Bell pair; MPAD on qubit 0. Collapse => XX parity becomes random; readout is p0/p1 noise on the Z outcome.
        c_x = stim.Circuit(f"H 0\nCX 0 1\nMPAD[LEAKAGE_MEASUREMENT: {tag}] 0\nH 0 1\nM 0 1")
        c_z = stim.Circuit(f"H 0\nCX 0 1\nMPAD[LEAKAGE_MEASUREMENT: {tag}] 0\nM 1")

        def cos_records(circuit: stim.Circuit, n: int) -> np.ndarray:
            op = LeakageUint8Coset().compile_op_handler(circuit=circuit, batch_size=n)
            sim = CosetsideSimulator(
                circuit=circuit, compiled_op_handler=op, batch_size=n, seed=11, use_cpp_kernels=use_cpp
            )
            sim.run()
            return np.asarray(sim.get_final_measurement_records(), dtype=bool)

        def tab_records(circuit: stim.Circuit, n: int) -> np.ndarray:
            recs = []
            for s in range(n):
                op = LeakageUint8Tableau().compile_op_handler(circuit=circuit, batch_size=1)
                sim = TablesideSimulator(circuit=circuit, compiled_op_handler=op, batch_size=1, seed=500 + s)
                sim.run()
                recs.append(sim.get_final_measurement_records()[0])
            return np.asarray(recs, dtype=bool)

        for recs_x, recs_z in (
            (cos_records(c_x, 4000), cos_records(c_z, 4000)),
            (tab_records(c_x, 300), tab_records(c_z, 300)),
        ):
            n_x = len(recs_x)
            assert abs(np.mean(recs_x[:, 1] ^ recs_x[:, 2]) - 0.5) < 4.5 * 0.5 / np.sqrt(n_x)
            r, m = recs_z[:, 0], recs_z[:, 1]
            for mask, p in ((~m, p0), (m, p1)):
                tol = 4.5 * np.sqrt(p * (1 - p) / mask.sum())
                assert abs(np.mean(r[mask]) - p) < tol

    def test_interactive_do_divergence_raises_but_extension_works(self) -> None:
        circuit = stim.Circuit("X 0\nM 0")
        with pytest.raises(ValueError, match="`I 0` doesn't match the circuit .* has `X 0` there"):
            # Used to give M 0 = 1: the op was compiled onto the reference after the whole circuit.
            self._get_simulator_for_circuit(circuit).interactive_do(stim.Circuit("I 0\nM 0"))
        tss = self._get_simulator_for_circuit(circuit)
        tss.interactive_do(circuit)
        tss.interactive_do(stim.Circuit("H 0\nM 0"))  # past the end of the circuit: still supported
        tss.finish_interactive_run()
        recs = tss.get_final_measurement_records()
        assert recs[:, 0].all() and 0 < recs[:, 1].mean() < 1
        tss.clear()
        tss.interactive_do(circuit)
        with pytest.raises(ValueError, match="doesn't match `H 0`, operation 2"):
            # Used to give M 0 = 0: the extension above stays on the reference after clear().
            tss.interactive_do(stim.Circuit("M 0"))

    def test_interactive_do_replays_mixed_controls_and_compares_exactly(self) -> None:
        # The reference stores `CX 0 1 rec[-1] 2` as `CX 0 1`; replaying the circuit must still work.
        circuit = stim.Circuit("X 0\nM 0\nCX 0 1 rec[-1] 2\nM 1 2")
        tss = self._get_simulator_for_circuit(circuit)
        tss.interactive_do(circuit)
        tss.finish_interactive_run()
        assert tss.get_final_measurement_records().all()
        # Same target values but different target flags: used to be accepted, reusing the
        # construction circuit's inversion or Pauli basis, or doing CX 0 1 for CX sweep[0] 1.
        for a, b in [
            ("X 0\nM 0", "X 0\nM !0"),
            ("R 0 1\nMPP X0*X1", "R 0 1\nMPP Z0*Z1"),
            ("X 0\nCX 0 1", "X 0\nCX sweep[0] 1"),
        ]:
            with pytest.raises(ValueError, match="doesn't match"):
                self._get_simulator_for_circuit(stim.Circuit(a)).interactive_do(stim.Circuit(b))

    @staticmethod
    def _outputs(sim: CosetsideSimulator) -> list[np.ndarray]:
        return [
            sim.get_final_measurement_records(),
            sim.get_detector_flips(),
            sim.get_observable_flips(),
        ]

    def test_interactive_do_exact_replay_matches_run(self) -> None:
        circuit = stim.Circuit(
            """
            R 0 1 2
            H 0
            DEPOLARIZE1(0.2) 0 1
            M 0
            CX 0 1 rec[-1] 2
            REPEAT 2 {
                H 1
                CZ rec[-1] 1 0 2
                X_ERROR(0.1) 2
                M(0.05) 1 2
                DETECTOR rec[-1] rec[-2]
            }
            OBSERVABLE_INCLUDE(0) rec[-1]
            """
        )
        ran, replayed = (self._get_simulator_for_circuit(circuit) for _ in range(2))
        ran.run()
        replayed.interactive_do(circuit)
        replayed.finish_interactive_run()
        for x, y in zip(self._outputs(ran), self._outputs(replayed)):
            np.testing.assert_array_equal(x, y)

    def test_interactive_do_noise_strength_may_differ(self) -> None:
        # Untagged noise is applied live from the interactive op; nothing precomputed depends on it.
        def text(p: float) -> str:
            return (
                f"R 0 1\nH 0\nX_ERROR({p}) 0 1\nDEPOLARIZE2({p}) 0 1\nHERALDED_ERASE({p}) 1\n"
                f"M({p}) 0 1\nDETECTOR rec[-1] rec[-2]\nOBSERVABLE_INCLUDE(0) rec[-3]"
            )

        truth = self._get_simulator_for_circuit(stim.Circuit(text(0.3)))
        truth.run()
        tss = self._get_simulator_for_circuit(stim.Circuit(text(0)))
        tss.interactive_do(stim.Circuit(text(0.3)))
        tss.finish_interactive_run()
        for x, y in zip(self._outputs(truth), self._outputs(tss)):
            np.testing.assert_array_equal(x, y)
        assert 0 < truth.get_detector_flips().mean() < 1

    def test_interactive_do_checks_tags_and_arguments(self) -> None:
        leak = "I[LEAKAGE_TRANSITION_1: ({}, U-->2)] 0"
        for a, b in [
            ("X 0\nCX 0 1\nM 1", "X 0\nCX[foo] 0 1\nM 1"),  # only a tag added
            ("R 0\nI 0\nX 0\nM 0", f"R 0\n{leak.format(1.0)}\nX 0\nM 0"),
            (f"R 0\n{leak.format(0.0)}\nX 0\nM 0", f"R 0\n{leak.format(1.0)}\nX 0\nM 0"),
            ("X_ERROR[t](0.1) 0", "X_ERROR[t](0.2) 0"),  # a tagged gate's strength
            ("MPAD(0) 0", "MPAD(1) 0"),  # MPAD isn't a noisy gate
            (
                "M 0 1\nOBSERVABLE_INCLUDE(0) rec[-1]\nOBSERVABLE_INCLUDE(1) rec[-2]",
                "M 0 1\nOBSERVABLE_INCLUDE(1) rec[-1]\nOBSERVABLE_INCLUDE(0) rec[-2]",
            ),
            ("M 0\nDETECTOR(1) rec[-1]", "M 0\nDETECTOR(2) rec[-1]"),
            ("M 0 1\nDETECTOR rec[-1]", "M 0 1\nDETECTOR rec[-2]"),
        ]:
            # The handlers pass tag-stripped ops on, and the detector/observable converter is the
            # construction circuit's, so these used to be accepted with the old tag / definition.
            with pytest.raises(ValueError, match="doesn't match the circuit"):
                self._get_simulator_for_circuit(stim.Circuit(a)).interactive_do(stim.Circuit(b))
        # flattened() drops the SHIFT_COORDS (operation 2 here), so it doesn't replay the circuit.
        rounds = stim.Circuit("REPEAT 2 {\n    M 0\n    DETECTOR(0) rec[-1]\n    SHIFT_COORDS(1)\n}")
        with pytest.raises(ValueError, match=r"not `circuit\.flattened\(\)` or `circuit \+ more`"):
            self._get_simulator_for_circuit(rounds).interactive_do(rounds.flattened())

    def test_interactive_do_past_the_end(self) -> None:
        circuit = stim.Circuit("R 0 1\nX 0\nM 0\nDETECTOR rec[-1]")
        extension = stim.Circuit("H 1\nCX 1 0\nX_ERROR(0.2) 0\nTICK\nM 0 1")
        truth = self._get_simulator_for_circuit(circuit + extension)
        truth.run()
        tss = self._get_simulator_for_circuit(circuit)
        tss.interactive_do(circuit)
        tss.interactive_do(extension)
        tss.finish_interactive_run()
        np.testing.assert_array_equal(
            tss.get_final_measurement_records(), truth.get_final_measurement_records()
        )
        with pytest.raises(ValueError):  # the construction circuit's converter: 1 measurement
            tss.get_detector_flips()
        for op in [
            "DETECTOR rec[-1]",
            "OBSERVABLE_INCLUDE(0) rec[-1]",
            "I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0",
            "X[foo] 0",
        ]:
            tss = self._get_simulator_for_circuit(circuit)
            tss.interactive_do(circuit)
            with pytest.raises(ValueError, match="past the end of the circuit"):
                tss.interactive_do(stim.Circuit(op))
        tss = self._get_simulator_for_circuit(circuit)  # 2 qubits
        tss.interactive_do(circuit)
        with pytest.raises(ValueError, match="targets qubit 2, which the CosetsideSimulator wasn't"):
            tss.interactive_do(stim.Circuit("H 2"))  # used to be a raw IndexError

    @pytest.mark.parametrize("gate", ["SPP", "SPP_DAG"])
    def test_spp_raises_not_implemented(self, gate: str) -> None:
        # Pauli product rotations used to crash the reference compiler with a TypeError / IndexError.
        for targets in ("Z0*Y1", "X0"):
            with pytest.raises(NotImplementedError, match=f"does not support {gate} "):
                self._get_simulator_for_circuit(stim.Circuit(f"R 0 1\n{gate} {targets}\nM 0 1"))
        tss = self._get_simulator_for_circuit(stim.Circuit("R 0 1"))
        tss.interactive_do(stim.Circuit("R 0 1"))
        with pytest.raises(NotImplementedError, match=f"does not support {gate} "):
            tss.interactive_do(stim.Circuit(f"{gate} Z0*Y1"))

    @pytest.mark.parametrize("use_cpp", [True, False])
    def test_correlated_error_on_a_leaked_qubit_is_skipped_like_pauli_channel_2(self, use_cpp: bool) -> None:
        # E(1) X1 X3 is the same channel as PAULI_CHANNEL_2(XX=1) 1 3, so in the shots where q1 is leaked it must not
        # flip q3 either. A skipped E still counts as fired, so the following ELSE_CORRELATED_ERROR stays blocked.
        leak = "I[LEAKAGE_TRANSITION_1: (0.5, U-->2)] 1\nMPAD[LEAKAGE_MEASUREMENT: (1.0, 2) : 1] 0"
        pc2_xx = "PAULI_CHANNEL_2(0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) 1 3"
        for noise in (
            "E(1) X1 X3",
            "E(1) X3 X1",
            pc2_xx,
            "E(1) X1 X3\nELSE_CORRELATED_ERROR(1) X4",
            "E(1) X3 X4",
        ):
            circuit = stim.Circuit(f"R 1 3 4\n{leak}\n{noise}\nM 3 4")
            op = LeakageUint8Coset().compile_op_handler(circuit=circuit, batch_size=64)
            sim = CosetsideSimulator(
                circuit=circuit, compiled_op_handler=op, batch_size=64, seed=5, use_cpp_kernels=use_cpp
            )
            sim.run()
            recs = np.asarray(sim.get_final_measurement_records(), dtype=bool)
            leaked = recs[:, 0]
            assert 0 < leaked.sum() < len(leaked)
            if noise == "E(1) X3 X4":  # doesn't touch q1: always applied
                assert np.all(recs[:, 1:])
            else:
                np.testing.assert_array_equal(recs[:, 1], ~leaked, err_msg=noise)
                assert not np.any(recs[:, 2]), noise

