import inspect
import itertools
import pickle
import warnings

import numpy as np
import pymatching  # type: ignore[import-untyped]
import pytest
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

import stimside
from stimside.dem_generators import (
    BranchAndBoundDecoder,
    LeakageDecoder,
    MarginalDecoder,
    MarginalLeakageDemGenerator,
)
from stimside.dem_generators.bnb_decoder import (
    _check_validity,
    _conflict_branch,
    _fewest_branch,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8
from stimside.sampler_coset import CosetsideSampler
from stimside.sampler_tableau import TablesideSampler

# The search needs per-shot edge_reweights / return_no_matching (PyMatching fork).
fork_only = pytest.mark.skipif(
    "return_no_matching" not in inspect.signature(pymatching.Matching.decode_batch).parameters,
    reason="needs the PyMatching version with per-shot edge_reweights",
)


def ec4_circuit(
    distance: int, rounds: int, p_leak: float, p_pauli: float = 0.0
) -> stim.Circuit:
    """Rotated memory-Z with skip-gate leakage and an erasure check every round.

    Before each CX layer, each pair leaks one of its qubits (p_leak/2 each);
    ``DEPOLARIZE2(p_pauli)`` follows each CX layer; after the 4th CX layer of
    each round every qubit is heralded (``MPAD`` leakage flag) and unleaked.
    """
    raw = stim.Circuit.generated(
        "surface_code:rotated_memory_z", distance=distance, rounds=rounds
    ).flattened()
    qubits = sorted({t.value for op in raw if op.name == "CX" for t in op.targets_copy()})
    herald = "LEAKAGE_MEASUREMENT: (1.0, 2): " + " ".join(map(str, qubits))
    leak = f"LEAKAGE_TRANSITION_2: ({p_leak / 2}, U_U-->2_U) ({p_leak / 2}, U_U-->U_2)"
    out = stim.Circuit()
    old_to_new: list[int] = []
    num_new = 0
    num_cx = 0
    for op in raw:
        if op.name in ("DETECTOR", "OBSERVABLE_INCLUDE"):
            targets = [
                stim.target_rec(old_to_new[len(old_to_new) + t.value] - num_new)
                if t.is_measurement_record_target
                else t
                for t in op.targets_copy()
            ]
            out.append(op.name, targets, op.gate_args_copy())
            continue
        if op.name == "CX":
            targets = op.targets_copy()
            if p_leak > 0:
                out.append("II_ERROR", targets, [], tag=leak)
            out.append(op)
            if p_pauli > 0:
                out.append("DEPOLARIZE2", targets, [p_pauli])
            num_cx += 1
            if num_cx % 4 == 0:
                out.append("MPAD", [0] * len(qubits), [], tag=herald)
                num_new += len(qubits)
                out.append("I_ERROR", qubits, [], tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)")
            continue
        out.append(op)
        for _ in range(op.num_measurements):
            old_to_new.append(num_new)
            num_new += 1
    return out


def _sample(circuit: stim.Circuit, shots: int, seed: int):
    """(records, dets, observables) of independent Cosetside shots."""
    sampler = CosetsideSampler(
        op_handler=LeakageUint8Coset(), batch_size=shots, dem_decoder=MarginalDecoder(), seed=seed
    )
    sim = sampler.compiled_sampler_for_task(sinter.Task(circuit=circuit, decoder="x")).simulator
    sim.clear()
    sim.run()
    records = np.asarray(sim.get_final_measurement_records(), dtype=bool)
    det_obs = np.asarray(sim.get_detector_flips(append_observables=True), dtype=bool)
    n = circuit.num_detectors
    return records, det_obs[:, :n], det_obs[:, n:]


def _compile(circuit: stim.Circuit, **kwargs):
    return BranchAndBoundDecoder(**kwargs).compile_for_task(sinter.Task(circuit=circuit))


def _unpack(packed: np.ndarray, num: int) -> np.ndarray:
    return np.unpackbits(packed, axis=1, count=num, bitorder="little").astype(bool)


def _brute_valid(matched, covered, rows) -> bool:
    need = (matched - covered) & frozenset().union(*(s for row in rows for s in row))
    return any(need <= frozenset().union(*choice) for choice in itertools.product(*rows))


def _random_rows(rng, universe: int, max_rows: int = 4, max_cands: int = 4):
    return [
        [
            frozenset(np.flatnonzero(rng.random(universe) < 0.35).tolist())
            for _ in range(rng.integers(1, max_cands + 1))
        ]
        for _ in range(rng.integers(0, max_rows + 1))
    ]


def test_settings_and_exports():
    assert stimside.BranchAndBoundDecoder is BranchAndBoundDecoder
    assert issubclass(BranchAndBoundDecoder, LeakageDecoder)
    dec = BranchAndBoundDecoder()
    assert dec.name == "bnb:pymatching"
    assert dec.needs_records and not dec.needs_leakage_events
    assert (dec.max_mwpm_calls, dec.max_queue, dec.priors, dec.branching) == (
        2000,
        4096,
        "trace",
        "conflict",
    )
    with pytest.raises(ValueError, match="needs_records cannot be set to False"):
        dec.needs_records = False
    dec.needs_records = True
    assert (
        BranchAndBoundDecoder(decoder=sinter.BUILT_IN_DECODERS["pymatching"]).name
        == f"bnb:{type(sinter.BUILT_IN_DECODERS['pymatching']).__name__}"
    )
    assert BranchAndBoundDecoder(name="mine").name == "mine"
    with pytest.raises(ValueError, match="loss_oracle"):
        BranchAndBoundDecoder(MarginalLeakageDemGenerator(loss_oracle=True))
    with pytest.raises(TypeError, match="MarginalLeakageDemGenerator"):
        BranchAndBoundDecoder(stim.DetectorErrorModel())
    with pytest.raises(ValueError, match="Unknown built-in sinter decoder"):
        BranchAndBoundDecoder(decoder="no_such_decoder")
    for bad in (dict(priors="x"), dict(branching="x"), dict(max_mwpm_calls=0), dict(max_queue=0)):
        with pytest.raises(ValueError):
            BranchAndBoundDecoder(**bad)
    clone = pickle.loads(pickle.dumps(BranchAndBoundDecoder(priors="uniform", max_queue=None)))
    assert clone.priors == "uniform" and clone.max_queue is None and clone.needs_records


def test_validity_check_matches_brute_force_and_witness_is_sound():
    rng = np.random.default_rng(1)
    num_invalid = 0
    for _ in range(3000):
        rows = _random_rows(rng, 9)
        matched = frozenset(np.flatnonzero(rng.random(9) < 0.5).tolist())
        covered = frozenset(np.flatnonzero(rng.random(9) < 0.15).tolist())
        valid, witness = _check_validity(matched, covered, rows)
        assert valid == _brute_valid(matched, covered, rows)
        if valid:
            assert witness == frozenset()
            continue
        num_invalid += 1
        need = (matched - covered) & frozenset().union(*(s for row in rows for s in row))
        assert witness and witness <= need
        # No choice of one candidate per row covers the witness.
        assert not any(
            witness <= frozenset().union(*choice) for choice in itertools.product(*rows)
        )
        # Branching always finds an open row (the conflict rule may find none).
        assert _conflict_branch(witness, matched, rows) in (None, *range(len(rows)))
        assert _fewest_branch(witness, rows) is not None
    assert num_invalid > 300


def test_dominated_events_can_be_dropped_from_the_validity_check():
    rng = np.random.default_rng(2)
    for _ in range(2000):
        rows = _random_rows(rng, 9)
        dominated = []
        for row in rows:
            if rng.random() < 0.5:
                row.append(frozenset().union(*row))  # a dominant candidate
                dominated.append(True)
            else:
                dominated.append(False)
        matched = frozenset(np.flatnonzero(rng.random(9) < 0.5).tolist())
        covered = frozenset(np.flatnonzero(rng.random(9) < 0.15).tolist())
        kept = [row for row, dom in zip(rows, dominated) if not dom]
        extra = frozenset().union(*(s for row, dom in zip(rows, dominated) if dom for s in row))
        want = _brute_valid(matched, covered, rows)
        assert _check_validity(matched, covered, rows)[0] == want
        assert _check_validity(matched, covered | extra, kept)[0] == want


@fork_only
def test_candidates_average_to_the_marginal_envelopes_and_root_is_marginal():
    circuit = ec4_circuit(3, 2, 0.03, 0.03 / 20)
    records, dets, _ = _sample(circuit, 256, seed=3)
    comp = _compile(circuit)
    assert comp.dominated.any() and not comp.dominated[
        [f for f, env in enumerate(comp.env_factors) if env is not None]
    ].all()
    a = comp.analysis
    raised = records[:, a.flag_records] ^ a.flag_invert
    want = MarginalLeakageDemGenerator(decompose_errors=True, reweight_only=True)(
        circuit, records
    )
    num_events = 0
    for s in range(records.shape[0]):
        events = comp._events(raised[s])
        num_events += len(events)
        rw, _ = comp._reweights(events, {})
        assert rw.shape == want[s].shape
        np.testing.assert_array_equal(rw[:, :2], want[s][:, :2])
        np.testing.assert_allclose(rw[:, 2], want[s][:, 2], rtol=0, atol=1e-9)
    assert num_events > 100
    # Root predictions (on the search graph) == MarginalDecoder's.
    packed = np.packbits(dets, axis=1, bitorder="little")
    roots = [comp._reweights(comp._events(raised[s]), {})[0] for s in range(len(dets))]
    got = comp._get_matcher().decode_batch(
        packed, bit_packed_shots=True, bit_packed_predictions=True, edge_reweights=roots
    )
    marginal = MarginalDecoder(decompose_errors=True, reweight_only=True).compile_for_task(
        sinter.Task(circuit=circuit)
    )
    np.testing.assert_array_equal(
        got,
        marginal.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records),
    )


