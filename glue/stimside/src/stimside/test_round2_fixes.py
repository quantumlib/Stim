import os
import numpy as np
import pytest
import sinter
import stim

from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import (
    LeakageUint8Coset,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    LeakageUint8 as LeakageUint8Flip,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
    LeakageUint8 as LeakageUint8Tableau,
)
from stimside.op_handlers.leakage_handlers.tag_registry import (
    parse_leakage_tag,
)
from stimside.sampler_coset import CosetsideSampler
from stimside.sampler_flip import FlipsideSampler
from stimside.sampler_tableau import TablesideSampler
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_flip import FlipsideSimulator
from stimside.simulator_tableau import TablesideSimulator


def _run_sim(sim_cls, handler_cls, circuit, *, seed=42, batch_size=4, sync=True):
    handler = handler_cls()
    compiled = handler.compile_op_handler(circuit=circuit, batch_size=batch_size)
    sim = sim_cls(
        circuit,
        compiled_op_handler=compiled,
        seed=seed,
        batch_size=batch_size,
        record_unleaked_to_leaked=True,
        **({"sync_tableside_rng": True} if sync else {}),
    )
    unleaked_to_leaked = sim.run()
    rec = sim.get_final_measurement_records()
    det = sim.get_detector_flips()
    obs = sim.get_observable_flips()
    if sim_cls is FlipsideSimulator:
        det = det.T
        obs = obs.T
    return sim, rec, det, obs, unleaked_to_leaked


def test_1a_tag_registry_widened_leakage_projection_z():
    allowed = ("M", "MZ", "MR", "MRZ", "MX", "MY", "MRX", "MRY")
    for g in allowed:
        c = stim.Circuit(
            f"{g}[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.8, 2)] 0"
        )
        for sim_kind in ("flip", "tableau"):
            parsed = parse_leakage_tag(c[0], simulator=sim_kind)
            assert parsed is not None

    for bad_gate in ("R", "H", "CX", "MPAD"):
        c = stim.Circuit(
            f"{bad_gate}[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.8, 2)] 0 1"
        )
        for sim_kind in ("flip", "tableau"):
            with pytest.raises(ValueError):
                parse_leakage_tag(c[0], simulator=sim_kind)


def test_1a_leakage_projection_z_all_bases_and_mpad():
    circuit = stim.Circuit(
        """
        R 0 1 2 3
        I[LEAKAGE_TRANSITION_1: (0.5, U-->2)] 0 1 2 3
        H 0
        H_YZ 1
        X 2
        MX[LEAKAGE_PROJECTION_Z: (0.1, 0) (0.9, 1) (0.75, 2)] 0 !1
        MY[LEAKAGE_PROJECTION_Z: (0.1, 0) (0.9, 1) (0.75, 2)] 1
        MR[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.8, 2)] 2 !3
        MRX[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.8, 2)] 0
        MRY[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.8, 2)] 1
        MPAD 0 1 1 0
        MPAD(0.25) 0 1 1
        MPAD[LEAKAGE_MEASUREMENT: (0.9, 2) : 0 2] 0 1
        MPAD[LEAKAGE_MEASUREMENT: (0.1, 0) (0.8, 1) (0.95, 2) : 1 3] 1 0
        M 0 1 2 3
        DETECTOR rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-3] rec[-5]
        """
    )
    _, t_rec, t_det, t_obs, t_ev = _run_sim(
        TablesideSimulator, LeakageUint8Tableau, circuit, seed=101, batch_size=8
    )
    _, c_rec, c_det, c_obs, c_ev = _run_sim(
        CosetsideSimulator, LeakageUint8Coset, circuit, seed=101, batch_size=8
    )

    np.testing.assert_array_equal(t_rec, c_rec)
    np.testing.assert_array_equal(t_det, c_det)
    np.testing.assert_array_equal(t_obs, c_obs)
    for b in range(8):
        np.testing.assert_array_equal(t_ev[b], c_ev[b])


