"""SWAP[LEAKAGE_SWAP] swaps leakage states too; untagged SWAP never does. Every simulator and mode."""

import inspect

import numpy as np
import pymatching  # type: ignore[import-untyped]
import pytest
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.dem_generators import (
    BranchAndBoundDecoder,
    CallableDecoder,
    MarginalDecoder,
    MarginalLeakageDemGenerator,
)
from stimside.op_handlers.leakage_handlers import (
    leakage_tag_parsing_flip,
    leakage_tag_parsing_tableau,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    LeakageUint8 as LeakageUint8Flip,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
    LeakageUint8 as LeakageUint8Tableau,
)
from stimside.sampler_coset import CosetsideSampler
from stimside.sampler_flip import FlipsideSampler
from stimside.sampler_tableau import TablesideSampler
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_flip import FlipsideSimulator
from stimside.simulator_tableau import TablesideSimulator

# The B&B search needs per-shot edge_reweights / return_no_matching (PyMatching fork).
fork_only = pytest.mark.skipif(
    "return_no_matching" not in inspect.signature(pymatching.Matching.decode_batch).parameters,
    reason="needs the PyMatching version with per-shot edge_reweights",
)

PARSERS = [leakage_tag_parsing_flip.parse_leakage_tag, leakage_tag_parsing_tableau.parse_leakage_tag]

# (simulator, mode, batch size). Tableside "interactive" runs the generic per-op path.
PATHS = [
    ("tableside", "cpp", 1),
    ("tableside", "cpp", 16),
    ("tableside", "py", 1),
    ("tableside", "py", 16),
    ("tableside", "sync", 16),
    ("tableside", "interactive", 1),
    ("cosetside", "default", 64),
    ("cosetside", "sync", 64),
    ("flipside", "default", 256),
]
# Flipside has no unconditional_condition_on_U.
PATH_UC = [(p, uc) for p in PATHS for uc in (True, False) if p[0] != "flipside" or uc]
PATH_UC_IDS = [f"{s}-{m}-bs{b}-uc{uc}" for (s, m, b), uc in PATH_UC]


def _leak(qubit, state=2):
    return f"I[LEAKAGE_TRANSITION_1: (1.0, U-->{state})] {qubit}"


READ = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) (1.0, 3): 0 1 2] 0 0 0"  # flags leaked qubits 0 1 2
READ_2 = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 1 2] 0 0 0"  # flags state 2 only

# name: (circuit lines, records, leakage events as (op_index, qubit, old, new), op indices of
# unleaked-to-leaked transitions); the same in every shot. Measurements of leaked qubits are random,
# so these only measure unleaked qubits.
UNTAGGED_CASES = {
    "first_target_leaked": (["R 0 1 2", _leak(0), "SWAP 0 1", READ], [1, 0, 0], [(1, 0, 0, 2)], [1]),
    "second_target_leaked": (["R 0 1 2", _leak(1), "SWAP 0 1", READ], [0, 1, 0], [(1, 1, 0, 2)], [1]),
    "fused": (  # stim fuses the two SWAPs into SWAP 1 0 0 2
        ["R 0 1 2", _leak(1), "SWAP 1 0", "SWAP 0 2", READ], [0, 1, 0], [(1, 1, 0, 2)], [1]
    ),
    "both_leaked": (
        ["R 0 1 2", _leak(0, 2), _leak(1, 3), "SWAP 0 1", READ_2],
        [1, 0, 0],
        [(1, 0, 0, 2), (2, 1, 0, 3)],
        [1, 2],
    ),
}

