"""Branch-and-bound (B&B) leakage decoder (App. B4 of surface_code_leakage_erasure.pdf).

The marginal decoder (``MarginalDecoder``) decodes a shot's leakage flags with
the disjoint average ``G_E = sum_j P_j G_L(j)`` of each erasure-check event E
over its candidate leakage flows j. With skip-gate leakage the candidates of
one event have different (non-nested) error sets, and the matching of the
averaged graph can use edges that no single choice of one candidate per event
explains. ``BranchAndBoundDecoder`` then searches, best-first by matching
weight, over partially constrained graphs in which some events keep only one
candidate (scaled by its prior, Eqs. B10-B12), until the matching of a node is
explained by one candidate per unconstrained event (Eq. B13).

The search follows the paper's reference implementation
(surface-code-leakage-erasure, ``decoding.py``): the validity check backtracks
with a lifted conflict witness, and it branches on the event most implicated in
the witness's conflict pairs. The search runs on PyMatching (it needs each
node's matching weight and edges), which requires the PyMatching version with
per-shot ``edge_reweights`` and ``return_no_matching``. With PyMatching as the
``decoder`` setting, the accepted node's (graphlike) search graph is decoded;
another decoder decodes the accepted node's undecomposed DEM (hyperedges kept).
"""

from __future__ import annotations

from collections import Counter
import heapq
import inspect
import itertools
import math
from typing import Any, Sequence
import warnings

import numpy as np
from numpy.typing import NDArray
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.dem_generators.dem_decoding import (
    _decode_with_edge_reweights,
    _is_pymatching_decoder,
)
from stimside.dem_generators.dem_generator_marginal import (
    MarginalLeakageDemGenerator,
    _Analysis,
    _Builder,
)
from stimside.dem_generators.leakage_decoder import (
    CompiledLeakageDecoder,
    LeakageDecoder,
    _check_decoder,
)
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageMeasurementParams,
)
from stimside.op_handlers.leakage_handlers.tag_registry import parse_leakage_tag
from stimside.util.leakage_events import ShotLeakageEvents

# Weight of the edges that only leakage can flip (the marginal generator's
# 1e-12 placeholders, weight ~27.6) in the search graph: large enough that a
# node's matching avoids leaks nobody heralded, and far below PyMatching's
# 2**24 - 1 cap, whose use as "infinity" costs weight resolution.
_PLACEHOLDER_WEIGHT = 65.0
_MAX_REWEIGHT = 100.0
_PRIORS = ("trace", "uniform")
_BRANCHINGS = ("conflict", "fewest")


def _backtrack(
    rows: Sequence[Sequence[frozenset[int]]],
    uncovered: frozenset[int],
    start: int = 0,
) -> tuple[bool, frozenset[int]]:
    """Whether one set per row (from ``start`` on) can cover ``uncovered``.

    Port of the reference ``backtrack`` (with its fix 07fcaab). On failure the
    witness is a subset of ``uncovered`` that no choice from these rows covers.
    """
    if not uncovered:
        return True, frozenset()
    if start >= len(rows):
        return False, uncovered
    row = rows[start]
    coverage = [len(uncovered & s) for s in row]
    order = sorted(range(len(row)), key=lambda i: (coverage[i], i), reverse=True)
    candidates = [row[i] for i in order if coverage[i] > 0]
    if not candidates:
        return _backtrack(rows, uncovered, start + 1)
    conflict = uncovered
    lifted: set[int] = set()
    for idx, edges in enumerate(candidates):
        valid, witness = _backtrack(rows, uncovered - edges, start + 1)
        if valid:
            return True, frozenset()
        lifted |= witness
        if len(witness) < len(conflict):
            conflict = witness
        if not conflict & frozenset().union(*candidates[idx + 1 :]):
            break  # no remaining sibling covers any of the conflict witness
    return False, frozenset(lifted)