def test_1b_heralded_and_product_measurements_with_leakage():
    circuit = stim.Circuit(
        """
        R 0 1 2 3
        H 0 1
        CX 0 2 1 3
        HERALDED_ERASE(0.3) 0 !1
        HERALDED_PAULI_CHANNEL_1(0.05, 0.1, 0.1, 0.15) 2 !3
        I[LEAKAGE_TRANSITION_1: (0.4, U-->2)] 0 2
        MXX(0.1) 0 1 !2 3
        MYY(0.1) 0 !1 2 3
        MZZ(0.1) 0 2 !1 3
        MPP(0.15) X0*X1 !Y1*Z2 Z0*X2*Y3
        M[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.85, 2)] 0 1 2 3
        DETECTOR rec[-1] rec[-5]
        OBSERVABLE_INCLUDE(0) rec[-2] rec[-6]
        """
    )
    _, t_rec, t_det, t_obs, _ = _run_sim(
        TablesideSimulator, LeakageUint8Tableau, circuit, seed=202, batch_size=8
    )
    _, c_rec, c_det, c_obs, _ = _run_sim(
        CosetsideSimulator, LeakageUint8Coset, circuit, seed=202, batch_size=8
    )

    assert t_rec.shape == (8, 17)
    np.testing.assert_array_equal(t_rec, c_rec)
    np.testing.assert_array_equal(t_det, c_det)
    np.testing.assert_array_equal(t_obs, c_obs)


def test_3_sampler_distinct_seeds_across_compiles_and_workers(monkeypatch):
    for sampler_cls, handler_cls in (
        (FlipsideSampler, LeakageUint8Flip),
        (TablesideSampler, LeakageUint8Tableau),
        (CosetsideSampler, LeakageUint8Coset),
    ):
        sampler = sampler_cls(op_handler=handler_cls(), seed=42, batch_size=4)
        s0 = sampler._next_compiled_seed()
        s1 = sampler._next_compiled_seed()
        assert s0 == 42
        assert s1 is not None and s1 != 42

        sampler_w = sampler_cls(op_handler=handler_cls(), seed=42, batch_size=4)
        orig_pid = os.getpid()
        monkeypatch.setattr(os, "getpid", lambda: orig_pid + 99)
        s_worker = sampler_w._next_compiled_seed()
        assert s_worker is not None and s_worker != 42 and s_worker != s1
        monkeypatch.setattr(os, "getpid", lambda: orig_pid)


def test_6_tableside_batch_size_greater_than_one_and_sampler():
    circuit = stim.Circuit(
        """
        R 0 1
        H 0
        CX 0 1
        I[LEAKAGE_TRANSITION_1: (0.5, U-->2)] 0
        M[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.8, 2)] 0 1
        DETECTOR rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
        """
    )
    # Non-synced batch_size > 1
    _, t_rec, t_det, t_obs, t_ev = _run_sim(
        TablesideSimulator,
        LeakageUint8Tableau,
        circuit,
        seed=77,
        batch_size=6,
        sync=False,
    )
    assert t_rec.shape == (6, 2)
    assert t_det.shape == (6, 1)
    assert t_obs.shape == (6, 1)
    assert len(t_ev) == 6

    # TablesideSampler with batch_size > 1
    sampler = TablesideSampler(
        op_handler=LeakageUint8Tableau(),
        seed=77,
        batch_size=4,
        decoder="vacuous",
    )
    compiled = sampler.compiled_sampler_for_task(sinter.Task(circuit=circuit))
    stats = compiled.sample(8)
    assert stats.shots == 8


