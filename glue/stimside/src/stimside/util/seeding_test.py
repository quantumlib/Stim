"""Seeding of TablesideSimulator and CosetsideSimulator (see stimside.util.seeding).

Runs with different seeds must not share random streams and consecutive batches must not get
correlated seeds, while sync_tableside_rng keeps Tableside and Cosetside in lockstep shot for shot.
"""

import numpy as np
import pytest
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
    LeakageUint8 as LeakageUint8Tableau,
)
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_tableau import TablesideSimulator

_Q = " ".join(str(q) for q in range(12))
_RANDOM_12Q = stim.Circuit(f"R {_Q}\nH {_Q}\nI[LEAKAGE_TRANSITION_1: (0.3, U-->2)] {_Q}\nM {_Q}")

# name: (simulator, handler, batch size, kwargs, shots by which the stream of seed S + 1 led that of
# seed S when batch (sync: shot) n was seeded with S + n)
_PATHS = {
    "tab_cpp_bs1": (TablesideSimulator, LeakageUint8Tableau, 1, dict(use_cpp_kernels=True), 1),
    "tab_py_bs1": (TablesideSimulator, LeakageUint8Tableau, 1, dict(use_cpp_kernels=False), 1),
    "tab_cpp_bs4": (TablesideSimulator, LeakageUint8Tableau, 4, dict(use_cpp_kernels=True), 4),
    "tab_py_bs4": (TablesideSimulator, LeakageUint8Tableau, 4, dict(use_cpp_kernels=False), 4),
    "tab_sync_bs4": (TablesideSimulator, LeakageUint8Tableau, 4, dict(sync_tableside_rng=True), 1),
    "cos_cpp_bs4": (CosetsideSimulator, LeakageUint8Coset, 4, dict(use_cpp_kernels=True), 4),
    "cos_py_bs4": (CosetsideSimulator, LeakageUint8Coset, 4, dict(use_cpp_kernels=False), 4),
    "cos_sync_bs4": (CosetsideSimulator, LeakageUint8Coset, 4, dict(sync_tableside_rng=True), 1),
}


def _records(path, circuit, seed, num_batches, batch_size=None):
    """Final measurement records of num_batches batches (cleared in between), shots in run order."""
    sim_cls, handler_cls, bs, kwargs, _ = _PATHS[path]
    bs = bs if batch_size is None else batch_size
    handler = handler_cls().compile_op_handler(circuit=circuit, batch_size=bs)
    sim = sim_cls(circuit, compiled_op_handler=handler, batch_size=bs, seed=seed, **kwargs)
    out = []
    for k in range(num_batches):
        if k:
            sim.clear()
        sim.run()
        out.append(np.asarray(sim.get_final_measurement_records(), dtype=bool))
    return np.concatenate(out)


@pytest.mark.parametrize("path", list(_PATHS))
def test_adjacent_seeds_do_not_share_random_streams(path):
    _, _, bs, _, lead = _PATHS[path]
    num_batches = 20 if bs == 1 else 5
    a = _records(path, _RANDOM_12Q, 100, num_batches)
    b = _records(path, _RANDOM_12Q, 101, num_batches)
    same = np.all(a[lead:] == b[:-lead], axis=1)
    assert same.mean() < 0.5


def test_sync_shot_seeds_follow_the_shot_index_in_the_run():
    # However the run is split into batches, shot n is seeded from (seed, n) by both simulators.
    ref = _records("tab_sync_bs4", _RANDOM_12Q, 9, 1, batch_size=8)
    for path in ("tab_sync_bs4", "cos_sync_bs4"):
        for bs, num_batches in ((8, 1), (4, 2), (1, 8)):
            recs = _records(path, _RANDOM_12Q, 9, num_batches, batch_size=bs)
            np.testing.assert_array_equal(recs, ref, err_msg=f"{path} {bs}x{num_batches}")


@pytest.mark.parametrize("use_cpp", [True, False])
def test_cosetside_tie_break_bits_are_not_replayed(use_cpp):
    # Each pair's readouts come from a tie-break bit. With seed + n seeding, the 2nd bit of batch k
    # (or of seed S) was the 1st bit of batch k + 1 (or of seed S + 1).
    circuit = stim.Circuit(
        "R 0 1 2 3\nH 0 2\nCX 0 1 2 3\nI[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 1 3\nCX 0 1 2 3\n"
        "M 1 3\nM 0 2"
    )

    def make(seed):
        handler = LeakageUint8Coset().compile_op_handler(circuit=circuit, batch_size=1)
        return CosetsideSimulator(
            circuit, compiled_op_handler=handler, batch_size=1, seed=seed, use_cpp_kernels=use_cpp
        )

    def run(sim):
        sim.run()
        return np.asarray(sim.get_final_measurement_records(), dtype=bool)[0]

    sim = make(5)
    by_batch = []
    for k in range(200):
        if k:
            sim.clear()
        by_batch.append(run(sim))
    by_seed = [run(make(s)) for s in range(200)]
    for recs in (np.array(by_batch), np.array(by_seed)):
        agree = [np.mean(recs[1:, i] == recs[:-1, j]) for i in range(4) for j in range(4)]
        assert max(agree) < 0.75