def _check_validity(
    matched: frozenset[int],
    covered: frozenset[int],
    rows: Sequence[Sequence[frozenset[int]]],
) -> tuple[bool, frozenset[int]]:
    """Eq. B13: whether one set per row covers the matched edges they contain.

    Args:
        matched: the node's matched edges.
        covered: edges already explained (the chosen candidates of the
            constrained events and the envelopes of dominated events).
        rows: the candidates' edge sets of each remaining event.

    Returns ``(valid, witness)`` as ``_backtrack``, with the reference's
    ordering: rows with no uncovered matched edge are dropped and the others
    tried most constrained (fewest (edge, candidate) incidences) first.
    """
    need = set(matched - covered)
    in_rows: set[int] = set()
    for row in rows:
        for s in row:
            in_rows |= s
    need &= in_rows
    counts = [sum(len(need & s) for s in row) for row in rows]
    order = sorted((c, i) for i, c in enumerate(counts) if c > 0)
    return _backtrack([rows[i] for _, i in order], frozenset(need))


def _conflict_branch(
    witness: frozenset[int],
    matched: frozenset[int],
    rows: Sequence[Sequence[frozenset[int]]],
) -> int | None:
    """The reference's branching rule; returns a row index or None.

    For each witness edge e and each (row, candidate) covering e, the matched
    edges in the row's union but not in the candidate form a conflict pair with
    e; the row covering edges of the conflict pairs most often is returned
    (ties: lowest index).
    """
    coverage: dict[int, list[tuple[int, int]]] = {}
    for r, row in enumerate(rows):
        for c, s in enumerate(row):
            for e in s:
                coverage.setdefault(e, []).append((r, c))
    unions = [frozenset().union(*row) for row in rows]
    counts: Counter[int] = Counter()
    for e in witness:
        for r, c in coverage.get(e, ()):
            problems = matched & (unions[r] - rows[r][c])
            if problems:
                for edge in (e, *problems):
                    counts.update(rr for rr, _ in coverage[edge])
    if not counts:
        return None
    return min(counts, key=lambda r: (-counts[r], r))


def _fewest_branch(
    witness: frozenset[int], rows: Sequence[Sequence[frozenset[int]]]
) -> int | None:
    """The row with the fewest candidates covering a witness edge (ties: lowest index)."""
    hits = [r for r, row in enumerate(rows) if any(witness & s for s in row)]
    return min(hits, key=lambda r: (len(rows[r]), r)) if hits else None


def _search_dem(analysis: _Analysis) -> stim.DetectorErrorModel:
    """``matching_base_dem`` with its placeholder edges at ``_PLACEHOLDER_WEIGHT``."""
    num_base = len(analysis.baseline)
    p_placeholder = 1.0 / (1.0 + math.exp(_PLACEHOLDER_WEIGHT))
    out = stim.DetectorErrorModel()
    for idx, inst in enumerate(analysis.matching_base_dem):
        if idx < num_base:
            out.append(inst)
            continue
        assert inst.type == "error" and inst.args_copy() == [1e-12], inst
        out.append("error", p_placeholder, inst.targets_copy())
    return out


def _edge_factors(
    sym_probs: dict[int, float], analysis: _Analysis
) -> tuple[NDArray[np.int32], NDArray[np.float64]]:
    """(sorted edge ids, prod over the symptoms on each edge of 1 - 2p)."""
    offs, ids = analysis.sym_edge_offsets, analysis.sym_edge_ids
    edges: list[int] = []
    factors: list[float] = []
    for sym, p in sym_probs.items():
        for e in ids[offs[sym] : offs[sym + 1]]:
            edges.append(int(e))
            factors.append(1.0 - 2.0 * p)
    return _reduce_factors(
        np.asarray(edges, dtype=np.int32), np.asarray(factors, dtype=np.float64)
    )


def _reduce_factors(
    edges: NDArray[np.int32], factors: NDArray[np.float64]
) -> tuple[NDArray[np.int32], NDArray[np.float64]]:
    if edges.size == 0:
        return edges, factors
    order = np.argsort(edges, kind="stable")
    edges, factors = edges[order], factors[order]
    starts = np.flatnonzero(np.r_[True, edges[1:] != edges[:-1]])
    return edges[starts], np.multiply.reduceat(factors, starts)


def _has_fork_matching() -> bool:
    import pymatching  # type: ignore[import-untyped]

    params = inspect.signature(pymatching.Matching.decode_batch).parameters
    return "return_no_matching" in params and "edge_reweights" in params


