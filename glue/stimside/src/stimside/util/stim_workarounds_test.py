"""Tests of the stim.FlipSimulator.broadcast_pauli_errors workaround (partial 64-shot words) and of
stim-fused MPAD[LEAKAGE_MEASUREMENT] / CONDITIONED_ON_OTHER instructions."""

import numpy as np
import pytest
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import LeakageUint8 as LeakageUint8Flip
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import (
    LeakageUint8 as LeakageUint8Tableau,
)
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_flip import FlipsideSimulator
from stimside.simulator_tableau import TablesideSimulator
from stimside.util.stim_workarounds import broadcast_pauli_errors, split_fused_instruction


def _x_flips(sim: stim.FlipSimulator) -> np.ndarray:
    """(num_qubits, batch_size) bool array of X components of the Pauli frames."""
    xs, _, _, _, _ = sim.to_numpy(output_xs=True)
    return xs


def _trailing(bs: int) -> slice:
    """The shots of the trailing partial 64-shot word (all shots when bs < 64)."""
    return slice((bs // 64) * 64, bs)


@pytest.mark.parametrize("bs", [1, 8, 63, 65, 100])
@pytest.mark.parametrize("p", [0.5, 0.2])
def test_helper_applies_rate_p_in_the_trailing_word(bs, p):
    num_qubits = 8
    reps = -(-4000 // (num_qubits * (bs % 64)))  # >= 4000 trailing samples
    rng = np.random.default_rng(5)
    hits = []
    for r in range(reps):
        sim = stim.FlipSimulator(
            batch_size=bs, num_qubits=num_qubits, disable_stabilizer_randomization=True, seed=r
        )
        broadcast_pauli_errors(
            sim, pauli="X", mask=np.ones((num_qubits, bs), dtype=np.bool_), p=p, np_rng=rng
        )
        hits.append(_x_flips(sim)[:, _trailing(bs)])
    rate = np.concatenate(hits, axis=1).mean()
    sigma = np.sqrt(p * (1 - p) / (num_qubits * (bs % 64) * reps))
    assert abs(rate - p) < 6 * sigma, (rate, p)


def test_helper_only_hits_masked_entries():
    bs = 100
    sim = stim.FlipSimulator(batch_size=bs, num_qubits=4, disable_stabilizer_randomization=True)
    mask = np.zeros((4, bs), dtype=np.bool_)
    mask[1, ::3] = True
    mask[3, 70:] = True
    broadcast_pauli_errors(sim, pauli="Y", mask=mask, p=0.5, np_rng=np.random.default_rng(0))
    xs, zs, _, _, _ = sim.to_numpy(output_xs=True, output_zs=True)
    assert not (xs & ~mask).any()
    assert np.array_equal(xs, zs)  # Y flips both
    assert xs[mask].any() and not xs[mask].all()


@pytest.mark.parametrize("bs", [1, 65, 128])
@pytest.mark.parametrize("p", [0.0, 1.0])
def test_helper_p0_and_p1_match_stim_and_draw_nothing(bs, p):
    mask = np.random.default_rng(1).random((5, bs)) < 0.5
    a = stim.FlipSimulator(batch_size=bs, num_qubits=5, disable_stabilizer_randomization=True, seed=3)
    b = stim.FlipSimulator(batch_size=bs, num_qubits=5, disable_stabilizer_randomization=True, seed=3)
    rng = np.random.default_rng(7)
    state = rng.bit_generator.state
    a.broadcast_pauli_errors(pauli="X", mask=mask, p=p)
    broadcast_pauli_errors(b, pauli="X", mask=mask, p=p, np_rng=rng)
    assert np.array_equal(_x_flips(a), _x_flips(b))
    if p == 1.0:
        assert np.array_equal(_x_flips(b), mask)
    assert rng.bit_generator.state == state


@pytest.mark.parametrize("bs", [64, 128, 256])
def test_helper_is_stim_unchanged_for_multiples_of_64(bs):
    mask = np.random.default_rng(2).random((6, bs)) < 0.7
    a = stim.FlipSimulator(batch_size=bs, num_qubits=6, disable_stabilizer_randomization=True, seed=9)
    b = stim.FlipSimulator(batch_size=bs, num_qubits=6, disable_stabilizer_randomization=True, seed=9)
    rng = np.random.default_rng(7)
    state = rng.bit_generator.state
    for pauli in ("X", "Z", "Y"):
        a.broadcast_pauli_errors(pauli=pauli, mask=mask, p=0.3)
        broadcast_pauli_errors(b, pauli=pauli, mask=mask, p=0.3, np_rng=rng)
    xa, za, _, _, _ = a.to_numpy(output_xs=True, output_zs=True)
    xb, zb, _, _, _ = b.to_numpy(output_xs=True, output_zs=True)
    assert np.array_equal(xa, xb) and np.array_equal(za, zb)
    assert rng.bit_generator.state == state


# A leak returning to U is depolarized with p=0.5 X and Z broadcasts in both
# Cosetside (non-sync) and Flipside, so each measurement is 1 with probability 1/2.
_RETURN_TO_U = stim.Circuit("""
R 0 1
I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0 1
I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 1
M 0 1
""")


@pytest.mark.parametrize("kind", ["coset", "flip"])
@pytest.mark.parametrize("bs", [1, 8, 65, 100])
def test_simulators_depolarize_returned_leaks_in_the_trailing_word(kind, bs):
    c = _RETURN_TO_U
    if kind == "coset":
        h = LeakageUint8Coset().compile_op_handler(circuit=c, batch_size=bs)
        sim = CosetsideSimulator(c, compiled_op_handler=h, batch_size=bs, seed=1)
    else:
        h = LeakageUint8Flip().compile_op_handler(circuit=c, batch_size=bs)
        sim = FlipsideSimulator(c, compiled_op_handler=h, batch_size=bs, seed=1)
    reps = -(-1000 // (2 * (bs % 64)))  # >= 1000 trailing samples
    ms = []
    for _ in range(reps):
        sim.clear()
        sim.run()
        ms.append(sim.get_final_measurement_records()[_trailing(bs)])
    rate = np.concatenate(ms).mean()
    assert abs(rate - 0.5) < 0.1, rate  # > 6 sigma


def test_split_fused_instruction():
    op = stim.CircuitInstruction("X_ERROR", [1, 2, 1, 2, 1, 2], [0.25], tag="t")
    copies = split_fused_instruction(op, 2)
    assert copies == [stim.CircuitInstruction("X_ERROR", [1, 2], [0.25], tag="t")] * 3
    for copy_len in (6, 4, 7):  # one copy, non-multiple, too long
        assert split_fused_instruction(op, copy_len) == [op]


# Paths: (simulator, handler, kwargs). Flipside does not support CONDITIONED_ON_OTHER.
_PATHS = {
    "tab_cpp": (TablesideSimulator, LeakageUint8Tableau, dict(use_cpp_kernels=True)),
    "tab_py": (TablesideSimulator, LeakageUint8Tableau, dict(use_cpp_kernels=False)),
    "tab_sync": (TablesideSimulator, LeakageUint8Tableau, dict(sync_tableside_rng=True)),
    "coset": (CosetsideSimulator, LeakageUint8Coset, {}),
    "coset_sync": (CosetsideSimulator, LeakageUint8Coset, dict(sync_tableside_rng=True)),
    "flip": (FlipsideSimulator, LeakageUint8Flip, {}),
}


def _run(path, circuit, bs, seed):
    sim_cls, handler_cls, kw = _PATHS[path]
    h = handler_cls().compile_op_handler(circuit=circuit, batch_size=bs)
    sim = sim_cls(circuit, compiled_op_handler=h, batch_size=bs, seed=seed, **kw)
    sim.run()
    return np.asarray(sim.get_final_measurement_records(), dtype=np.bool_), h


def _fused_and_separated(pre, lines, post):
    """stim fuses consecutive lines with the same gate, args and tag into one op; TICKs don't."""
    fused = stim.Circuit("\n".join([pre, *lines, post]))
    separated = stim.Circuit("\n".join([pre, "\nTICK\n".join(lines), post]))
    assert len(fused) < len(separated)
    return fused, separated


_LEAK = "I[LEAKAGE_TRANSITION_1: (0.3, U-->2)] 0 1"
_MP = "MPAD[LEAKAGE_MEASUREMENT: {} : {}] {}"
_MPAD_CASES = {
    "pads_0_1": ("H 0", [_MP.format("(0, 0), (1, 1)", "0", p) for p in "01"], "M 0"),
    "k3_entangled": ("H 0\nCX 0 2", [_MP.format("(0, 0), (1, 1)", "0 1", "1 0")] * 3, "M 0 1 2"),
    "noisy_leaked": (
        "H 0 1\n" + _LEAK, [_MP.format("(0.1, 0), (0.8, 1), (0.6, 2)", "0 1", "0 1")] * 2, "M 0 1"
    ),
    "no_0_1_keys": ("H 0\n" + _LEAK, [_MP.format("(0.5, 2)", "0", "0")] * 2, "M 0"),
}


@pytest.mark.parametrize("path", list(_PATHS))
@pytest.mark.parametrize("case", list(_MPAD_CASES))
@pytest.mark.parametrize("bs", [1, 7])
def test_fused_mpad_leakage_measurement_is_k_copies(path, case, bs):
    fused, separated = _fused_and_separated(*_MPAD_CASES[case])
    for seed in (1, 2):
        np.testing.assert_array_equal(
            _run(path, fused, bs, seed)[0], _run(path, separated, bs, seed)[0]
        )


@pytest.mark.parametrize("path", list(_PATHS))
def test_fused_mpad_in_repeat_body(path):
    line = _MP.format("(0, 0), (1, 1)", "0", "0")
    fused = stim.Circuit(f"H 0\nREPEAT 2 {{\n{line}\n{line}\n}}\nM 0")
    separated = stim.Circuit(f"H 0\nREPEAT 2 {{\n{line}\nTICK\n{line}\nTICK\n}}\nM 0")
    for seed in (1, 2):
        recs = _run(path, fused, 4, seed)[0]
        np.testing.assert_array_equal(recs, _run(path, separated, 4, seed)[0])
        assert np.all(recs == recs[:, -1:])  # each MPAD reads out the final Z value


_CO_CASES = {
    "superposed_control": ("H 0", ["X[CONDITIONED_ON_OTHER: 1 : 0] 2"] * 3, "M 0 2"),
    "entangled_controls": ("H 0\nCX 0 1", ["X[CONDITIONED_ON_OTHER: 1 : 0 1] 2 3"] * 2, "M 0 1 2 3"),
    "target_is_control": ("H 0", ["X[CONDITIONED_ON_OTHER: 0 : 0] 1 0"] * 2, "M 0 1"),
    "leak_condition": ("H 0\n" + _LEAK, ["X[CONDITIONED_ON_OTHER: 2 : 0] 1"] * 3, "M 1"),
    "U_condition_leaked": ("H 0\n" + _LEAK, ["X[CONDITIONED_ON_OTHER: U : 0] 1"] * 3, "M 1"),
    "x_error": ("H 0", ["X_ERROR[CONDITIONED_ON_OTHER: 1 : 0](0.5) 1"] * 2, "M 0 1"),
    "depolarize1_leaked": (
        "H 0 1\n" + _LEAK, ["DEPOLARIZE1[CONDITIONED_ON_OTHER: 1 2 : 0](0.4) 1"] * 3, "M 0 1"
    ),
}


@pytest.mark.parametrize(
    "path, case, bs",
    [
        (path, case, bs)
        for path in _PATHS
        if path != "flip"
        for case, (_, lines, _) in _CO_CASES.items()
        for bs in (1, 7)
        # At batch_size > 1, Tableside appends the per-copy noise ops to one batch circuit where stim
        # fuses them again, so only the stim RNG draws differ (same distribution): not compared.
        if bs == 1
        or path not in ("tab_cpp", "tab_py")
        or stim.gate_data(stim.Circuit(lines[0])[0].name).is_unitary
    ],
)
def test_fused_conditioned_on_other_is_k_copies(path, case, bs):
    fused, separated = _fused_and_separated(*_CO_CASES[case])
    for seed in (1, 2):
        np.testing.assert_array_equal(
            _run(path, fused, bs, seed)[0], _run(path, separated, bs, seed)[0]
        )


@pytest.mark.parametrize("path", [p for p in _PATHS if p != "flip"])
@pytest.mark.parametrize("k, expected", [(2, 0), (3, 1)])
def test_fused_conditioned_on_other_applies_every_copy(path, k, expected):
    fused, _ = _fused_and_separated("X 0", ["X[CONDITIONED_ON_OTHER: 1 : 0] 2"] * k, "M 2")
    assert np.all(_run(path, fused, 4, 1)[0] == expected)


@pytest.mark.parametrize(
    "case", [*_MPAD_CASES.values(), *_CO_CASES.values()], ids=[*_MPAD_CASES, *_CO_CASES]
)
def test_fused_tags_keep_tableside_coset_rng_lockstep(case):
    fused, _ = _fused_and_separated(*case)
    for seed in (1, 2, 3):
        rec_t, h_t = _run("tab_sync", fused, 1, seed)
        rec_c, h_c = _run("coset_sync", fused, 1, seed)
        np.testing.assert_array_equal(rec_t, rec_c)
        np.testing.assert_array_equal(h_t.state.reshape(-1), h_c.state.reshape(-1))


@pytest.mark.parametrize("gate", ["X_ERROR", "DEPOLARIZE1"])
@pytest.mark.parametrize("pre", ["H 0 1", "H 0 1\n" + _LEAK])
def test_fused_U_conditioned_noise_keeps_sync_lockstep(gate, pre):
    # With unleaked controls the sync runner must not run the bare fused op (one RNG draw pattern
    # for all copies) but apply the copies one by one, like CosetsideSimulator does.
    fused, separated = _fused_and_separated(
        pre, [f"{gate}[CONDITIONED_ON_OTHER: U : 0](0.3) 2"] * 3, "M 1 2"
    )
    for seed in range(1, 9):
        rec_t = _run("tab_sync", fused, 1, seed)[0]
        np.testing.assert_array_equal(rec_t, _run("coset_sync", fused, 1, seed)[0])
        np.testing.assert_array_equal(rec_t, _run("tab_sync", separated, 1, seed)[0])


@pytest.mark.parametrize("case", ["k3_entangled", "noisy_leaked", "x_error", "superposed_control"])
@pytest.mark.parametrize("sync", [False, True])
def test_fused_tags_coset_interactive_matches_run(case, sync):
    pre, lines, post = {**_MPAD_CASES, **_CO_CASES}[case]
    c = stim.Circuit("\n".join([pre, *lines, "H 0 1 2\nCX 0 1 1 2", post]))  # ops after the fused one
    recs = []
    for interactive in (False, True):
        h = LeakageUint8Coset().compile_op_handler(circuit=c, batch_size=7)
        sim = CosetsideSimulator(c, compiled_op_handler=h, batch_size=7, seed=3, sync_tableside_rng=sync)
        if interactive:
            sim.interactive_do(c)
            sim.finish_interactive_run()
        else:
            sim.run()
        recs.append(sim.get_final_measurement_records())
    np.testing.assert_array_equal(*recs)


@pytest.mark.parametrize("path", list(_PATHS))
@pytest.mark.parametrize(
    "text",
    [
        "MPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0 1] 0 0 0\nM 0",  # not a multiple
        "MPAD[LEAKAGE_MEASUREMENT: (0, 0), (1, 1) : 0 0] 0 0\nM 0",  # qubit repeated in a copy
        "X[CONDITIONED_ON_OTHER: 1 : 0 1] 2 3 4\nM 2",  # not a multiple
        "CX[CONDITIONED_ON_OTHER: 1 : 0] 1 2\nM 1",  # CONDITIONED_ON_OTHER is 1Q-only
    ],
)
def test_fused_tag_errors_still_raise(path, text):
    with pytest.raises(ValueError):
        _run(path, stim.Circuit(text), 1, 1)