TAGGED_CASES = {
    "no_leak": (["R 0 1", "X 1", "SWAP[LEAKAGE_SWAP] 0 1", "M 0 1"], [1, 0], [], []),
    # The leaked qubit gets the computational SWAP too: qubit 1's |1> moves to qubit 0.
    "leak_and_state_move": (
        ["R 0 1 2", _leak(0), "X 1", "SWAP[LEAKAGE_SWAP] 0 1", READ, "M 0"],
        [0, 1, 0, 1],
        [(1, 0, 0, 2), (3, 0, 2, 0), (3, 1, 0, 2)],
        [1, 3],  # the qubit that receives the leak counts as unleaked-to-leaked
    ),
    "leak_on_second_target": (
        ["R 0 1 2", _leak(1), "SWAP[LEAKAGE_SWAP] 0 1", READ],
        [1, 0, 0],
        [(1, 1, 0, 2), (2, 0, 0, 2), (2, 1, 2, 0)],
        [1, 2],
    ),
    "both_leaked": (
        ["R 0 1 2", _leak(0, 2), _leak(1, 3), "SWAP[LEAKAGE_SWAP] 0 1", READ_2],
        [0, 1, 0],
        [(1, 0, 0, 2), (2, 1, 0, 3), (3, 0, 2, 3), (3, 1, 3, 2)],
        [1, 2],
    ),
    # Pairs are swapped from left to right; qubit 1 ends unleaked, so it has no event.
    "fused_left_to_right": (
        ["R 0 1 2", _leak(0), "SWAP[LEAKAGE_SWAP] 0 1 1 2", READ],
        [0, 0, 1],
        [(1, 0, 0, 2), (2, 0, 2, 0), (2, 2, 0, 2)],
        [1, 2],
    ),
    "two_tagged_swaps_in_a_row": (
        ["R 0 1 2", _leak(0), "X 2", "SWAP[LEAKAGE_SWAP] 0 1", "SWAP[LEAKAGE_SWAP] 2 0", READ, "M 0"],
        [0, 1, 0, 1],
        [(1, 0, 0, 2), (3, 0, 2, 0), (3, 1, 0, 2)],
        [1, 3],
    ),
    "untagged_then_tagged": (
        ["R 0 1 2", _leak(0), "SWAP 0 1", "SWAP[LEAKAGE_SWAP] 1 2", READ],
        [1, 0, 0],
        [(1, 0, 0, 2)],
        [1],
    ),
    "other_qubit_leaked": (
        ["R 0 1 2", _leak(2), "X 1", "SWAP[LEAKAGE_SWAP] 0 1", READ, "M 0 1"],
        [0, 0, 1, 1, 0],
        [(1, 2, 0, 2)],
        [1],
    ),
    # The fast Tableside runners apply this transition from their own state buffer, which the
    # handler must therefore update in place.
    "transition_after_move": (
        [
            "R 0 1 2",
            _leak(0),
            "SWAP[LEAKAGE_SWAP] 0 1",
            "I[LEAKAGE_TRANSITION_1: (1.0, 2-->3)] 1",
            "MPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0 1 2] 0 0 0",
        ],
        [0, 1, 0],
        [(1, 0, 0, 2), (2, 0, 2, 0), (2, 1, 0, 2), (3, 1, 2, 3)],
        [1, 2],
    ),
    # The leak goes 0 -> 2 -> 1 -> 0, and every move counts as unleaked-to-leaked.
    "repeat_block": (
        ["R 0 1 2", _leak(0), "REPEAT 3 {", "SWAP[LEAKAGE_SWAP] 0 1 1 2", "}", READ],
        [1, 0, 0],
        [
            (1, 0, 0, 2),
            (2, 0, 2, 0),
            (2, 2, 0, 2),
            (3, 1, 0, 2),
            (3, 2, 2, 0),
            (4, 0, 0, 2),
            (4, 1, 2, 0),
        ],
        [1, 2, 3, 4],
    ),
}