@fork_only
@pytest.mark.parametrize("priors", ["trace", "uniform"])
def test_child_weight_is_never_below_its_parent(priors):
    circuit = ec4_circuit(3, 2, 0.05, 0.05 / 20)
    records, dets, _ = _sample(circuit, 128, seed=4)
    comp = _compile(circuit, priors=priors)
    a = comp.analysis
    raised = records[:, a.flag_records] ^ a.flag_invert
    matcher = comp._get_matcher()
    num_children = 0
    for s in range(len(dets)):
        events = comp._events(raised[s])
        free = [f for f in events if not comp.dominated[f]][:2]
        if not free:
            continue
        d = dets[s].astype(np.uint8)
        _, w_root = matcher.decode(d, return_weight=True, edge_reweights=comp._reweights(events, {})[0])
        for c in range(len(comp.cand_edges[free[0]])):
            cons = {free[0]: c}
            _, w = matcher.decode(d, return_weight=True, edge_reweights=comp._reweights(events, cons)[0])
            assert w >= w_root - 1e-6
            num_children += 1
            for f2 in free[1:]:
                for c2 in range(len(comp.cand_edges[f2])):
                    cons2 = {**cons, f2: c2}
                    _, w2 = matcher.decode(
                        d, return_weight=True, edge_reweights=comp._reweights(events, cons2)[0]
                    )
                    assert w2 >= w - 1e-6
    assert num_children > 50