def test_7_end_to_end_2way_shot_for_shot_equivalence():
    circuit = stim.Circuit(
        """
        R 0 1 2 3
        RX 1
        RY 2
        I[LEAKAGE_TRANSITION_1: (0.35, U-->2)] 0 1 2
        HERALDED_ERASE(0.2) 0 1
        HERALDED_PAULI_CHANNEL_1(0.05, 0.05, 0.05, 0.05) 2 3
        MXX(0.05) 0 1 2 3
        MPP(0.05) X0*Z1 Y2*X3
        MRX[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.8, 2)] 0 !1
        MRY[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.8, 2)] 2 !3
        I[LEAKAGE_TRANSITION_1: (0.6, 2-->U)] 0 1 2 3
        R 0 1 2 3
        MPAD(0.1) 0 1 1
        MPAD[LEAKAGE_MEASUREMENT: (0.9, 2) : 0 1] 0 1
        M[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.95, 1) (0.85, 2)] 0 1 2 3
        DETECTOR rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-3] rec[-4]
        """
    )
    _, t_rec, t_det, t_obs, t_ev = _run_sim(
        TablesideSimulator, LeakageUint8Tableau, circuit, seed=303, batch_size=10
    )
    _, c_rec, c_det, c_obs, c_ev = _run_sim(
        CosetsideSimulator, LeakageUint8Coset, circuit, seed=303, batch_size=10
    )

    np.testing.assert_array_equal(t_rec, c_rec)
    np.testing.assert_array_equal(t_det, c_det)
    np.testing.assert_array_equal(t_obs, c_obs)
    for b in range(10):
        np.testing.assert_array_equal(t_ev[b], c_ev[b])


def test_non_flip_compatible_x_y_reset_leakage_projection_z_equivalence():
    circuit = stim.Circuit(
        """
        R 0 1 2
        H 0 1
        X_ERROR(0.3) 0 1 2
        I[LEAKAGE_TRANSITION_1: (0.25, 0-->2) (0.45, 1-->2)] 0 1 2
        MX[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.92, 1) (0.7, 2)] 0 !1
        MY[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.92, 1) (0.7, 2)] 0 1
        MR[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.92, 1) (0.7, 2)] 0 !1 2
        MRX[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.92, 1) (0.7, 2)] 0 !1 2
        MRY[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.92, 1) (0.7, 2)] 0 1 2
        I[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 1 2
        MX[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.92, 1) (0.7, 2)] 0 1 2
        MR[LEAKAGE_PROJECTION_Z: (0.05, 0) (0.92, 1) (0.7, 2)] 0 1 2
        DETECTOR rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-3] rec[-4]
        """
    )
    _, t_rec, t_det, t_obs, t_ev = _run_sim(
        TablesideSimulator, LeakageUint8Tableau, circuit, seed=99, batch_size=24
    )
    _, c_rec, c_det, c_obs, c_ev = _run_sim(
        CosetsideSimulator, LeakageUint8Coset, circuit, seed=99, batch_size=24
    )
    np.testing.assert_array_equal(t_rec, c_rec)
    np.testing.assert_array_equal(t_det, c_det)
    np.testing.assert_array_equal(t_obs, c_obs)
    for b in range(24):
        np.testing.assert_array_equal(t_ev[b], c_ev[b])


def test_noiseless_leakage_projection_z_sync_all_bases():
    circuit = stim.Circuit(
        """
        R 0 1 2 3
        H 0
        I[LEAKAGE_TRANSITION_1: (0.3, U-->2)] 0 1 2 3
        MR[LEAKAGE_PROJECTION_Z: (0.8, 2)] 0
        MX[LEAKAGE_PROJECTION_Z: (0.8, 2)] 1
        MY[LEAKAGE_PROJECTION_Z: (0.8, 2)] 2
        MRX[LEAKAGE_PROJECTION_Z: (0.8, 2)] 3
        MRY[LEAKAGE_PROJECTION_Z: (0.8, 2)] 1
        MRZ[LEAKAGE_PROJECTION_Z: (0.8, 2)] 2
        I[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 1 2 3
        M 0 1 2 3
        DETECTOR rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-3] rec[-4]
        """
    )
    _, t_rec, t_det, t_obs, t_ev = _run_sim(
        TablesideSimulator, LeakageUint8Tableau, circuit, seed=12, batch_size=16
    )
    _, c_rec, c_det, c_obs, c_ev = _run_sim(
        CosetsideSimulator, LeakageUint8Coset, circuit, seed=12, batch_size=16
    )
    np.testing.assert_array_equal(t_rec, c_rec)
    np.testing.assert_array_equal(t_det, c_det)
    np.testing.assert_array_equal(t_obs, c_obs)
    for b in range(16):
        np.testing.assert_array_equal(t_ev[b], c_ev[b])