def _flag_symptoms(
    a: _Analysis, priors: str
) -> tuple[list[list[dict[int, float]]], list[dict[int, float] | None]]:
    """Per flag: each candidate's prior-scaled symptom probabilities, and their disjoint average.

    Flags that are never events (no envelope in the marginal generator) get no
    candidates and a ``None`` envelope.
    """
    assert a.flag_cand_offsets is not None and a.flag_cand_flows is not None
    assert a.flag_cand_weights is not None and a.flow_site_offsets is not None
    assert a.flow_site_ids is not None and a.entry_site_ids is not None
    assert a.entry_sym_ids is not None and a.entry_probs is not None
    site_syms: dict[int, dict[int, float]] = {}
    for site, sym, p in zip(
        a.entry_site_ids.tolist(), a.entry_sym_ids.tolist(), a.entry_probs.tolist()
    ):
        if p > 0:
            d = site_syms.setdefault(site, {})
            q = d.get(sym, 0.0)
            d[sym] = q + p - 2 * q * p
    flow_syms: list[dict[int, float]] = []
    for k in range(len(a.flow_site_offsets) - 1):
        acc: dict[int, float] = {}
        for site in a.flow_site_ids[a.flow_site_offsets[k] : a.flow_site_offsets[k + 1]]:
            for sym, p in site_syms.get(int(site), {}).items():
                q = acc.get(sym, 0.0)
                acc[sym] = q + p - 2 * q * p
        flow_syms.append({s: p for s, p in acc.items() if p > 0})

    num_flags = len(a.flag_records)
    cand_syms: list[list[dict[int, float]]] = [[] for _ in range(num_flags)]
    env_syms: list[dict[int, float] | None] = [None] * num_flags
    for f in range(num_flags):
        if a.flag_env_offsets[f + 1] == a.flag_env_offsets[f]:
            continue  # never an event (as in the marginal generator)
        c0, c1 = int(a.flag_cand_offsets[f]), int(a.flag_cand_offsets[f + 1])
        flows = a.flag_cand_flows[c0:c1].tolist()
        weights = a.flag_cand_weights[c0:c1]
        if priors == "trace":
            probs = (weights / weights.sum()).tolist()
        else:
            probs = [1.0 / len(flows)] * len(flows)
        env: dict[int, float] = {}
        for k, prob in zip(flows, probs):
            syms = flow_syms[k]
            cand_syms[f].append({s: prob * p for s, p in syms.items()})
            for s, p in syms.items():
                env[s] = env.get(s, 0.0) + prob * p
        env_syms[f] = env
    return cand_syms, env_syms