@fork_only
@pytest.mark.parametrize("branching", ["conflict", "fewest"])
def test_accepted_nodes_are_valid_by_brute_force(branching):
    circuit = ec4_circuit(3, 3, 0.05, 0.05 / 20)
    records, dets, _ = _sample(circuit, 256, seed=5)
    comp = _compile(circuit, branching=branching)
    a = comp.analysis
    raised = records[:, a.flag_records] ^ a.flag_invert
    statuses = []
    for s in range(len(dets)):
        d = dets[s].astype(np.uint8)
        events = comp._events(raised[s])
        rw, status, cons, matched = comp._decode_shot(d, events)
        statuses.append(status)
        if status == "fallback":
            continue
        if matched is None:
            matched = comp._matched_edges(d, rw)
        covered = frozenset().union(*(comp.cand_edges[f][c] for f, c in cons.items()))
        rows = [comp.cand_edges[f] for f in events if f not in cons]  # incl. dominated
        assert _brute_valid(matched, covered, rows)
    assert statuses.count("child") > 0, statuses
    assert comp.stats["fallbacks"] == statuses.count("fallback") == 0


@fork_only
def test_budget_fallback_returns_the_marginal_graph():
    circuit = ec4_circuit(3, 3, 0.05, 0.05 / 20)
    records, dets, _ = _sample(circuit, 256, seed=5)
    packed = np.packbits(dets, axis=1, bitorder="little")
    comp = _compile(circuit, max_mwpm_calls=1)
    got = comp.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records)
    assert comp.stats["fallbacks"] > 0
    assert comp.stats["child"] == 0
    assert comp.stats["fallbacks"] == comp.stats["budget_exhausted"]
    marginal = MarginalDecoder(decompose_errors=True, reweight_only=True).compile_for_task(
        sinter.Task(circuit=circuit)
    )
    np.testing.assert_array_equal(
        got,
        marginal.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records),
    )