def test_leaked_targets_reset_in_target_order_sync():
    circuit = stim.Circuit(
        """
        R 0 1 2 3 4 5 6 7
        H 0 1 4 5
        CX 0 2 1 3 4 6 5 7
        I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 1 4 5
        M[LEAKAGE_PROJECTION_Z: (0.8, 2)] 1 0
        MX[LEAKAGE_PROJECTION_Z: (0.1, 0) (0.9, 1) (0.8, 2)] 5 4
        M 2 3
        MX 6 7
        """
    )
    _, t_rec, _, _, t_ev = _run_sim(
        TablesideSimulator, LeakageUint8Tableau, circuit, seed=12, batch_size=16
    )
    _, c_rec, _, _, c_ev = _run_sim(
        CosetsideSimulator, LeakageUint8Coset, circuit, seed=12, batch_size=16
    )
    np.testing.assert_array_equal(t_rec, c_rec)
    for b in range(16):
        np.testing.assert_array_equal(t_ev[b], c_ev[b])


def test_duplicate_leaked_target_leakage_projection_z_sync_raises():
    circuit = stim.Circuit(
        """
        R 0
        I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        M[LEAKAGE_PROJECTION_Z: (1.0, 2)] 0 0
        """
    )
    with pytest.raises(NotImplementedError):
        _run_sim(CosetsideSimulator, LeakageUint8Coset, circuit, sync=True)


def test_repeated_targets_in_leakage_projection_z_and_mr():
    c_sync = stim.Circuit(
        """
        R 0
        X_ERROR(1.0) 0
        MR[LEAKAGE_PROJECTION_Z: (0.0, 0) (1.0, 1) (0.8, 2)] 0 0
        X_ERROR(1.0) 0
        MR(0.1) 0 0
        MX[LEAKAGE_PROJECTION_Z: (0.0, 0) (1.0, 1) (0.8, 2)] 0 0
        DETECTOR rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-5] rec[-6]
        """
    )
    _, t_rec, t_det, t_obs, _ = _run_sim(
        TablesideSimulator, LeakageUint8Tableau, c_sync, seed=12, batch_size=16, sync=True
    )
    _, c_rec, c_det, c_obs, _ = _run_sim(
        CosetsideSimulator, LeakageUint8Coset, c_sync, seed=12, batch_size=16, sync=True
    )
    np.testing.assert_array_equal(t_rec, c_rec)
    np.testing.assert_array_equal(t_det, c_det)
    np.testing.assert_array_equal(t_obs, c_obs)

    c_nosync = stim.Circuit(
        """
        R 0
        X 0
        MR[LEAKAGE_PROJECTION_Z: (0.0, 0) (1.0, 1) (0.8, 2)] 0 0
        H 0
        Z 0
        MX[LEAKAGE_PROJECTION_Z: (0.0, 0) (1.0, 1) (0.8, 2)] 0 0
        DETECTOR rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-3] rec[-4]
        """
    )
    for sim_cls, h_cls in [
        (TablesideSimulator, LeakageUint8Tableau),
        (CosetsideSimulator, LeakageUint8Coset),
        (FlipsideSimulator, LeakageUint8Flip),
    ]:
        _, rec, _, _, _ = _run_sim(
            sim_cls, h_cls, c_nosync, seed=12, batch_size=4, sync=False
        )
        for b in range(4):
            assert rec[b].tolist() == [True, False, True, True]