class BranchAndBoundDecoder(LeakageDecoder):
    """Branch-and-bound decoder of leakage flags (App. B4 of the paper).

    Per shot, it decodes the marginal graph (the one of ``MarginalDecoder``
    with ``decompose_errors=True, reweight_only=True``) and checks that its
    matching is explained by one candidate leakage flow per erasure-check
    event (Eq. B13). If not, it searches best-first over partially constrained
    graphs (Eqs. B10-B12) until a node's matching is; if the search runs out of
    ``max_mwpm_calls`` or the queue empties, it falls back to the marginal
    graph (counted in the compiled decoder's ``stats["fallbacks"]``). The
    accepted graph is decoded with ``decoder``.

    B&B can only differ from marginal decoding when an event's candidates have
    non-nested edge sets, as with leaked qubits that skip their gates
    (``unconditional_condition_on_U=True``); like the paper, it assumes that
    each raised flag heralds exactly one leak. The search runs on PyMatching
    and needs the PyMatching version with per-shot ``edge_reweights``.

    Args:
        dem_gen: provides ``unconditional_condition_on_U`` (its other settings
            are ignored: the B&B search always runs on graphlike edges). ``None``
            uses ``MarginalLeakageDemGenerator()``. ``loss_oracle`` generators
            are rejected.
        decoder: decodes the accepted node: a ``sinter.Decoder`` or the name of
            one in ``sinter.BUILT_IN_DECODERS``. PyMatching decodes the search
            graph with that node's edge reweights, i.e. the graph the search
            validated. Any other decoder decodes the node's undecomposed DEM
            (built like ``MarginalLeakageDemGenerator(decompose_errors=False)``,
            with each constrained event's chosen candidate in place of its
            envelope), compiled once per distinct accepted node and batch; it
            must accept hyperedges, and the search's guarantees (which concern
            the MWPM of the graph) do not extend to its correction.
        max_mwpm_calls: MWPM calls per shot after which the search stops and
            falls back (checked before each node is expanded).
        max_queue: the search queue is trimmed to its ``max_queue`` lowest
            weight nodes (as the reference does); ``None`` keeps every node.
        priors: ``"trace"`` weighs each event's candidates like the marginal
            generator (so the root is its graph); ``"uniform"`` uses 1/N as
            the paper and its reference implementation.
        branching: ``"conflict"`` branches on the event most implicated in the
            conflict pairs (the reference's rule); ``"fewest"`` on the event
            with the fewest candidates covering an unexplained edge.
        name: the decoder's name; defaults to ``"bnb:<decoder>"``.
    """

    def __init__(
        self,
        dem_gen: MarginalLeakageDemGenerator | None = None,
        decoder: sinter.Decoder | str = "pymatching",
        max_mwpm_calls: int = 2000,
        max_queue: int | None = 4096,
        priors: str = "trace",
        branching: str = "conflict",
        name: str | None = None,
    ) -> None:
        if dem_gen is None:
            dem_gen = MarginalLeakageDemGenerator()
        if not isinstance(dem_gen, MarginalLeakageDemGenerator):
            raise TypeError(
                "BranchAndBoundDecoder requires dem_gen to be a "
                f"MarginalLeakageDemGenerator or None, got {type(dem_gen).__name__}."
            )
        if dem_gen.loss_oracle:
            raise ValueError("BranchAndBoundDecoder does not support loss_oracle=True.")
        if priors not in _PRIORS:
            raise ValueError(f"priors must be one of {_PRIORS}, got {priors!r}.")
        if branching not in _BRANCHINGS:
            raise ValueError(f"branching must be one of {_BRANCHINGS}, got {branching!r}.")
        if max_mwpm_calls < 1:
            raise ValueError(f"max_mwpm_calls must be >= 1, got {max_mwpm_calls}.")
        if max_queue is not None and max_queue < 1:
            raise ValueError(f"max_queue must be None or >= 1, got {max_queue}.")
        dec_name = _check_decoder(decoder, "BranchAndBoundDecoder")
        self.dem_gen = dem_gen
        self.decoder = decoder
        self.max_mwpm_calls = max_mwpm_calls
        self.max_queue = max_queue
        self.priors = priors
        self.branching = branching
        self._name = f"bnb:{dec_name}" if name is None else name

    @property
    def name(self) -> str:
        return self._name

    @property
    def needs_records(self) -> bool:
        """Always True: the events are the leakage flags in the records."""
        return True

    @needs_records.setter
    def needs_records(self, value: bool) -> None:
        if not value:
            raise ValueError(
                "BranchAndBoundDecoder needs records (its events are the leakage "
                "flags in them); needs_records cannot be set to False."
            )

    def compile_for_task(self, task: sinter.Task) -> CompiledLeakageDecoder:
        if task.circuit is None:
            raise ValueError("BranchAndBoundDecoder requires a circuit in the task.")
        decoder = self.decoder
        if isinstance(decoder, str):
            decoder = sinter.BUILT_IN_DECODERS[decoder]
        return _CompiledBranchAndBoundDecoder(
            circuit=task.circuit,
            decoder=decoder,
            unconditional_condition_on_U=self.dem_gen.unconditional_condition_on_U,
            max_mwpm_calls=self.max_mwpm_calls,
            max_queue=self.max_queue,
            priors=self.priors,
            branching=self.branching,
        )