def _run(path, circuit, uc=True, seed=5):
    """Runs `circuit` on one path.

    Returns (records, each shot's leakage events as tuples (None for Flipside), each shot's
    unleaked-to-leaked op indices).
    """
    sim_name, mode, batch_size = path
    if sim_name == "flipside":
        coh = LeakageUint8Flip().compile_op_handler(circuit=circuit, batch_size=batch_size)
        sim = FlipsideSimulator(
            circuit,
            compiled_op_handler=coh,
            batch_size=batch_size,
            seed=seed,
            record_unleaked_to_leaked=True,
        )
        sim.run()
    elif sim_name == "cosetside":
        coh = LeakageUint8Coset(unconditional_condition_on_U=uc).compile_op_handler(
            circuit=circuit, batch_size=batch_size
        )
        sim = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=coh,
            batch_size=batch_size,
            seed=seed,
            sync_tableside_rng=mode == "sync",
            record_unleaked_to_leaked=True,
            record_leakage_events=True,
        )
        sim.run()
    else:
        coh = LeakageUint8Tableau(unconditional_condition_on_U=uc).compile_op_handler(
            circuit=circuit, batch_size=1
        )
        sim = TablesideSimulator(
            circuit,
            compiled_op_handler=coh,
            batch_size=batch_size,
            seed=seed,
            use_cpp_kernels=mode == "cpp",
            sync_tableside_rng=mode == "sync",
            record_unleaked_to_leaked=True,
            record_leakage_events=True,
        )
        if mode == "cpp":
            assert sim._cpp_runner is not None, "the C++ kernels did not load"
        if mode == "interactive":
            sim.interactive_do(circuit)
            sim.finish_interactive_run()
        else:
            sim.run()
    records = np.asarray(sim.get_final_measurement_records(), dtype=bool)
    assert records.shape == (batch_size, circuit.num_measurements)
    events = (
        None
        if sim_name == "flipside"
        else [[tuple(e) for e in shot] for shot in sim.get_leakage_events()]
    )
    u2l = [shot.tolist() for shot in sim.get_unleaked_to_leaked_records()]
    return records, events, u2l


def _mismatch(path, uc, name, case):
    """None if every shot of `case` on `path` has the expected records, events and u2l."""
    lines, want_records, want_events, want_u2l = case
    records, events, u2l = _run(path, stim.Circuit("\n".join(lines)), uc)
    shots = len(records)
    if not np.array_equal(records, np.tile(np.array(want_records, dtype=bool), (shots, 1))):
        return f"{name}: records {records.astype(int).tolist()[:2]}..., expected {want_records}"
    if events is not None and events != [want_events] * shots:
        return f"{name}: events {events[:2]}..., expected {want_events}"
    if u2l != [want_u2l] * shots:
        return f"{name}: u2l {u2l[:2]}..., expected {want_u2l}"
    return None


# ##############################################################################
# Parsing
# ##############################################################################


def test_parse_leakage_swap_tag():
    from stimside.op_handlers.leakage_handlers.leakage_parameters import LeakageSwapParams

    want = LeakageSwapParams(from_tag="LEAKAGE_SWAP")
    assert want.name == "LEAKAGE_SWAP"
    for parse in PARSERS:
        assert parse(stim.Circuit("SWAP[LEAKAGE_SWAP] 0 1")[0]) == want
        assert parse(stim.Circuit("SWAP[LEAKAGE_SWAP] 0 1 2 3")[0]) == want
        assert parse(stim.Circuit("SWAP 0 1")[0]) is None


@pytest.mark.parametrize(
    "text, match",
    [
        ("SWAP[LEAKAGE_SWAP: (0.5)] 0 1", "LEAKAGE_SWAP takes no arguments"),
        ("SWAP[LEAKAGE_SWAP:x] 0 1", "LEAKAGE_SWAP takes no arguments"),
        ("SWAP[LEAKAGE_SWAP:] 0 1", "Malformed leakage tag structure"),
        ("SWAP[LEAKAGE_SWAP ] 0 1", "Malformed leakage tag structure"),
        ("SWAP[LEAKAGE_SWAPS] 0 1", "Malformed leakage tag structure"),
        ("CX[LEAKAGE_SWAP] 0 1", "must be attached to one of"),
        ("ISWAP[LEAKAGE_SWAP] 0 1", "must be attached to one of"),
        ("CXSWAP[LEAKAGE_SWAP] 0 1", "must be attached to one of"),
        ("II[LEAKAGE_SWAP] 0 1", "must be attached to one of"),
        ("H[LEAKAGE_SWAP] 0", "not-2Q stim gate"),
        ("I[LEAKAGE_SWAP] 0", "not-2Q stim gate"),
    ],
)
def test_bad_leakage_swap_tags_raise(text, match):
    op = stim.Circuit(text)[0]
    for parse in PARSERS:
        with pytest.raises(ValueError, match=match):
            parse(op)


@pytest.mark.parametrize("handler", [LeakageUint8Tableau, LeakageUint8Coset, LeakageUint8Flip])
def test_compiling_bad_leakage_swap_raises(handler):
    for text in ("CX[LEAKAGE_SWAP] 0 1", "SWAP[LEAKAGE_SWAP: (0.5)] 0 1"):
        with pytest.raises(ValueError, match="LEAKAGE_SWAP"):
            handler().compile_op_handler(circuit=stim.Circuit(text), batch_size=1)