def test_tableside_cpp_consecutive_batches_and_seeds_are_uncorrelated():
    # The C++ engine is seeded with the batch seed + 1000; for seeds seed + n its first draws were
    # correlated (lag-1 correlation of this leak bit about +0.2), across the batches of a run and
    # across runs with consecutive seeds (batch 0: n = 0 is hashed too).
    circuit = stim.Circuit(
        "I[LEAKAGE_TRANSITION_1: (0.5, U-->2)] 0\nMPAD[LEAKAGE_MEASUREMENT: (1.0, 2) : 0] 0"
    )

    def make(seed):
        handler = LeakageUint8Tableau().compile_op_handler(circuit=circuit, batch_size=1)
        sim = TablesideSimulator(
            circuit, compiled_op_handler=handler, batch_size=1, seed=seed, use_cpp_kernels=True
        )
        assert sim._cpp_runner is not None
        return sim

    def run(sim):
        sim.run()
        return sim.get_final_measurement_records()[0, 0]

    sim = make(0)
    by_batch = []
    for k in range(5000):
        if k:
            sim.clear()
        by_batch.append(run(sim))
    by_seed = [run(make(s)) for s in range(5000)]
    for leaked in (by_batch, by_seed):
        leaked_f = np.array(leaked, dtype=float)
        assert abs(np.corrcoef(leaked_f[:-1], leaked_f[1:])[0, 1]) < 0.08


@pytest.mark.parametrize("use_cpp", [True, False])
def test_unseeded_tableside_draws_63_bit_seeds(use_cpp):
    circuit = stim.Circuit("H 0\nI[LEAKAGE_TRANSITION_1: (0.3, U-->2)] 0\nM 0\nDETECTOR rec[-1]")
    handler = LeakageUint8Tableau().compile_op_handler(circuit=circuit, batch_size=4)
    sim = TablesideSimulator(
        circuit, compiled_op_handler=handler, batch_size=4, use_cpp_kernels=use_cpp
    )
    real, highs, seeds = sim._rng, [], [sim._tab_seed]

    class _Spy:  # records the upper bound of each integer drawn from sim._rng
        def __getattr__(self, name):
            return getattr(real, name)

        def integers(self, low, high=None, *args, **kwargs):
            highs.append(high)
            return real.integers(low, high, *args, **kwargs)

    sim._rng = _Spy()
    sim.clear()
    seeds.append(sim._tab_seed)
    sim.run()
    sim.get_final_measurement_records()  # draws the stim sampler's seed
    sim.clear()
    seeds.append(sim._tab_seed)
    sim.run()
    sim.get_detector_flips()  # draws the stim detector sampler's seed
    assert highs == [2**63] * 4
    assert min(seeds) >= 2**31  # fails with probability 3 * 2**-32


def test_unseeded_cosetside_sync_draws_63_bit_base_seeds():
    circuit = stim.Circuit("H 0\nM 0")
    handler = LeakageUint8Coset().compile_op_handler(circuit=circuit, batch_size=2)
    sim = CosetsideSimulator(
        circuit, compiled_op_handler=handler, batch_size=2, sync_tableside_rng=True
    )
    for _ in range(3):
        base = int(sim._shot_np_rngs[0].bit_generator.seed_seq.entropy)
        assert base >= 2**31  # fails with probability 2**-32
        sim.run()
        sim.clear()


def test_derived_seed():
    from stimside.util.seeding import derived_seed

    seeds = [derived_seed(s, n) for s in (0, 1, 2, 12345, 2**64 - 1) for n in range(50)]
    assert len(set(seeds)) == len(seeds)
    assert all(0 <= s < 2**63 for s in seeds)
    assert derived_seed(12345, 0) != 12345  # n = 0 is hashed too


@pytest.mark.parametrize("path", list(_PATHS))
def test_seed_must_be_an_integer_in_the_uint64_range(path):
    # As in stim, a seed is None or an integer in [0, 2**64); numpy integers work like ints.
    for bad in (-1, 2**64, np.int64(-1)):
        with pytest.raises(ValueError, match="seed must be"):
            _records(path, _RANDOM_12Q, bad, 2)
    for bad in (1.5, "5"):
        with pytest.raises(TypeError, match="cannot be interpreted as an integer"):
            _records(path, _RANDOM_12Q, bad, 2)
    ref = _records(path, _RANDOM_12Q, 5, 2)
    for seed in (np.int64(5), np.uint64(5)):
        np.testing.assert_array_equal(_records(path, _RANDOM_12Q, seed, 2), ref)