class _CompiledBranchAndBoundDecoder(CompiledLeakageDecoder):
    """Per-circuit search tables; ``stats`` counts what the searches did."""

    def __init__(
        self,
        circuit: stim.Circuit,
        decoder: sinter.Decoder,
        unconditional_condition_on_U: bool,
        max_mwpm_calls: int,
        max_queue: int | None,
        priors: str,
        branching: str,
    ) -> None:
        if not _has_fork_matching():
            raise ImportError(
                "BranchAndBoundDecoder needs the PyMatching version with per-shot "
                "edge_reweights and return_no_matching in Matching.decode_batch."
            )
        self.decoder = decoder
        self.max_mwpm_calls = max_mwpm_calls
        self.max_queue = max_queue
        self.branching = branching
        self.num_detectors = circuit.num_detectors
        self.num_observables = circuit.num_observables
        self.stats: Counter[str] = Counter()
        builder = _Builder(circuit, unconditional_condition_on_U)
        analysis = builder.build(decompose_errors=True, keep_candidates=True)
        self.analysis = analysis
        self.search_dem = _search_dem(analysis)
        self._matcher: Any = None
        self._build_tables(analysis, priors)
        self._warn_if_equivalent_to_marginal(circuit)
        # A non-PyMatching decoder decodes the accepted node's undecomposed DEM
        # (hyperedges kept), built from this analysis' symptoms.
        self.hyper_analysis: _Analysis | None = None
        if not _is_pymatching_decoder(decoder):
            hyper = builder.build(decompose_errors=False, keep_candidates=True)
            for field in (
                "flag_records",
                "flag_invert",
                "flag_cand_offsets",
                "flag_cand_flows",
                "flag_cand_weights",
                "flow_site_offsets",
                "flow_site_ids",
            ):
                if not np.array_equal(getattr(hyper, field), getattr(analysis, field)):
                    raise AssertionError(f"undecomposed analysis differs in {field}")
            self.hyper_analysis = hyper
            self.hyper_cand_syms, self.hyper_env_syms = _flag_symptoms(hyper, priors)

    def __getstate__(self) -> dict[str, Any]:
        state = self.__dict__.copy()
        state["_matcher"] = None
        return state

    def _get_matcher(self) -> Any:
        if self._matcher is None:
            import pymatching  # type: ignore[import-untyped]

            self._matcher = pymatching.Matching.from_detector_error_model(self.search_dem)
        return self._matcher

    def _build_tables(self, a: _Analysis, priors: str) -> None:
        cand_syms, env_syms = _flag_symptoms(a, priors)
        offs, ids = a.sym_edge_offsets, a.sym_edge_ids
        num_flags = len(a.flag_records)
        # Per flag: its candidates' edge sets and prior-scaled edge factors,
        # and the edge factors of its disjoint average (the root envelope).
        self.cand_edges: list[list[frozenset[int]]] = [[] for _ in range(num_flags)]
        self.cand_factors: list[list[tuple[NDArray[np.int32], NDArray[np.float64]]]] = [
            [] for _ in range(num_flags)
        ]
        self.env_factors: list[tuple[NDArray[np.int32], NDArray[np.float64]] | None] = [
            None
        ] * num_flags
        self.dominated = np.zeros(num_flags, dtype=bool)
        for f in range(num_flags):
            env = env_syms[f]
            if env is None:
                continue  # never an event (as in the marginal generator)
            for syms in cand_syms[f]:
                self.cand_edges[f].append(
                    frozenset(
                        int(e) for s in syms for e in ids[offs[s] : offs[s + 1]]
                    )
                )
                self.cand_factors[f].append(_edge_factors(syms, a))
            self.env_factors[f] = _edge_factors(env, a)
            union = frozenset().union(*self.cand_edges[f])
            self.dominated[f] = any(s == union for s in self.cand_edges[f])
        self.base_q = 1.0 - 2.0 * a.base_edge_probs
        self.placeholder = a.base_edge_probs == 0.0
        self.edge_id = {
            (int(u), int(v)): e for e, (u, v) in enumerate(a.edge_nodes.tolist())
        }

    def _warn_if_equivalent_to_marginal(self, circuit: stim.Circuit) -> None:
        eventful = [f for f, env in enumerate(self.env_factors) if env is not None]
        if not eventful:
            warnings.warn(
                "BranchAndBoundDecoder: the circuit has no leakage flags with "
                "candidate leaks; it decodes like MarginalDecoder.",
                stacklevel=3,
            )
        elif self.dominated[eventful].all():
            warnings.warn(
                "BranchAndBoundDecoder: every leakage flag has a candidate whose "
                "errors contain all the others' (no skip-gate style leakage?); "
                "it decodes like MarginalDecoder.",
                stacklevel=3,
            )
        for op in circuit.flattened():
            if op.name != "MPAD" or not op.tag:
                continue
            params = parse_leakage_tag(op, simulator="tableau")
            if isinstance(params, LeakageMeasurementParams) and params.targets:
                probs = params.prob_for_input_state
                if any((s < 2 and p > 0) or (s >= 2 and p < 1) for s, p in probs.items()):
                    warnings.warn(
                        f"BranchAndBoundDecoder assumes perfect leakage heralds; {op} "
                        "can miss leaks or raise false flags.",
                        stacklevel=3,
                    )
                    return

    def _events(self, raised: NDArray[np.bool_]) -> list[int]:
        """Raised flags with candidates not explained by a raised predecessor."""
        a = self.analysis
        events = []
        for f in np.flatnonzero(raised).tolist():
            if self.env_factors[f] is None:
                continue
            preds = a.pred_flags[a.pred_offsets[f] : a.pred_offsets[f + 1]]
            if not raised[preds].any():
                events.append(f)
        return events

    def _reweights(
        self, events: Sequence[int], cons: dict[int, int]
    ) -> tuple[NDArray[np.float64], NDArray[np.int32]]:
        """(edge_reweights of a node, its reweighted edge ids)."""
        parts = []
        for f in events:
            c = cons.get(f)
            env = self.env_factors[f] if c is None else self.cand_factors[f][c]
            assert env is not None
            parts.append(env)
        if not parts:
            return np.empty((0, 3), dtype=np.float64), np.empty(0, dtype=np.int32)
        edges, q = _reduce_factors(
            np.concatenate([p[0] for p in parts]), np.concatenate([p[1] for p in parts])
        )
        keep = q < 1.0
        edges, q = edges[keep], q[keep]
        p = 0.5 * (1.0 - q * self.base_q[edges])
        with np.errstate(divide="ignore"):
            w = np.where(p < 0.5, np.log((1.0 - p) / p), 0.0)
        w = np.clip(w, 0.0, _MAX_REWEIGHT)
        nodes = self.analysis.edge_nodes[edges]
        rows = np.empty((edges.size, 3), dtype=np.float64)
        rows[:, 0] = nodes[:, 0]
        rows[:, 1] = nodes[:, 1]
        rows[:, 2] = w
        return rows, edges

    def _matched_edges(self, dets: NDArray[np.uint8], rw: NDArray[np.float64]) -> frozenset[int]:
        pairs = self._get_matcher().decode_to_edges_array(dets, edge_reweights=rw)
        out = set()
        for u, v in pairs.tolist():
            key = (max(u, v), -1) if (u < 0 or v < 0) else (min(u, v), max(u, v))
            e = self.edge_id.get(key)
            if e is not None:
                out.add(e)
        return frozenset(out)

    def _decode_shot(
        self, dets: NDArray[np.uint8], events: list[int]
    ) -> tuple[NDArray[np.float64], str, dict[int, int], frozenset[int] | None]:
        """(accepted reweights, status, its constraints, its matched edges or None).

        Status is "root" (the marginal graph is valid), "child" (a constrained
        node is), or "fallback" (budget or queue exhausted: the root is used).
        """
        root_rw, root_edges = self._reweights(events, {})
        free = [f for f in events if not self.dominated[f]]
        if not free:
            return root_rw, "root", {}, None
        dominated_cover = frozenset().union(
            *(e for f in events if self.dominated[f] for e in self.cand_edges[f])
        )
        matcher = self._get_matcher()
        calls = 0
        tie = itertools.count()
        heap: list[tuple[float, int, dict[int, int], NDArray[np.float64], NDArray[np.int32]]]
        heap = [(0.0, next(tie), {}, root_rw, root_edges)]  # the root is popped first
        while heap:
            if calls >= self.max_mwpm_calls:
                self.stats["budget_exhausted"] += 1
                break
            _, _, cons, rw, rw_edges = heapq.heappop(heap)
            matched = self._matched_edges(dets, rw)
            calls += 1
            covered = dominated_cover.union(
                *(self.cand_edges[f][c] for f, c in cons.items())
            )
            open_flags = [f for f in free if f not in cons]
            rows = [self.cand_edges[f] for f in open_flags]
            valid, witness = _check_validity(matched, covered, rows)
            if valid:
                self.stats["mwpm_calls"] += calls
                reweighted = np.zeros(len(self.placeholder), dtype=bool)
                reweighted[rw_edges] = True
                if any(self.placeholder[e] and not reweighted[e] for e in matched):
                    self.stats["accepted_with_placeholder"] += 1
                return rw, ("child" if cons else "root"), cons, matched
            r = None
            if self.branching == "conflict":
                r = _conflict_branch(witness, matched, rows)
                if r is None:
                    self.stats["conflict_rule_empty"] += 1
            if r is None:
                r = _fewest_branch(witness, rows)
            assert r is not None  # witness edges are covered by some open row
            f = open_flags[r]
            children = []
            for c in range(len(self.cand_edges[f])):
                child = dict(cons)
                child[f] = c
                children.append((child, *self._reweights(events, child)))
            _, weights, no_matching = matcher.decode_batch(
                np.repeat(dets[np.newaxis, :], len(children), axis=0),
                return_weights=True,
                return_no_matching=True,
                edge_reweights=[ch[1] for ch in children],
            )
            calls += len(children)
            for ch, w, bad in zip(children, weights.tolist(), no_matching.tolist()):
                if not bad:
                    heapq.heappush(heap, (w, next(tie), *ch))
            if self.max_queue is not None and len(heap) > self.max_queue:
                heap = heapq.nsmallest(self.max_queue, heap)
        self.stats["mwpm_calls"] += calls
        self.stats["fallbacks"] += 1
        return root_rw, "fallback", {}, None

    def decode_shots_bit_packed(
        self,
        *,
        bit_packed_detection_event_data: NDArray[np.uint8],
        records: NDArray[np.bool_] | None = None,
        leakage_events: Sequence[ShotLeakageEvents] | None = None,
    ) -> NDArray[np.uint8]:
        if records is None:
            raise ValueError("BranchAndBoundDecoder requires records.")
        a = self.analysis
        rows = np.asarray(records, dtype=bool).reshape(-1, a.num_measurements)
        raised = rows[:, a.flag_records] ^ a.flag_invert
        dets = np.unpackbits(
            bit_packed_detection_event_data,
            axis=1,
            count=self.num_detectors,
            bitorder="little",
        )
        rw_list = []
        # One array per distinct accepted graph: a non-PyMatching decoder then
        # compiles once per graph and batch (_decode_with_edge_reweights groups
        # the shots by array identity).
        shared: dict[tuple[Any, ...], NDArray[np.float64]] = {}
        groups: dict[tuple[Any, ...], list[int]] = {}
        for s in range(rows.shape[0]):
            events = self._events(raised[s])
            rw, status, cons, _ = self._decode_shot(dets[s], events)
            self.stats["shots"] += 1
            self.stats[status] += 1
            key = (tuple(events), tuple(sorted(cons.items())))
            rw_list.append(shared.setdefault(key, rw))
            groups.setdefault(key, []).append(s)
        if self.hyper_analysis is not None:
            return self._decode_hyper(groups, bit_packed_detection_event_data)
        decoder = self._get_matcher() if _is_pymatching_decoder(self.decoder) else self.decoder
        return _decode_with_edge_reweights(
            decoder, rw_list, bit_packed_detection_event_data, base_dem=self.search_dem
        )

    def _hyper_dem(self, events: Sequence[int], cons: dict[int, int]) -> stim.DetectorErrorModel:
        """The accepted node's undecomposed DEM.

        As ``MarginalLeakageDemGenerator(decompose_errors=False)`` builds a
        shot's DEM (the baseline plus the xor over the events of their
        envelopes), with each constrained event's chosen candidate in place of
        its envelope.
        """
        hyper = self.hyper_analysis
        assert hyper is not None
        sym_probs: dict[int, float] = {}
        for f in events:
            c = cons.get(f)
            syms = self.hyper_env_syms[f] if c is None else self.hyper_cand_syms[f][c]
            assert syms is not None, f"event {f} has no undecomposed envelope"
            for sym, p in syms.items():
                q = sym_probs.get(sym)
                sym_probs[sym] = p if q is None else q + p - 2 * q * p
        sym_targets = hyper._get_sym_targets()
        dem = hyper.baseline.copy()
        for sym in sorted(sym_probs):
            if sym_probs[sym] > 0:
                dem.append("error", sym_probs[sym], sym_targets[sym])
        return dem

    def _decode_hyper(
        self, groups: dict[tuple[Any, ...], list[int]], bit_packed_dets: NDArray[np.uint8]
    ) -> NDArray[np.uint8]:
        out = np.zeros((bit_packed_dets.shape[0], (self.num_observables + 7) // 8), dtype=np.uint8)
        for (events, cons), shots in groups.items():
            dem = self._hyper_dem(events, dict(cons))
            compiled = self.decoder.compile_decoder_for_dem(dem=dem)
            out[shots] = compiled.decode_shots_bit_packed(
                bit_packed_detection_event_data=bit_packed_dets[shots]
            )
        return out