# ##############################################################################
# Simulation
# ##############################################################################


@pytest.mark.parametrize("path, uc", PATH_UC, ids=PATH_UC_IDS)
def test_untagged_swap_never_moves_leakage(path, uc):
    bad = [_mismatch(path, uc, n, c) for n, c in UNTAGGED_CASES.items()]
    assert [m for m in bad if m] == []


@pytest.mark.parametrize("path, uc", PATH_UC, ids=PATH_UC_IDS)
def test_leakage_swap_moves_leakage_with_the_qubit(path, uc):
    bad = [_mismatch(path, uc, n, c) for n, c in TAGGED_CASES.items()]
    assert [m for m in bad if m] == []


# Records: is-2 and is-3 flags of qubits 0, 1 before the swap, then after it.
RANDOM_LEAKS = stim.Circuit(
    "R 0 1\n"
    "I[LEAKAGE_TRANSITION_1: (0.5, U-->2) (0.2, U-->3)] 0 1\n"
    "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 1] 0 0\n"
    "MPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0 1] 0 0\n"
    "SWAP[LEAKAGE_SWAP] 0 1\n"
    "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 1] 0 0\n"
    "MPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0 1] 0 0"
)


@pytest.mark.parametrize("path, uc", PATH_UC, ids=PATH_UC_IDS)
def test_leakage_swap_moves_random_leaks_shot_by_shot(path, uc):
    asymmetric_shots = 0
    for seed in (1, 2, 3):
        records, _, _ = _run(path, RANDOM_LEAKS, uc, seed=seed)
        before, after = records[:, :4], records[:, 4:]
        np.testing.assert_array_equal(after, before[:, [1, 0, 3, 2]])
        asymmetric_shots += int(np.count_nonzero(before[:, 0] != before[:, 1]))
    # Without sync, the shots of a Tableside batch share one leakage trajectory.
    if path[0] != "tableside" or path[1] == "sync":
        assert asymmetric_shots > 0


LOCKSTEP_CIRCUIT = stim.Circuit(
    """
    R 0 1 2 3
    H 0 2
    CX 0 1
    I[LEAKAGE_TRANSITION_1: (0.3, U-->2) (0.1, U-->3)] 0 1 2
    DEPOLARIZE1(0.1) 0 1 2 3
    SWAP[LEAKAGE_SWAP] 0 3 1 2
    CX 3 2
    II[LEAKAGE_TRANSITION_2: (0.2, 2_U-->U_2)] 3 0
    SWAP[LEAKAGE_SWAP] 2 0
    X_ERROR(0.2) 0 1 2 3
    MPAD[LEAKAGE_MEASUREMENT: (0.9, 2) (0.8, 3) (0.05, 0) (0.05, 1): 0 1 2 3] 0 0 0 0
    M 0 1 2 3
    """
)


@pytest.mark.parametrize("uc", [True, False])
def test_leakage_swap_keeps_tableside_and_cosetside_sync_in_lockstep(uc):
    swap_events = 0
    for seed in range(1, 6):
        tab = _run(("tableside", "sync", 16), LOCKSTEP_CIRCUIT, uc, seed=seed)
        cos = _run(("cosetside", "sync", 16), LOCKSTEP_CIRCUIT, uc, seed=seed)
        np.testing.assert_array_equal(tab[0], cos[0])
        assert tab[1] == cos[1]
        assert tab[2] == cos[2]
        swap_events += sum(e[0] in (5, 8) for shot in tab[1] for e in shot)
    assert swap_events > 0


@pytest.mark.parametrize("mode", ["cpp", "py", "sync"])
@pytest.mark.parametrize("uc", [True, False])
def test_tableside_runs_leakage_swap_on_its_fast_paths(monkeypatch, mode, uc):
    def generic_path(self, this):
        raise AssertionError("TablesideSimulator.run() took the generic per-op path")

    monkeypatch.setattr(TablesideSimulator, "_do", generic_path)
    for name in ("leak_and_state_move", "other_qubit_leaked", "no_leak"):
        assert _mismatch(("tableside", mode, 16), uc, name, TAGGED_CASES[name]) is None