@fork_only
def test_bnb_corrects_more_than_marginal_and_avoids_placeholder_edges():
    circuit = ec4_circuit(3, 3, 0.05)  # eta = inf: every flip comes from a heralded leak
    records, dets, obs = _sample(circuit, 512, seed=6)
    packed = np.packbits(dets, axis=1, bitorder="little")
    comp = _compile(circuit)
    got = _unpack(
        comp.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records),
        circuit.num_observables,
    )
    marginal = MarginalDecoder(decompose_errors=True, reweight_only=True).compile_for_task(
        sinter.Task(circuit=circuit)
    )
    want = _unpack(
        marginal.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records),
        circuit.num_observables,
    )
    bnb_errors = int(np.any(got != obs, axis=1).sum())
    marginal_errors = int(np.any(want != obs, axis=1).sum())
    assert comp.stats["child"] > 0
    assert bnb_errors <= marginal_errors
    assert comp.stats["accepted_with_placeholder"] == 0
    assert comp.stats["fallbacks"] == 0


@fork_only
def test_forced_two_leak_pair_that_marginal_misdecodes():
    # Found by the exhaustive d=3 2-leak scan: both pairs 1 and 2 of the LT2
    # before instruction 77 leak their first qubit.
    full = ec4_circuit(3, 3, 0.01)
    forced = stim.Circuit()
    for j, op in enumerate(full):
        if op.name == "II_ERROR":
            if j == 76:
                t = op.targets_copy()
                forced.append(
                    "II_ERROR", t[2:6], [], tag="LEAKAGE_TRANSITION_2: (1.0, U_U-->2_U)"
                )
            continue
        forced.append(op)
    records, dets, obs = _sample(forced, 64, seed=3)
    packed = np.packbits(dets, axis=1, bitorder="little")
    task = sinter.Task(circuit=full)
    comp = BranchAndBoundDecoder(max_queue=None).compile_for_task(task)
    marginal = MarginalDecoder(decompose_errors=True, reweight_only=True).compile_for_task(task)
    errors = {}
    for name, dec in (("bnb", comp), ("marginal", marginal)):
        got = _unpack(
            dec.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records),
            full.num_observables,
        )
        errors[name] = int(np.any(got != obs, axis=1).sum())
    assert errors["marginal"] > 0
    assert errors["bnb"] == 0
    assert comp.stats["child"] > 0


