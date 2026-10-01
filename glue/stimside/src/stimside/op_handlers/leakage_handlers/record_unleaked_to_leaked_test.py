import numpy as np
import pytest
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import (
    LeakageUint8Coset,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    LeakageUint8 as LeakageUint8Flip,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
    LeakageUint8 as LeakageUint8Tableau,
)
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_flip import FlipsideSimulator
from stimside.simulator_tableau import TablesideSimulator
from stimside.util.known_states import _unroll_circuit


def _build_flip_sim(
    circuit: stim.Circuit,
    batch_size: int = 4,
    seed: int = 42,
    record_unleaked_to_leaked: bool = True,
) -> FlipsideSimulator:
    coh = LeakageUint8Flip().compile_op_handler(
        circuit=circuit, batch_size=batch_size
    )
    return FlipsideSimulator(
        circuit=circuit,
        compiled_op_handler=coh,
        batch_size=batch_size,
        seed=seed,
        record_unleaked_to_leaked=record_unleaked_to_leaked,
    )


def _build_tab_sim(
    circuit: stim.Circuit,
    seed: int = 42,
    record_unleaked_to_leaked: bool = True,
    use_cpp_kernels: bool | None = None,
    sync_tableside_rng: bool | None = None,
) -> TablesideSimulator:
    coh = LeakageUint8Tableau().compile_op_handler(
        circuit=circuit, batch_size=1
    )
    return TablesideSimulator(
        circuit=circuit,
        compiled_op_handler=coh,
        batch_size=1,
        seed=seed,
        use_cpp_kernels=use_cpp_kernels,
        sync_tableside_rng=sync_tableside_rng,
        record_unleaked_to_leaked=record_unleaked_to_leaked,
    )


def _build_coset_sim(
    circuit: stim.Circuit,
    batch_size: int = 4,
    seed: int = 42,
    record_unleaked_to_leaked: bool = True,
    use_cpp_kernels: bool = True,
    sync_tableside_rng: bool = False,
) -> CosetsideSimulator:
    coh = LeakageUint8Coset().compile_op_handler(
        circuit=circuit, batch_size=batch_size
    )
    return CosetsideSimulator(
        circuit=circuit,
        compiled_op_handler=coh,
        batch_size=batch_size,
        seed=seed,
        use_cpp_kernels=use_cpp_kernels,
        sync_tableside_rng=sync_tableside_rng,
        record_unleaked_to_leaked=record_unleaked_to_leaked,
    )


def test_default_flag_disabled_and_raises() -> None:
    circuit = stim.Circuit(
        """
        R 0
        I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        M 0
        """
    )
    fss = _build_flip_sim(circuit, record_unleaked_to_leaked=False)
    tss = _build_tab_sim(circuit, record_unleaked_to_leaked=False)
    css = _build_coset_sim(circuit, record_unleaked_to_leaked=False)

    for sim in (fss, tss, css):
        assert sim.record_unleaked_to_leaked is False
        assert sim.run() is None
        with pytest.raises(ValueError, match="record_unleaked_to_leaked=False"):
            sim.get_unleaked_to_leaked_records()
        with pytest.raises(ValueError, match="record_unleaked_to_leaked=False"):
            sim.get_unleaked_to_leaked_op_indices()
        with pytest.raises(ValueError, match="record_unleaked_to_leaked=False"):
            _ = sim.unleaked_to_leaked_records