def _predict_no_flip(circuit, bit_packed_dets, records=None):
    return np.zeros((bit_packed_dets.shape[0], 1), dtype=np.uint8)


@pytest.mark.parametrize(
    "sampler_cls, handler",
    [
        (TablesideSampler, LeakageUint8Tableau),
        (CosetsideSampler, LeakageUint8Coset),
        (FlipsideSampler, LeakageUint8Flip),
    ],
)
def test_samplers_run_leakage_swap(sampler_cls, handler):
    # The leak moves to qubit 1, so its flag (0 in the reference) flips the observable every shot.
    circuit = stim.Circuit(
        "R 0 1\n"
        "I[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0\n"
        "SWAP[LEAKAGE_SWAP] 0 1\n"
        "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 1] 0\n"
        "DETECTOR rec[-1]\n"
        "OBSERVABLE_INCLUDE(0) rec[-1]"
    )
    sampler = sampler_cls(handler(), CallableDecoder(_predict_no_flip), batch_size=8, seed=1)
    stats = sampler.compiled_sampler_for_task(sinter.Task(circuit=circuit)).sample(16)
    assert stats.shots == 16
    assert stats.errors == 16


# ##############################################################################
# DEM generation and decoders: not supported
# ##############################################################################

DEM_CIRCUIT = stim.Circuit(
    "R 0 1\n"
    "X_ERROR(0.1) 0 1\n"
    "I[LEAKAGE_TRANSITION_1: (0.1, U-->2)] 0\n"
    "SWAP[LEAKAGE_SWAP] 0 1\n"
    "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 1] 0 0\n"
    "M 0 1\n"
    "DETECTOR rec[-1]\n"
    "DETECTOR rec[-2]\n"
    "OBSERVABLE_INCLUDE(0) rec[-1]"
)


@pytest.mark.parametrize("uc", [True, False])
def test_marginal_dem_generator_and_decoder_reject_leakage_swap(uc):
    records = np.zeros((2, DEM_CIRCUIT.num_measurements), dtype=bool)
    with pytest.raises(NotImplementedError, match="LEAKAGE_SWAP"):
        MarginalLeakageDemGenerator(uc).base_dem(DEM_CIRCUIT)
    with pytest.raises(NotImplementedError, match="LEAKAGE_SWAP"):
        MarginalLeakageDemGenerator(uc, decompose_errors=True)(DEM_CIRCUIT, records)
    with pytest.raises(NotImplementedError, match="LEAKAGE_SWAP"):
        MarginalLeakageDemGenerator(uc, loss_oracle=True)(
            DEM_CIRCUIT, records, leakage_events=[[], []]
        )
    compiled = MarginalDecoder(MarginalLeakageDemGenerator(uc)).compile_for_task(
        sinter.Task(circuit=DEM_CIRCUIT)
    )
    with pytest.raises(NotImplementedError, match="LEAKAGE_SWAP"):
        compiled.decode_shots_bit_packed(
            bit_packed_detection_event_data=np.zeros((2, 1), dtype=np.uint8), records=records
        )


@pytest.mark.parametrize(
    "sampler_cls, handler",
    [
        (TablesideSampler, LeakageUint8Tableau),
        (CosetsideSampler, LeakageUint8Coset),
        (FlipsideSampler, LeakageUint8Flip),
    ],
)
def test_samplers_with_marginal_decoder_reject_leakage_swap(sampler_cls, handler):
    sampler = sampler_cls(handler(), MarginalDecoder(), batch_size=8, seed=1)
    compiled = sampler.compiled_sampler_for_task(sinter.Task(circuit=DEM_CIRCUIT))
    with pytest.raises(NotImplementedError, match="LEAKAGE_SWAP"):
        compiled.sample(8)


@fork_only
@pytest.mark.parametrize("uc", [True, False])
def test_bnb_decoder_rejects_leakage_swap(uc):
    with pytest.raises(NotImplementedError, match="LEAKAGE_SWAP"):
        BranchAndBoundDecoder(MarginalLeakageDemGenerator(uc)).compile_for_task(
            sinter.Task(circuit=DEM_CIRCUIT)
        )