class _RecordingDecoder(sinter.Decoder):
    """A non-PyMatching sinter decoder that records the DEMs it compiles (predicts 0)."""

    def __init__(self):
        self.dems: list[stim.DetectorErrorModel] = []

    def compile_decoder_for_dem(self, *, dem):
        self.dems.append(dem)
        num_obs = dem.num_observables

        class _Zeros:
            def decode_shots_bit_packed(self, *, bit_packed_detection_event_data):
                n = bit_packed_detection_event_data.shape[0]
                return np.zeros((n, (num_obs + 7) // 8), dtype=np.uint8)

        return _Zeros()


def _sym_probs(dem: stim.DetectorErrorModel) -> dict[tuple[str, ...], float]:
    out: dict[tuple[str, ...], float] = {}
    for inst in dem.flattened():
        if inst.type == "error":
            key = tuple(sorted(str(t) for t in inst.targets_copy()))
            p, q = inst.args_copy()[0], out.get(key, 0.0)
            out[key] = p + q - 2 * p * q
    return out


@fork_only
def test_other_decoders_decode_the_accepted_nodes_undecomposed_dem():
    circuit = ec4_circuit(3, 3, 0.05, 0.05 / 20)
    records, dets, _ = _sample(circuit, 256, seed=5)
    packed = np.packbits(dets, axis=1, bitorder="little")
    pm = _compile(circuit)
    pm.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records)
    rec = _RecordingDecoder()
    dec = BranchAndBoundDecoder(decoder=rec)
    assert dec.name == "bnb:_RecordingDecoder"
    comp = dec.compile_for_task(sinter.Task(circuit=circuit))
    assert comp.hyper_analysis is not None and pm.hyper_analysis is None
    got = comp.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records)
    assert got.shape == (len(dets), 1) and not got.any()
    # The search is the same as with PyMatching.
    assert comp.stats == pm.stats and comp.stats["child"] > 0
    # Per shot, the accepted node (in first-appearance order: one compile each).
    a = pm.analysis
    raised = records[:, a.flag_records] ^ a.flag_invert
    nodes: dict = {}
    shot_node = []
    for s in range(len(dets)):
        events = pm._events(raised[s])
        _, status, cons, _ = pm._decode_shot(dets[s].astype(np.uint8), events)
        key = (tuple(events), tuple(sorted(cons.items())))
        nodes.setdefault(key, status)
        shot_node.append(list(nodes).index(key))
    assert len(rec.dems) == len(nodes)
    marginal = MarginalLeakageDemGenerator(decompose_errors=False)(circuit, records)
    base = _sym_probs(comp.hyper_analysis.baseline)
    sym_targets = comp.hyper_analysis._get_sym_targets()

    def marginal_of(s: int, flags) -> dict[tuple[str, ...], float]:
        """The marginal DEM of shot s with exactly ``flags`` raised."""
        r = records[s].copy()
        r[a.flag_records] = np.isin(np.arange(len(a.flag_records)), list(flags)) ^ a.flag_invert
        assert pm._events(r[a.flag_records] ^ a.flag_invert) == sorted(flags)
        return _sym_probs(MarginalLeakageDemGenerator(decompose_errors=False)(circuit, r))

    def xor_in(acc: dict, syms: dict[int, float]) -> None:
        for sym, p in syms.items():
            k = tuple(sorted(str(t) for t in sym_targets[sym]))
            q = acc.get(k, 0.0)
            acc[k] = q + p - 2 * q * p

    num_child = 0
    for s in range(len(dets)):
        dem = rec.dems[shot_node[s]]
        assert "^" not in str(dem)
        got_p, want_p = _sym_probs(dem), _sym_probs(marginal[s])
        status = list(nodes.values())[shot_node[s]]
        if status == "root":
            assert got_p.keys() == want_p.keys()
            for k in want_p:
                assert got_p[k] == pytest.approx(want_p[k], abs=1e-12)
        else:
            assert status == "child"
            num_child += 1
            # A chosen candidate replaces its event's envelope: no new
            # mechanism, none more likely, and the graph differs.
            assert got_p.keys() <= want_p.keys()
            assert all(got_p[k] <= want_p[k] + 1e-12 for k in got_p)
            assert got_p != want_p
            # Exactly: the marginal DEM of the unconstrained events, xor each
            # constrained event's chosen candidate, whose candidates add up to
            # the generator's own envelope of that event.
            events, cons = (list(x) for x in list(nodes)[shot_node[s]])
            cons = dict(cons)
            oracle = marginal_of(s, [f for f in events if f not in cons])
            for f, c in cons.items():
                alone = marginal_of(s, [f])
                env = {
                    k: (p - base.get(k, 0.0)) / (1 - 2 * base.get(k, 0.0))
                    for k, p in alone.items()
                }
                total: dict[int, float] = {}
                for cand in comp.hyper_cand_syms[f]:
                    for sym, p in cand.items():
                        total[sym] = total.get(sym, 0.0) + p
                summed: dict = {}
                xor_in(summed, total)
                assert summed.keys() == {k for k, p in env.items() if abs(p) > 1e-15}
                assert all(summed[k] == pytest.approx(env[k], abs=1e-9) for k in summed)
                xor_in(oracle, comp.hyper_cand_syms[f][c])
            oracle = {k: p for k, p in oracle.items() if p > 0}
            assert got_p.keys() == oracle.keys()
            assert all(got_p[k] == pytest.approx(oracle[k], abs=1e-9) for k in oracle)
    assert num_child > 0
    assert any(len([t for t in k if t.startswith("D")]) > 2 for k in base)  # hyperedges kept
    # One compile per distinct accepted node per batch, not one per shot.
    once = len(rec.dems)
    comp.decode_shots_bit_packed(
        bit_packed_detection_event_data=np.concatenate([packed, packed]),
        records=np.concatenate([records, records]),
    )
    assert len(rec.dems) - once == once