def test_only_unleaked_to_leaked_transitions_recorded() -> None:
    """Verify that ONLY unleaked (<2) -> leaked (>=2) transitions are recorded,
    and NOT 2->3, 2->2, 2->0/U, or U->D/X/Y/Z/0/1.
    """
    circuit = stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        R 0 1 2
        TICK
        # op_idx 5: U_U -> D_D (unleaked to unleaked, must NOT record)
        II[LEAKAGE_TRANSITION_2: (1.0, U_U-->D_D)] 0 1
        # op_idx 6: U -> 2 on qubit 0 (1 unleaked->leaked transition at op_idx 6)
        I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        # op_idx 7: 2 -> 3 on qubit 0 (leaked to higher leaked, must NOT record)
        I[LEAKAGE_TRANSITION_1: (1.0, 2-->3)] 0
        # op_idx 8: 3 -> 2 on qubit 0 (leaked to leaked, must NOT record)
        I[LEAKAGE_TRANSITION_1: (1.0, 3-->2)] 0
        # op_idx 9: 2 -> 0 on qubit 0 (seepage to unleaked, must NOT record)
        I[LEAKAGE_TRANSITION_1: (1.0, 2-->0)] 0
        # op_idx 10: (U, U) -> (2, 2) on (0, 1) (both qubits unleaked->leaked -> op_idx 10 appears TWICE)
        II[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_2)] 0 1
        # op_idx 11: (2, U) -> (3, 2) on (0, 2) (q0 is 2->3 NOT recorded, q2 is U->2 recorded ONCE -> op_idx 11 appears ONCE)
        II[LEAKAGE_TRANSITION_2: (1.0, 2_U-->3_2)] 0 2
        M 0 1 2
        """
    )
    unrolled = _unroll_circuit(circuit)
    assert unrolled[6].name == "I"
    assert unrolled[10].name == "II"
    assert unrolled[11].name == "II"
    expected = np.array([6, 10, 10, 11], dtype=np.int64)

    fss = _build_flip_sim(circuit, batch_size=3, seed=7)
    res_flip = fss.run()
    assert res_flip is not None and len(res_flip) == 3
    for arr in res_flip:
        assert arr.dtype == np.int64
        np.testing.assert_array_equal(arr, expected)

    for use_cpp in (True, False):
        css = _build_coset_sim(
            circuit, batch_size=3, seed=7, use_cpp_kernels=use_cpp
        )
        res_coset = css.run()
        assert res_coset is not None and len(res_coset) == 3
        for arr in res_coset:
            np.testing.assert_array_equal(arr, expected)

    for mode_kwargs in (
        {"use_cpp_kernels": True},
        {"use_cpp_kernels": False},
        {"sync_tableside_rng": True},
    ):
        tss = _build_tab_sim(circuit, seed=7, **mode_kwargs)
        res_tab = tss.run()
        assert res_tab is not None and len(res_tab) == 1
        np.testing.assert_array_equal(res_tab[0], expected)
        np.testing.assert_array_equal(
            tss.get_unleaked_to_leaked_op_indices()[0], expected
        )


def test_unrolled_instruction_indexing_with_repeat_blocks_and_metadata() -> None:
    """Verify that REPEAT blocks and QUBIT_COORDS / SHIFT_COORDS / TICK / DETECTOR
    produce the exact 0-based unrolled instruction index (`_unroll_circuit(circuit)`).
    """
    circuit = stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        R 0 1
        SHIFT_COORDS(0, 1)
        TICK
        REPEAT 3 {
            SHIFT_COORDS(0, 1)
            X 0
            I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0
            DETECTOR(0, 0) rec[-1]
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->0)] 0
            TICK
        }
        II[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_D)] 0 1
        M 0 1
        OBSERVABLE_INCLUDE(0) rec[-1]
        """
    )
    unrolled = _unroll_circuit(circuit)
    expected_indices = [
        idx
        for idx, op in enumerate(unrolled)
        if "U-->2" in op.tag or "U_U-->2_D" in op.tag
    ]
    assert len(expected_indices) == 4
    expected = np.array(expected_indices, dtype=np.int64)

    fss = _build_flip_sim(circuit, batch_size=2, seed=11)
    res_flip = fss.run()
    assert res_flip is not None
    for arr in res_flip:
        np.testing.assert_array_equal(arr, expected)

    for use_cpp in (True, False):
        css = _build_coset_sim(
            circuit, batch_size=2, seed=11, use_cpp_kernels=use_cpp
        )
        res_coset = css.run()
        assert res_coset is not None
        for arr in res_coset:
            np.testing.assert_array_equal(arr, expected)

    for mode_kwargs in (
        {"use_cpp_kernels": True},
        {"use_cpp_kernels": False},
        {"sync_tableside_rng": True},
    ):
        tss = _build_tab_sim(circuit, seed=11, **mode_kwargs)
        res_tab = tss.run()
        assert res_tab is not None
        np.testing.assert_array_equal(res_tab[0], expected)


def test_fused_mode_b_1_to_2_transition_unrolled_indexing() -> None:
    """Verify OP_FUSED_TRANS1_MEAS (1-->2 followed by M[LEAKAGE_PROJECTION_Z: (1.0, 2)])
    records the exact unrolled instruction index of the transition op across all
    TablesideSimulator tiers (C++ FastEngine, Python v2, sync_rng, fallback) and CosetsideSimulator.
    """
    circuit = stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        R 0 1
        REPEAT 3 {
            SHIFT_COORDS(0, 1)
            X 0
            I[LEAKAGE_TRANSITION_1: (1.0, 1-->2)] 0
            M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0
            DETECTOR(0, 0) rec[-1]
            I[LEAKAGE_TRANSITION_1: (1.0, 2-->0)] 0
            TICK
        }
        """
    )
    unrolled = _unroll_circuit(circuit)
    expected = np.array(
        [idx for idx, op in enumerate(unrolled) if "1-->2" in op.tag],
        dtype=np.int64,
    )
    assert len(expected) == 3

    for use_cpp in (True, False):
        css = _build_coset_sim(
            circuit, batch_size=2, seed=19, use_cpp_kernels=use_cpp
        )
        res_coset = css.run()
        assert res_coset is not None
        for arr in res_coset:
            np.testing.assert_array_equal(arr, expected)

    for mode_kwargs in (
        {"use_cpp_kernels": True},
        {"use_cpp_kernels": False},
        {"sync_tableside_rng": True},
    ):
        tss = _build_tab_sim(circuit, seed=19, **mode_kwargs)
        res_tab = tss.run()
        assert res_tab is not None
        np.testing.assert_array_equal(res_tab[0], expected)

    # Also explicitly test the pure `_do(self.circuit)` fallback path
    tss_fallback = _build_tab_sim(circuit, seed=19, use_cpp_kernels=False)
    tss_fallback._precompiled_circuit = None
    res_fallback = tss_fallback.run()
    assert res_fallback is not None
    np.testing.assert_array_equal(res_fallback[0], expected)


def test_exact_shot_by_shot_sync_rng_agreement_and_clear() -> None:
    """Verify exact shot-for-shot op_idx agreement across CosetsideSimulator
    and TablesideSimulator under sync_tableside_rng=True, and verify that
    .clear() properly resets the event lists across batches.
    """
    circuit = stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        R 0 1 2
        REPEAT 4 {
            SHIFT_COORDS(0, 1)
            X_ERROR(0.4) 0 1 2
            I[LEAKAGE_TRANSITION_1: (0.35, U-->2), (0.3, 2-->3), (0.5, 2-->0)] 0 1
            II[LEAKAGE_TRANSITION_2: (0.25, U_U-->2_2), (0.2, U_U-->2_D)] 1 2
            M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0 1 2
            TICK
        }
        """
    )
    batch_size = 12
    seed = 12345

    css = _build_coset_sim(
        circuit,
        batch_size=batch_size,
        seed=seed,
        sync_tableside_rng=True,
    )
    tss = _build_tab_sim(
        circuit,
        seed=seed,
        sync_tableside_rng=True,
    )

    for batch_iter in range(2):
        if batch_iter > 0:
            css.clear()
            assert all(len(a) == 0 for a in css.get_unleaked_to_leaked_records())

        coset_records = css.run()
        assert coset_records is not None

        tab_records: list[np.ndarray] = []
        for b in range(batch_size):
            if batch_iter > 0 or b > 0:
                tss.clear()
                assert len(tss.get_unleaked_to_leaked_records()[0]) == 0
            res_b = tss.run()
            assert res_b is not None
            tab_records.append(res_b[0])

        total_events = sum(len(r) for r in coset_records)
        assert total_events > 0
        for b in range(batch_size):
            np.testing.assert_array_equal(coset_records[b], tab_records[b])


def test_leakage_transition_z_in_flipside_simulator() -> None:
    circuit = stim.Circuit(
        """
        R 0 1
        X 0
        TICK
        I[LEAKAGE_TRANSITION_Z: (1.0, 1-->2), (0.0, 0-->2)] 0 1
        M 0 1
        """
    )
    fss = _build_flip_sim(circuit, batch_size=3, seed=99)
    res = fss.run()
    assert res is not None
    for arr in res:
        np.testing.assert_array_equal(arr, np.array([3], dtype=np.int64))