@fork_only
def test_tesseract_decodes_the_accepted_nodes_dem():
    tsc = pytest.importorskip("tesseract_decoder.tesseract_sinter_compat")
    circuit = ec4_circuit(3, 3, 0.05, 0.05 / 20)
    records, dets, obs = _sample(circuit, 256, seed=6)
    packed = np.packbits(dets, axis=1, bitorder="little")
    comp = BranchAndBoundDecoder(decoder=tsc.TesseractSinterDecoder()).compile_for_task(
        sinter.Task(circuit=circuit)
    )
    got = _unpack(
        comp.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records),
        circuit.num_observables,
    )
    assert got.shape == obs.shape
    assert int(np.any(got != obs, axis=1).sum()) < len(obs) // 4


@fork_only
def test_requires_records_and_warns_when_equivalent_to_marginal():
    plain = stim.Circuit.generated(
        "repetition_code:memory", distance=3, rounds=2, after_clifford_depolarization=0.01
    )
    with pytest.warns(UserWarning, match="no leakage flags"):
        comp = _compile(plain)
    with pytest.raises(ValueError, match="requires records"):
        comp.decode_shots_bit_packed(bit_packed_detection_event_data=np.zeros((1, 1), np.uint8))
    imperfect = stim.Circuit(str(ec4_circuit(3, 2, 0.01)).replace("(1.0, 2):", "(0.9, 2):"))
    with pytest.warns(UserWarning, match="perfect leakage heralds"):
        _compile(imperfect)
    with warnings.catch_warnings():
        warnings.simplefilter("error")
        _compile(ec4_circuit(3, 2, 0.01))


@fork_only
def test_pickles_and_runs_in_two_sinter_workers():
    circuit = ec4_circuit(3, 2, 0.02, 0.001)
    records, dets, _ = _sample(circuit, 64, seed=8)
    packed = np.packbits(dets, axis=1, bitorder="little")
    comp = _compile(circuit)
    want = comp.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records)
    clone = pickle.loads(pickle.dumps(comp))
    np.testing.assert_array_equal(
        clone.decode_shots_bit_packed(bit_packed_detection_event_data=packed, records=records),
        want,
    )
    samplers = {
        "tab": TablesideSampler(
            op_handler=LeakageUint8(), batch_size=4, dem_decoder=BranchAndBoundDecoder(), seed=9
        ),
        "cos": CosetsideSampler(
            op_handler=LeakageUint8Coset(),
            batch_size=16,
            dem_decoder=BranchAndBoundDecoder(),
            seed=9,
        ),
    }
    for s in samplers.values():
        assert pickle.loads(pickle.dumps(s)).dem_decoder.name == "bnb:pymatching"
    stats = sinter.collect(
        num_workers=2,
        tasks=[
            sinter.Task(circuit=circuit, decoder=k, json_metadata={"k": k}) for k in samplers
        ],
        max_shots=64,
        custom_decoders=samplers,
    )
    assert sorted(s.decoder for s in stats) == sorted(samplers)
    assert all(s.shots >= 64 for s in stats)
