"""Per-shot "marginal" leakage DEM generator for the stimside samplers.

This implements the DEM construction of the marginal decoder in
surface-code-leakage-erasure (Appendix B.3 of surface_code_leakage_erasure.pdf)
for circuits written with stimside's leakage tags:

* ``G_base`` is the DEM of the circuit with every qubit assumed unleaked.
* Every leakage source L (an ``LEAKAGE_TRANSITION_1/2`` branch that takes an
  unleaked qubit to a leaked state) has an "envelope" DEM ``G_L``: the noiseless
  circuit with depolarizing channels wherever the leaked qubit makes the
  simulated trajectory differ from the unleaked one (e.g. a skipped two-qubit
  gate, a role change, the partner of a leakage transition, unleaking, or a
  measurement or gate acting on a leaked qubit after gates skipped it). When
  the leak hops to the partner of a ``LEAKAGE_TRANSITION_2`` with probability
  p < 1, the hop is followed on a separate branch of weight p (only once per
  source: a branch that already hopped does not branch again).
* In each shot, a raised ``MPAD[LEAKAGE_MEASUREMENT...]`` flag that is not
  explained by an earlier raised flag on the same leakage flow is an
  erasure-check event E. Its DEM is the disjoint average
  ``G_E = sum_L P(L | E) G_L`` over the sources whose flow can raise it,
  with ``P(L | E)`` proportional to the probability that L happens, is still
  there at the flag and is first detected by it. Like the paper's ideal
  erasure checks, a raised flag is always attributed to a leak.
* Events combine independently, and the shot's DEM is
  ``G_base + (G_E1 xor G_E2 xor ...)``.

By default (``decompose_errors=False``), error mechanisms are never decomposed:
every mechanism keeps its full symptom (detectors and observables), so
hyperedges stay hyperedges. Setting ``decompose_errors=True`` decomposes the
DEM into matchable graphlike components using Stim's native decomposition APIs.
Setting ``reweight_only=True`` returns only the per-shot DEM reweight updates
(or, when both ``decompose_errors=True`` and ``reweight_only=True`` are set,
PyMatching ``edge_reweights`` arrays of shape ``(num_reweights, 3)``).
"""

from __future__ import annotations

from collections import defaultdict
import dataclasses
from typing import Any, Sequence

import numpy as np
from numpy.typing import NDArray
import stim  # type: ignore[import-untyped]

from stimside.dem_generators.dem_generator_base import DemGenerator
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageConditioningParams,
    LeakageMeasurementParams,
    LeakageSwapParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
)
from stimside.op_handlers.leakage_handlers.tag_registry import parse_leakage_tag
from stimside.util.leakage_events import (
    ShotLeakageEvents,
    unrolled_to_flattened_indices,
)
from stimside.util.marginal_dem_kernels import (
    CondFireRuleC,
    MOP_BARE_1Q_UNITARY,
    MOP_BARE_NOOP,
    MOP_BARE_OTHER,
    MOP_BARE_RESET,
    MOP_CONDITIONED,
    MOP_MEAS_FLAG,
    MOP_MEAS_PROJ_Z,
    MOP_NONE,
    MOP_TRANS_1,
    MOP_TRANS_2,
    MOP_UNTAGGED_1Q_UNITARY,
    MOP_UNTAGGED_2Q_UNITARY,
    MeasStateProbC,
    MoveBranchC,
    OpDescC,
    SourceDescC,
    Trans1RuleC,
    Trans2RuleC,
    build_envelopes_cpp,
    process_shots_cpp,
    trace_flows_cpp,
)

_ANNOTATIONS = frozenset(
    {"DETECTOR", "OBSERVABLE_INCLUDE", "QUBIT_COORDS", "SHIFT_COORDS", "TICK"}
)
_HERALDED = frozenset({"HERALDED_ERASE", "HERALDED_PAULI_CHANNEL_1"})
_RESET_BASIS = {"R": "Z", "MR": "Z", "RX": "X", "MRX": "X", "RY": "Y", "MRY": "Y"}
# For a controlled-Pauli gate, the eigenstate each leg must be in for the gate
# to act trivially (so skipping it is the same as applying it).
_CONTROL_BASIS = {
    "CX": "ZX",
    "CY": "ZY",
    "CZ": "ZZ",
    "XCX": "XX",
    "XCY": "XY",
    "XCZ": "XZ",
    "YCX": "YX",
    "YCY": "YY",
    "YCZ": "YZ",
}
_BASIS_TO_ID = {"X": 1, "Y": 2, "Z": 3}
_PAULI_STRINGS = (
    None,
    stim.PauliString("+X"),
    stim.PauliString("+Y"),
    stim.PauliString("+Z"),
    stim.PauliString("-X"),
    stim.PauliString("-Y"),
    stim.PauliString("-Z"),
)
_PAULI_TO_ID = {str(p): idx for idx, p in enumerate(_PAULI_STRINGS) if p is not None}

_ONE = 1 - 1e-9
_SOURCE_TAG = "marginal_leakage_source"
_ITEM_ANALYSIS_META: dict[
    int,
    tuple[
        Any,
        _Analysis,
        bool,
        bool,
        MarginalLeakageDemGenerator | None,
        stim.Circuit | None,
        NDArray[np.bool_] | None,
        ShotLeakageEvents | None,
    ],
] = {}


def _register_item_analysis(
    obj: Any,
    analysis: _Analysis,
    decompose_errors: bool,
    reweight_only: bool,
    generator: MarginalLeakageDemGenerator | None = None,
    circuit: stim.Circuit | None = None,
    record_1d: NDArray[np.bool_] | None = None,
    leakage_events_1d: ShotLeakageEvents | None = None,
) -> None:
    if len(_ITEM_ANALYSIS_META) >= 8192:
        oldest_key = next(iter(_ITEM_ANALYSIS_META))
        _ITEM_ANALYSIS_META.pop(oldest_key, None)
    _ITEM_ANALYSIS_META[id(obj)] = (
        obj,
        analysis,
        decompose_errors,
        reweight_only,
        generator,
        circuit,
        record_1d,
        leakage_events_1d,
    )


def _get_item_analysis_meta(
    obj: Any,
) -> tuple[
    _Analysis,
    bool,
    bool,
    MarginalLeakageDemGenerator | None,
    stim.Circuit | None,
    NDArray[np.bool_] | None,
] | None:
    entry = _ITEM_ANALYSIS_META.get(id(obj))
    if entry is not None and entry[0] is obj:
        return entry[1:7]
    return None


def _get_item_leakage_events(obj: Any) -> ShotLeakageEvents | None:
    """The leakage events of the shot an item was generated for (loss-oracle mode), else None."""
    entry = _ITEM_ANALYSIS_META.get(id(obj))
    if entry is not None and entry[0] is obj:
        return entry[7]
    return None


Symptom = tuple[tuple[int, ...], tuple[int, ...]]  # (detector ids, observable ids)


def _is_compound_symptom(sym: Any) -> bool:
    return (
        bool(sym)
        and isinstance(sym[0], tuple)
        and bool(sym[0])
        and isinstance(sym[0][0], tuple)
    )


def _symptom_components(sym: Any) -> tuple[Symptom, ...]:
    if _is_compound_symptom(sym):
        return sym
    if sym == ((), ()):
        return ()
    return (sym,)


def _canonical_symptom_from_components(comps: Sequence[Symptom]) -> Any:
    non_empty = [c for c in comps if c[0] or c[1]]
    if not non_empty:
        return ((), ())
    if len(non_empty) == 1:
        return non_empty[0]
    return tuple(sorted(non_empty))


def _sym_to_targets(sym: Any) -> list[stim.DemTarget]:
    comps = _symptom_components(sym)
    targets: list[stim.DemTarget] = []
    for idx, (dets, obs) in enumerate(comps):
        if idx > 0:
            targets.append(stim.target_separator())
        targets.extend(stim.target_relative_detector_id(d) for d in dets)
        targets.extend(stim.target_logical_observable_id(o) for o in obs)
    return targets


class _GeneratedDemList(list):
    """A list of per-shot DEMs or edge_reweights arrays carrying circuit/baseline metadata."""

    def __init__(
        self,
        items: Sequence[Any],
        *,
        analysis: _Analysis | None = None,
        generator: MarginalLeakageDemGenerator | None = None,
        circuit: stim.Circuit | None = None,
        records: NDArray[np.bool_] | None = None,
        decompose_errors: bool = False,
        reweight_only: bool = False,
        leakage_events: Sequence[ShotLeakageEvents] | None = None,
    ) -> None:
        super().__init__(items)
        self.analysis = analysis
        self.base_dem = analysis.baseline if analysis is not None else None
        self.matching_base_dem = (
            analysis.matching_base_dem if analysis is not None else None
        )
        self._generator = generator
        self._circuit = circuit
        self._records = records
        self.decompose_errors = decompose_errors
        self.reweight_only = reweight_only
        self.leakage_events = leakage_events

    def __getitem__(self, index: Any) -> Any:
        res = super().__getitem__(index)
        if isinstance(index, slice):
            return _GeneratedDemList(
                res,
                analysis=self.analysis,
                generator=self._generator,
                circuit=self._circuit,
                records=self._records[index] if self._records is not None else None,
                decompose_errors=self.decompose_errors,
                reweight_only=self.reweight_only,
                leakage_events=(
                    self.leakage_events[index]
                    if self.leakage_events is not None
                    else None
                ),
            )
        return res


def _is_leaked(state) -> bool:
    return isinstance(state, int) and state >= 2


def _pauli_dist(out) -> NDArray[np.float64]:
    """[I, X, Y, Z] probabilities applied to an unleaked qubit by an unleaked output."""
    if out == "U":
        return np.array([1.0, 0.0, 0.0, 0.0])
    if out in ("X", "Y", "Z"):
        dist = np.zeros(4)
        dist["IXYZ".index(out)] = 1.0
        return dist
    # D, V and resets to 0/1 all leave the qubit maximally mixed (after twirling).
    return np.full(4, 0.25)


def _allowed(slot, state) -> bool:
    if slot == "U":
        return state is None or state < 2  # classical (rec) targets count as unleaked
    return state == slot


def _fires(params: LeakageConditioningParams, states: Sequence[int | None]) -> bool:
    """Whether a conditioning group with the given subject leakage states fires."""
    if len(params.args) == 1:
        return all(any(_allowed(slot, st) for slot in params.args[0]) for st in states)
    first, second = params.args
    return any(
        _allowed(a, states[0]) and _allowed(b, states[1]) for a, b in zip(first, second)
    )


def _condition_groups(
    op: stim.CircuitInstruction, params: LeakageConditioningParams
) -> list[tuple[list[stim.GateTarget], list[int | None]]]:
    """Split a conditioned op into (targets, condition subjects) groups.

    The tag registry only allows CONDITIONED_ON_SELF/OTHER on 1Q gates and
    CONDITIONED_ON_PAIR on 2Q gates.
    """
    targets = op.targets_copy()
    if params.targets is not None:  # CONDITIONED_ON_OTHER
        if len(targets) != len(params.targets):
            raise ValueError(
                f"The number of targets of {op} does not match the number of "
                "targets in its CONDITIONED_ON_OTHER tag."
            )
        return [([t], [q]) for t, q in zip(targets, params.targets)]
    if len(params.args) == 2:  # CONDITIONED_ON_PAIR
        return [
            (targets[k : k + 2], [t.qubit_value for t in targets[k : k + 2]])
            for k in range(0, len(targets), 2)
        ]
    return [([t], [t.qubit_value]) for t in targets]  # CONDITIONED_ON_SELF


def _check_supported(params) -> None:
    """Reject tags whose simulated effect depends on the computational state, and LEAKAGE_SWAP."""
    if isinstance(params, LeakageSwapParams):
        raise NotImplementedError(
            "MarginalLeakageDemGenerator (and so MarginalDecoder, BranchAndBoundDecoder and the "
            "loss oracle) does not support SWAP[LEAKAGE_SWAP]: it does not model leakage moving "
            "between qubits."
        )
    if isinstance(params, LeakageTransition1Params):
        bad = any(_is_int_below_2(s) for s in params.args_by_input_state)
    elif isinstance(params, LeakageTransition2Params):
        bad = any(_is_int_below_2(s) for key in params.args_by_input_state for s in key)
    elif isinstance(params, LeakageConditioningParams):
        parts = params.from_tag.split(":")
        bad = any(_is_int_below_2(s) for group in params.args for s in group) or (
            len(parts) > 1 and any(tok in ("0", "1") for tok in parts[1].split())
        )
    else:
        bad = False
    if bad:
        raise NotImplementedError(
            "MarginalLeakageDemGenerator does not support leakage tags that depend on "
            f"the computational state 0/1 ({params.from_tag!r}); use 'U' instead."
        )


def _is_int_below_2(state) -> bool:
    return isinstance(state, int) and state < 2


def _stripped(op: stim.CircuitInstruction, args=None) -> stim.CircuitInstruction:
    """The op without its tag (and optionally with replaced gate args)."""
    return stim.CircuitInstruction(
        op.name, op.targets_copy(), op.gate_args_copy() if args is None else args
    )


def _symptom(inst: stim.DemInstruction) -> Symptom:
    dets: set[int] = set()
    obs: set[int] = set()
    for t in inst.targets_copy():
        if t.is_relative_detector_id():
            dets ^= {t.val}
        elif t.is_logical_observable_id():
            obs ^= {t.val}
    return tuple(sorted(dets)), tuple(sorted(obs))


def _raw_inst_components(inst: stim.DemInstruction) -> list[Symptom]:
    """Extract separator-delimited (detectors, observables) components of a DEM instruction."""
    comps: list[Symptom] = []
    dets: set[int] = set()
    obs: set[int] = set()
    for t in inst.targets_copy():
        if t.is_separator():
            if dets or obs:
                comps.append((tuple(sorted(dets)), tuple(sorted(obs))))
            dets.clear()
            obs.clear()
        elif t.is_relative_detector_id():
            dets ^= {t.val}
        elif t.is_logical_observable_id():
            obs ^= {t.val}
    if dets or obs:
        comps.append((tuple(sorted(dets)), tuple(sorted(obs))))
    return comps


def _collect_known_graphlike(
    *dems: stim.DetectorErrorModel | None,
) -> dict[tuple[int, ...], Symptom]:
    """Collect known 1- and 2-detector components from DEM(s) for hyperedge decomposition."""
    known: dict[tuple[int, ...], Symptom] = {}
    for dem in dems:
        if dem is None:
            continue
        for inst in dem.flattened():
            if inst.type != "error":
                continue
            args = inst.args_copy()
            if not args or args[0] <= 0:
                continue
            for dets, obs in _raw_inst_components(inst):
                if 1 <= len(dets) <= 2 and dets not in known:
                    known[dets] = (dets, obs)
    return known


def _decompose_hyperedge_component(
    dets: tuple[int, ...],
    obs: tuple[int, ...],
    known_graphlike: dict[tuple[int, ...], Symptom] | None = None,
) -> list[Symptom]:
    """Decompose a >2-detector component into <=2-detector components using Stim's algorithm."""
    n = len(dets)
    if n == 0:
        return []
    if n <= 2:
        return [(dets, obs)]

    if known_graphlike:
        target_obs = set(obs)
        out: list[Symptom] = []

        def _search(start: int, used_mask: int, cur_obs: set[int]) -> bool:
            while start < n and ((used_mask >> start) & 1):
                start += 1
            if start >= n:
                return cur_obs == target_obs
            used_mask |= 1 << start
            d0 = dets[start]
            for k in range(start + 1, n + 1):
                if k < n:
                    if (used_mask >> k) & 1:
                        continue
                    key: tuple[int, ...] = (d0, dets[k])
                    next_mask = used_mask | (1 << k)
                else:
                    key = (d0,)
                    next_mask = used_mask
                match = known_graphlike.get(key)
                if match is not None:
                    if _search(start + 1, next_mask, cur_obs ^ set(match[1])):
                        out.append(match)
                        return True
            return False

        if n < 64 and _search(0, 0, set()):
            out.reverse()
            return out

        # Fallback: find a disjoint subset of known graphlike edges covering the
        # maximum number of detectors (minimizing missed detectors), then add
        # remnant edge(s) for any remaining missed detectors.
        if n <= 16:
            best_missed = n + 1
            best_peeled: list[Symptom] = []
            best_missed_dets: list[int] = []
            cur_peeled: list[Symptom] = []
            cur_missed_dets: list[int] = []

            def _search_cover(start: int, used_mask: int) -> None:
                nonlocal best_missed, best_peeled, best_missed_dets
                if len(cur_missed_dets) >= best_missed:
                    return
                while start < n and ((used_mask >> start) & 1):
                    start += 1
                if start >= n:
                    best_missed = len(cur_missed_dets)
                    best_peeled = list(cur_peeled)
                    best_missed_dets = list(cur_missed_dets)
                    return
                used_mask |= 1 << start
                d0 = dets[start]
                for k in range(start + 1, n):
                    if (used_mask >> k) & 1:
                        continue
                    match = known_graphlike.get((d0, dets[k]))
                    if match is not None:
                        cur_peeled.append(match)
                        _search_cover(start + 1, used_mask | (1 << k))
                        cur_peeled.pop()
                        if best_missed == 0:
                            return
                match_single = known_graphlike.get((d0,))
                if match_single is not None:
                    cur_peeled.append(match_single)
                    _search_cover(start + 1, used_mask)
                    cur_peeled.pop()
                    if best_missed == 0:
                        return
                if len(cur_missed_dets) + 1 < best_missed:
                    cur_missed_dets.append(d0)
                    _search_cover(start + 1, used_mask)
                    cur_missed_dets.pop()

            _search_cover(0, 0)
            peeled = best_peeled
            missed = tuple(best_missed_dets)
            rem_obs = set(obs)
            for _, o in peeled:
                rem_obs ^= set(o)
        else:
            done = [False] * n
            rem_obs = set(obs)
            peeled = []
            for k in range(n):
                if not done[k]:
                    for k2 in range(k + 1, n):
                        if not done[k2]:
                            match = known_graphlike.get((dets[k], dets[k2]))
                            if match is not None:
                                done[k] = done[k2] = True
                                peeled.append(match)
                                rem_obs ^= set(match[1])
                                break
            for k in range(n):
                if not done[k]:
                    match = known_graphlike.get((dets[k],))
                    if match is not None:
                        done[k] = True
                        peeled.append(match)
                        rem_obs ^= set(match[1])
            missed = tuple(dets[k] for k in range(n) if not done[k])

        if len(missed) <= 2:
            if missed:
                peeled.append((missed, tuple(sorted(rem_obs))))
            elif rem_obs and peeled:
                d0, o0 = peeled[0]
                peeled[0] = (d0, tuple(sorted(set(o0) ^ rem_obs)))
            return peeled
        for idx in range(0, len(missed), 2):
            peeled.append(
                (missed[idx : idx + 2], tuple(sorted(rem_obs)) if idx == 0 else ())
            )
        return peeled

    return [
        (dets[idx : idx + 2], obs if idx == 0 else ()) for idx in range(0, n, 2)
    ]


def _graphlike_components(
    inst: stim.DemInstruction,
    known_graphlike: dict[tuple[int, ...], Symptom] | None = None,
) -> list[Symptom]:
    """Extract graphlike (<=2 detector) symptom components from a DEM error instruction."""
    comps: list[Symptom] = []
    for d_sorted, o_sorted in _raw_inst_components(inst):
        if len(d_sorted) <= 2:
            if d_sorted:
                comps.append((d_sorted, o_sorted))
        else:
            comps.extend(
                _decompose_hyperedge_component(d_sorted, o_sorted, known_graphlike)
            )
    return comps


def _xor(p: float, q: float) -> float:
    """Probability that exactly one of two independent mechanisms fires."""
    return p + q - 2 * p * q


@dataclasses.dataclass
class _Source:
    op_index: int
    group: int  # the target (LEAKAGE_TRANSITION_1) or target pair (_2) of the op that leaks
    qubit: int
    state: int
    weight: float
    partner: int | None = None
    partner_channel: tuple[float, ...] | None = None  # PAULI_CHANNEL_1 args


@dataclasses.dataclass
class _Analysis:
    """Per-circuit data needed to build per-shot DEMs (plain, picklable data)."""

    num_measurements: int
    baseline: stim.DetectorErrorModel
    matching_base_dem: stim.DetectorErrorModel
    flag_records: NDArray[np.intp]
    flag_invert: NDArray[np.bool_]
    pred_offsets: NDArray[np.int32]
    pred_flags: NDArray[np.int32]
    flag_env_offsets: NDArray[np.int32]
    flag_env_sym_ids: NDArray[np.int32]
    flag_env_probs: NDArray[np.float64]
    symptoms: list[Any]
    base_sym_probs: NDArray[np.float64]
    sym_edge_offsets: NDArray[np.int32]
    sym_edge_ids: NDArray[np.int32]
    edge_nodes: NDArray[np.int32]
    base_edge_probs: NDArray[np.float64]
    # Loss-oracle analyses: the "flags" are the leakage sources, raised from a
    # shot's leakage events via these lookups (see `_match_shot_sources`).
    oracle: bool = False
    source_lookup: dict[tuple[int, int, int], int] = dataclasses.field(
        default_factory=dict
    )  # (flattened op index, qubit, leaked state) -> source index
    lt2_partner: dict[tuple[int, int], int] = dataclasses.field(
        default_factory=dict
    )  # (flattened op index of a LEAKAGE_TRANSITION_2, qubit) -> its pair partner
    unrolled_to_flat: NDArray[np.int64] | None = None
    # Kept only by `_Builder.build(keep_candidates=True)` (else None): the
    # inputs of `build_envelopes_cpp`, i.e. each flag's candidate flows and
    # their (unnormalized) weights, each flow's sites, and each site's
    # (symptom id, probability) entries.
    flag_cand_offsets: NDArray[np.int32] | None = None
    flag_cand_flows: NDArray[np.int32] | None = None
    flag_cand_weights: NDArray[np.float64] | None = None
    flow_site_offsets: NDArray[np.int32] | None = None
    flow_site_ids: NDArray[np.int32] | None = None
    entry_site_ids: NDArray[np.int32] | None = None
    entry_sym_ids: NDArray[np.int32] | None = None
    entry_probs: NDArray[np.float64] | None = None
    _sym_targets: list[list[stim.DemTarget]] | None = dataclasses.field(
        default=None, init=False, repr=False, compare=False
    )
    _empty_reweight: NDArray[np.float64] | None = dataclasses.field(
        default=None, init=False, repr=False, compare=False
    )
    _cached_matcher: Any = dataclasses.field(
        default=None, init=False, repr=False, compare=False
    )

    def __getstate__(self) -> dict[str, Any]:
        state = self.__dict__.copy()
        state["_sym_targets"] = None
        state["_empty_reweight"] = None
        state["_cached_matcher"] = None
        return state

    def _get_sym_targets(self) -> list[list[stim.DemTarget]]:
        if self._sym_targets is None:
            self._sym_targets = [_sym_to_targets(sym) for sym in self.symptoms]
        return self._sym_targets

    def _get_empty_reweight(self) -> NDArray[np.float64]:
        if self._empty_reweight is None:
            self._empty_reweight = np.empty((0, 3), dtype=np.float64)
        return self._empty_reweight

    def generate_batch(
        self,
        raised: NDArray[np.bool_],
        *,
        decompose_errors: bool,
        reweight_only: bool,
    ) -> list[Any]:
        num_shots, num_flags = raised.shape
        mode = 2 if (decompose_errors and reweight_only) else (1 if reweight_only else 0)

        if num_flags == 0 or len(self.symptoms) == 0:
            if mode == 2:
                empty_rw = self._get_empty_reweight()
                _register_item_analysis(empty_rw, self, decompose_errors, reweight_only)
                return [empty_rw] * num_shots
            if mode == 1:
                empty_dem = stim.DetectorErrorModel()
                _register_item_analysis(empty_dem, self, decompose_errors, reweight_only)
                return [empty_dem] * num_shots
            base_copy = self.baseline.copy()
            _register_item_analysis(base_copy, self, decompose_errors, reweight_only)
            return [base_copy] * num_shots

        (
            shot_group_ids,
            group_item_offsets,
            group_sym_ids,
            group_probs,
            group_reweights,
        ) = process_shots_cpp(
            raised=np.ascontiguousarray(raised, dtype=np.uint8),
            pred_offsets=self.pred_offsets,
            pred_flags=self.pred_flags,
            flag_env_offsets=self.flag_env_offsets,
            flag_env_sym_ids=self.flag_env_sym_ids,
            flag_env_probs=self.flag_env_probs,
            num_symptoms=len(self.symptoms),
            mode=mode,
            base_sym_probs=self.base_sym_probs if mode == 1 else None,
            sym_edge_offsets=self.sym_edge_offsets if mode == 2 else None,
            sym_edge_ids=self.sym_edge_ids if mode == 2 else None,
            edge_nodes=self.edge_nodes if mode == 2 else None,
            base_edge_probs=self.base_edge_probs if mode == 2 else None,
        )

        num_groups = len(group_item_offsets) - 1
        if mode == 2:
            assert group_reweights is not None
            empty_rw = self._get_empty_reweight()
            _register_item_analysis(empty_rw, self, decompose_errors, reweight_only)
            group_objs: list[Any] = []
            for g in range(num_groups):
                start = int(group_item_offsets[g])
                end = int(group_item_offsets[g + 1])
                if start == end:
                    group_objs.append(empty_rw)
                else:
                    arr = group_reweights[start:end].copy()
                    _register_item_analysis(arr, self, decompose_errors, reweight_only)
                    group_objs.append(arr)
            return [group_objs[int(gid)] for gid in shot_group_ids]

        assert group_sym_ids is not None and group_probs is not None
        sym_targets = self._get_sym_targets()
        group_dems: list[stim.DetectorErrorModel] = []
        empty_base = (
            stim.DetectorErrorModel() if mode == 1 else self.baseline.copy()
        )
        _register_item_analysis(empty_base, self, decompose_errors, reweight_only)
        for g in range(num_groups):
            start = int(group_item_offsets[g])
            end = int(group_item_offsets[g + 1])
            if start == end:
                group_dems.append(empty_base)
                continue
            extra = stim.DetectorErrorModel()
            for idx in range(start, end):
                sym_id = int(group_sym_ids[idx])
                p = float(group_probs[idx])
                if p > 0:
                    extra.append("error", p, sym_targets[sym_id])
            dem_obj = extra if mode == 1 else (self.baseline + extra)
            _register_item_analysis(dem_obj, self, decompose_errors, reweight_only)
            group_dems.append(dem_obj)
        return [group_dems[int(gid)] for gid in shot_group_ids]


class MarginalLeakageDemGenerator(DemGenerator):
    """Per-shot DEM generator implementing the marginal leakage decoder's DEMs.

    Pass an instance to the samplers as ``MarginalDecoder(dem_gen=...)`` (the
    ``dem_decoder`` of ``TablesideSampler``, ``CosetsideSampler``, or
    ``FlipsideSampler``). Called with a circuit and its
    ``(shots, measurements)`` measurement records, it returns one DEM per shot
    (a single DEM for 1D records), built from the ``MPAD[LEAKAGE_MEASUREMENT...]``
    flags of that shot.

    Args:
        unconditional_condition_on_U: must match the op handler's flag of the
            same name (``LeakageUint8`` and ``LeakageUint8Coset`` default to
            True). It decides whether untagged gates skip leaked qubits.
        decompose_errors: if True, decompose the DEM into matchable graphlike
            components (<=2 detectors per component) using Stim's native APIs.
        reweight_only: if True, produce only the DEM reweight update instead of
            the full DEM. When both ``decompose_errors=True`` and
            ``reweight_only=True``, produces PyMatching ``edge_reweights``
            arrays of shape ``(num_reweights, 3)`` with rows
            ``[node1, node2, weight]``.
        loss_oracle: if True, build each shot's DEM from the shot's true
            leakage events (``leakage_events=``, recorded by the simulator)
            instead of its leakage flags: ``G_base + (G_L1 xor G_L2 xor ...)``
            over the sources L that actually leaked in the shot, each included
            with weight 1 (heralds / ``MPAD`` flags are ignored). A shot without
            leakage gets ``G_base``. ``G_L`` is L's full envelope: the disjoint
            mixture of L's root flow and its hop branches, where a branch forked
            at a ``LEAKAGE_TRANSITION_2`` op into leaked state m has weight
            P(the leak is still on the root's qubit just before that op) * p_m
            (with the unleak / move factors of the weighted model) and the root
            has weight 1 - (sum of the branch weights). A source is the
            unleaked-to-leaked transition of a ``LEAKAGE_TRANSITION_1`` target or
            of a ``LEAKAGE_TRANSITION_2`` pair whose partner was unleaked before
            the op; an unleaked-to-leaked event of a ``LEAKAGE_TRANSITION_2`` target
            whose partner was already leaked before the op (a hop target or a
            spread) is not a source and is ignored (its effect is part of the
            leaked partner's source's envelope). Raises NotImplementedError for
            circuits with a qubit listed twice in one leakage transition, and
            ValueError for an unleaked-to-leaked event that matches no source.
    """

    def __init__(
        self,
        unconditional_condition_on_U: bool = True,
        decompose_errors: bool = False,
        reweight_only: bool = False,
        loss_oracle: bool = False,
    ) -> None:
        super().__init__(
            unconditional_condition_on_U=unconditional_condition_on_U,
            decompose_errors=decompose_errors,
            reweight_only=reweight_only,
            loss_oracle=loss_oracle,
        )
        self._circuit: stim.Circuit | None = None
        self._analysis: _Analysis | None = None
        self._analyses: dict[bool, _Analysis] = {}

    def _get_analysis(
        self, circuit: stim.Circuit, decompose_errors: bool
    ) -> _Analysis:
        if self._circuit is None or circuit != self._circuit:
            self._circuit = circuit.copy()
            self._analyses = {}
            self._analysis = None
        if decompose_errors not in self._analyses:
            self._analyses[decompose_errors] = _Builder(
                circuit, self.unconditional_condition_on_U
            ).build(decompose_errors=decompose_errors, oracle=self.loss_oracle)
        analysis = self._analyses[decompose_errors]
        self._analysis = analysis
        return analysis

    def base_dem(
        self, circuit: stim.Circuit, *, decompose_errors: bool | None = None
    ) -> stim.DetectorErrorModel:
        """Return the baseline DEM for ``circuit`` (matchable when ``decompose_errors=True``)."""
        dec = self.decompose_errors if decompose_errors is None else bool(decompose_errors)
        analysis = self._get_analysis(circuit, dec)
        return (
            analysis.matching_base_dem.copy()
            if dec
            else analysis.baseline.copy()
        )

    def __call__(
        self,
        circuit: stim.Circuit,
        records: NDArray[np.bool_],
        decompose_errors: bool | None = None,
        reweight_only: bool | None = None,
        *,
        leakage_events: Sequence[ShotLeakageEvents] | ShotLeakageEvents | None = None,
    ) -> (
        stim.DetectorErrorModel
        | list[stim.DetectorErrorModel]
        | NDArray[np.float64]
        | list[NDArray[np.float64]]
    ):
        dec = self.decompose_errors if decompose_errors is None else bool(decompose_errors)
        rew = self.reweight_only if reweight_only is None else bool(reweight_only)
        analysis = self._get_analysis(circuit, dec)

        records_arr = np.asarray(records, dtype=bool)
        if (
            records_arr.ndim not in (1, 2)
            or records_arr.shape[-1] != analysis.num_measurements
        ):
            raise ValueError(
                f"Expected records of shape (shots, {analysis.num_measurements}) or "
                f"({analysis.num_measurements},), got {records_arr.shape}."
            )
        rows = records_arr.reshape(-1, records_arr.shape[-1])
        shot_events: list[ShotLeakageEvents] | None = None
        if analysis.oracle:
            if leakage_events is None:
                raise ValueError(
                    "MarginalLeakageDemGenerator(loss_oracle=True) needs the shots' "
                    "leakage_events (record them with the simulator's "
                    "record_leakage_events=True)."
                )
            shot_events = (
                [leakage_events]  # type: ignore[list-item]
                if records_arr.ndim == 1
                else list(leakage_events)  # type: ignore[arg-type]
            )
            if len(shot_events) != rows.shape[0]:
                raise ValueError(
                    f"Got leakage_events for {len(shot_events)} shots but records "
                    f"for {rows.shape[0]} shots."
                )
            num_sources = len(analysis.flag_env_offsets) - 1
            raised = np.zeros((rows.shape[0], num_sources), dtype=bool)
            for shot_idx, evs in enumerate(shot_events):
                matched, _ = _match_shot_sources(analysis, evs)
                raised[shot_idx, matched] = True
        else:
            raised = rows[:, analysis.flag_records] ^ analysis.flag_invert

        items = analysis.generate_batch(
            raised, decompose_errors=dec, reweight_only=rew
        )
        assert self._circuit is not None
        if records_arr.ndim == 1:
            single_out = items[0].copy() if isinstance(items[0], np.ndarray) else items[0]
            _register_item_analysis(
                single_out,
                analysis,
                dec,
                rew,
                generator=self,
                circuit=self._circuit,
                record_1d=records_arr,
                leakage_events_1d=shot_events[0] if shot_events is not None else None,
            )
            return single_out
        seen_ids: set[int] = set()
        for shot_idx, item_obj in enumerate(items):
            oid = id(item_obj)
            if oid not in seen_ids:
                seen_ids.add(oid)
                _register_item_analysis(
                    item_obj,
                    analysis,
                    dec,
                    rew,
                    generator=self,
                    circuit=self._circuit,
                    record_1d=rows[shot_idx],
                    leakage_events_1d=(
                        shot_events[shot_idx] if shot_events is not None else None
                    ),
                )
        return _GeneratedDemList(
            items,
            analysis=analysis,
            generator=self,
            circuit=self._circuit,
            records=records_arr,
            decompose_errors=dec,
            reweight_only=rew,
            leakage_events=shot_events,
        )


def _match_shot_sources(
    analysis: _Analysis, events: ShotLeakageEvents
) -> tuple[list[int], int]:
    """(sorted indices of the sources that leaked in a shot, number of ignored events).

    Replays the shot's events op by op (stable-sorted by op index). An
    unleaked-to-leaked event at a ``LEAKAGE_TRANSITION_2`` op whose pair partner
    was leaked before the op (a hop target or a spread) is ignored and counted;
    every other unleaked-to-leaked event must match a source.
    """
    assert analysis.unrolled_to_flat is not None
    u2f = analysis.unrolled_to_flat
    ordered = sorted(events, key=lambda ev: ev.op_index)
    leaked: dict[int, int] = {}
    matched: set[int] = set()
    ignored = 0
    i = 0
    while i < len(ordered):
        op_index = ordered[i].op_index
        j = i
        while j < len(ordered) and ordered[j].op_index == op_index:
            j += 1
        group = ordered[i:j]
        for ev in group:
            if not (ev.old_state < 2 <= ev.new_state):
                continue
            flat_j = int(u2f[op_index]) if 0 <= op_index < len(u2f) else -1
            partner = analysis.lt2_partner.get((flat_j, ev.qubit))
            if partner is not None and leaked.get(partner, 0) >= 2:
                ignored += 1
                continue
            src = analysis.source_lookup.get((flat_j, ev.qubit, ev.new_state))
            if src is None:
                raise ValueError(f"Leakage event {ev} matches no leakage source of the circuit.")
            matched.add(src)
        for ev in group:
            if ev.new_state >= 2:
                leaked[ev.qubit] = ev.new_state
            else:
                leaked.pop(ev.qubit, None)
        i = j
    return sorted(matched), ignored


class _Builder:
    """Analyses a circuit once; only the resulting _Analysis is kept."""

    def __init__(self, circuit: stim.Circuit, uc: bool) -> None:
        self.uc = uc
        self.circuit = circuit
        self.ops = list(circuit.flattened())
        parsed: dict[tuple[str, str], object] = {}
        self.params = []
        for op in self.ops:
            if uc and op.name in ("SPP", "SPP_DAG"):
                raise NotImplementedError(
                    f"MarginalLeakageDemGenerator does not support {op.name} with "
                    "unconditional_condition_on_U=True: it does not model a Pauli product term "
                    "that touches a leaked qubit."
                )
            key = (op.name, op.tag)
            if key not in parsed:
                parsed[key] = parse_leakage_tag(op, simulator="tableau")
                _check_supported(parsed[key])
            self.params.append(parsed[key])
        self.gate_data = {name: stim.gate_data(name) for name, _ in parsed}

    def build(
        self,
        decompose_errors: bool = False,
        oracle: bool = False,
        keep_candidates: bool = False,
    ) -> _Analysis:
        if oracle:
            self._check_oracle_supported()
        base = stim.Circuit()
        self.ref_ops: list[stim.CircuitInstruction | None] = []
        self.base_ops: list[stim.CircuitInstruction | None] = []
        for op, params in zip(self.ops, self.params):
            base_op, ref_op = self._base_and_ref(op, params)
            if base_op is not None:
                base.append(base_op)
            self.base_ops.append(base_op)
            self.ref_ops.append(ref_op)

        if decompose_errors:
            try:
                baseline = base.detector_error_model(
                    decompose_errors=True, approximate_disjoint_errors=True
                )
            except ValueError:
                baseline = base.detector_error_model(
                    decompose_errors=True,
                    approximate_disjoint_errors=True,
                    ignore_decomposition_failures=True,
                )
        else:
            baseline = base.detector_error_model(
                decompose_errors=False, approximate_disjoint_errors=True
            )

        flag_records, flag_invert = self._find_flags()
        self._build_touch_lists()
        sources = self._find_sources()
        # In loss-oracle mode the "flags" of the analysis are the sources.
        num_flags = len(sources) if oracle else len(flag_records)

        (
            sites,
            flow_site_offsets,
            flow_site_ids,
            flag_cand_offsets,
            flag_cand_flows,
            flag_cand_weights,
            pred_offsets,
            pred_flags,
            channel_list,
        ) = self._trace_flows_fast(sources, num_flags, source_mode=oracle)

        num_sites = sites.shape[0]
        num_used_flows = len(flow_site_offsets) - 1
        if num_sites > 0 and num_used_flows > 0:
            (
                symptoms,
                entry_site_ids,
                entry_sym_ids,
                entry_probs,
            ) = self._propagate_unique_sites(
                sites,
                channel_list,
                decompose_errors=decompose_errors,
                baseline=baseline,
            )
            num_symptoms = len(symptoms)
            flag_env_offsets, flag_env_sym_ids, flag_env_probs = build_envelopes_cpp(
                num_sites=num_sites,
                num_symptoms=num_symptoms,
                entry_site_ids=entry_site_ids,
                entry_sym_ids=entry_sym_ids,
                entry_probs=entry_probs,
                num_used_flows=num_used_flows,
                flow_site_offsets=flow_site_offsets,
                flow_site_ids=flow_site_ids,
                num_flags=num_flags,
                flag_cand_offsets=flag_cand_offsets,
                flag_cand_flows=flag_cand_flows,
                flag_cand_weights=flag_cand_weights,
            )
        else:
            symptoms = []
            flag_env_offsets = np.zeros(num_flags + 1, dtype=np.int32)
            flag_env_sym_ids = np.empty(0, dtype=np.int32)
            flag_env_probs = np.empty(0, dtype=np.float64)

        if decompose_errors:
            has_hyperedge = any(
                inst.type == "error"
                and any(len(d) > 2 for d, _ in _raw_inst_components(inst))
                for inst in baseline.flattened()
            )
            if has_hyperedge:
                known_base = _collect_known_graphlike(baseline)
                for sym in symptoms:
                    for dets, obs in _symptom_components(sym):
                        if 1 <= len(dets) <= 2 and dets not in known_base:
                            known_base[dets] = (dets, obs)
                decomposed_base = stim.DetectorErrorModel()
                for inst in baseline.flattened():
                    if inst.type != "error":
                        decomposed_base.append(inst)
                        continue
                    p = float(inst.args_copy()[0])
                    raw_comps = _raw_inst_components(inst)
                    if any(len(d) > 2 for d, _ in raw_comps):
                        comps = _graphlike_components(inst, known_base)
                    else:
                        comps = [(d, o) for d, o in raw_comps if d]
                    sym = _canonical_symptom_from_components(comps)
                    if sym != ((), ()):
                        decomposed_base.append("error", p, _sym_to_targets(sym))
                baseline = decomposed_base

        (
            base_sym_probs,
            sym_edge_offsets,
            sym_edge_ids,
            edge_nodes,
            base_edge_probs,
            matching_base_dem,
        ) = self._build_reweight_tables(
            baseline, symptoms, decompose_errors=decompose_errors
        )

        oracle_fields: dict[str, Any] = {}
        if oracle:
            oracle_fields = dict(
                oracle=True,
                source_lookup={
                    (src.op_index, int(src.qubit), int(src.state)): idx
                    for idx, src in enumerate(sources)
                },
                lt2_partner=self._lt2_partners(),
                unrolled_to_flat=unrolled_to_flattened_indices(self.circuit),
            )
        cand_fields: dict[str, Any] = {}
        if keep_candidates:
            has_entries = num_sites > 0 and num_used_flows > 0
            cand_fields = dict(
                flag_cand_offsets=flag_cand_offsets,
                flag_cand_flows=flag_cand_flows,
                flag_cand_weights=flag_cand_weights,
                flow_site_offsets=flow_site_offsets,
                flow_site_ids=flow_site_ids,
                entry_site_ids=(
                    entry_site_ids if has_entries else np.empty(0, dtype=np.int32)
                ),
                entry_sym_ids=(
                    entry_sym_ids if has_entries else np.empty(0, dtype=np.int32)
                ),
                entry_probs=(
                    entry_probs if has_entries else np.empty(0, dtype=np.float64)
                ),
            )
        return _Analysis(
            num_measurements=self.num_measurements,
            baseline=baseline,
            matching_base_dem=matching_base_dem,
            flag_records=np.array(flag_records, dtype=np.intp),
            flag_invert=np.array(flag_invert, dtype=bool),
            pred_offsets=pred_offsets,
            pred_flags=pred_flags,
            flag_env_offsets=flag_env_offsets,
            flag_env_sym_ids=flag_env_sym_ids,
            flag_env_probs=flag_env_probs,
            symptoms=symptoms,
            base_sym_probs=base_sym_probs,
            sym_edge_offsets=sym_edge_offsets,
            sym_edge_ids=sym_edge_ids,
            edge_nodes=edge_nodes,
            base_edge_probs=base_edge_probs,
            **oracle_fields,
            **cand_fields,
        )

    def _check_oracle_supported(self) -> None:
        for op, params in zip(self.ops, self.params):
            if isinstance(params, (LeakageTransition1Params, LeakageTransition2Params)):
                qubits = [t.qubit_value for t in op.targets_copy()]
                if len(set(qubits)) != len(qubits):
                    raise NotImplementedError(
                        f"loss_oracle=True does not support a qubit listed twice in one "
                        f"leakage transition: {op}"
                    )

    def _lt2_partners(self) -> dict[tuple[int, int], int]:
        partners: dict[tuple[int, int], int] = {}
        for j, (op, params) in enumerate(zip(self.ops, self.params)):
            if isinstance(params, LeakageTransition2Params):
                targets = op.targets_copy()
                for k in range(0, len(targets), 2):
                    q0 = int(targets[k].qubit_value)
                    q1 = int(targets[k + 1].qubit_value)
                    partners[(j, q0)] = q1
                    partners[(j, q1)] = q0
        return partners

    def _trace_flows_fast(
        self, sources: list[_Source], num_flags: int, source_mode: bool = False
    ) -> tuple[
        NDArray[np.int32],
        NDArray[np.int32],
        NDArray[np.int32],
        NDArray[np.int32],
        NDArray[np.int32],
        NDArray[np.float64],
        NDArray[np.int32],
        NDArray[np.int32],
        list[tuple[str, tuple[float, ...]]],
    ]:
        num_ops = len(self.ops)
        max_q = -1
        for q in self.touch:
            if q > max_q:
                max_q = q
        for src in sources:
            if src.qubit > max_q:
                max_q = src.qubit
            if src.partner is not None and src.partner > max_q:
                max_q = src.partner
        num_qubits = max_q + 1

        all_leaked_states: set[int] = {2}
        for src in sources:
            all_leaked_states.add(src.state)
        for params in self.params:
            if isinstance(params, LeakageTransition1Params):
                for s, branches in params.args_by_input_state.items():
                    if _is_leaked(s):
                        all_leaked_states.add(s)
                    for out, _ in branches:
                        if _is_leaked(out):
                            all_leaked_states.add(out)
            elif isinstance(params, LeakageTransition2Params):
                for key, branches in params.args_by_input_state.items():
                    for s in key:
                        if _is_leaked(s):
                            all_leaked_states.add(s)
                    for outs, _ in branches:
                        for s in outs:
                            if _is_leaked(s):
                                all_leaked_states.add(s)
            elif isinstance(params, LeakageMeasurementParams):
                for s in params.prob_for_input_state:
                    if _is_leaked(s):
                        all_leaked_states.add(s)
            elif isinstance(params, LeakageConditioningParams):
                for group in params.args:
                    for s in group:
                        if _is_leaked(s):
                            all_leaked_states.add(s)
        all_states = sorted(all_leaked_states)

        channel_list: list[tuple[str, tuple[float, ...]]] = [("DEPOLARIZE1", (0.75,))]
        channel_map: dict[tuple[str, tuple[float, ...]], int] = {
            ("DEPOLARIZE1", (0.75,)): 0
        }

        def get_channel_id(name: str, args: tuple[float, ...]) -> int:
            key = (name, args)
            cid = channel_map.get(key)
            if cid is None:
                cid = len(channel_list)
                channel_list.append(key)
                channel_map[key] = cid
            return cid

        gate_id_map: dict[str, int] = {}
        conj_rows: list[list[int]] = [[0] * 7]
        for name, gd in self.gate_data.items():
            if gd.is_unitary and gd.is_single_qubit_gate:
                tab = stim.Tableau.from_named_gate(name)
                row = [0]
                for idx in range(1, 7):
                    row.append(_PAULI_TO_ID[str(tab(_PAULI_STRINGS[idx]))])
                gate_id_map[name] = len(conj_rows)
                conj_rows.append(row)
        conj_table = np.asarray(conj_rows, dtype=np.int32).reshape(-1)

        ops_arr = (OpDescC * num_ops)()
        target_qubits_list: list[int] = []
        meas_probs_list: list[MeasStateProbC] = []
        trans1_rules_list: list[Trans1RuleC] = []
        trans2_rules_list: list[Trans2RuleC] = []
        move_branches_list: list[MoveBranchC] = []
        cond_rules_list: list[CondFireRuleC] = []

        for j, (op, params) in enumerate(zip(self.ops, self.params)):
            gd = self.gate_data[op.name]
            desc = ops_arr[j]
            desc.kind = MOP_NONE
            desc.target_offset = len(target_qubits_list)
            desc.target_count = 0
            desc.aux_offset = 0
            desc.aux_count = 0
            desc.gate_id = gate_id_map.get(op.name, 0)
            desc.reset_basis = _BASIS_TO_ID.get(_RESET_BASIS.get(op.name, ""), 0)
            ctrl = _CONTROL_BASIS.get(op.name)
            desc.ctrl_leg0_basis = _BASIS_TO_ID[ctrl[0]] if ctrl is not None else 0
            desc.ctrl_leg1_basis = _BASIS_TO_ID[ctrl[1]] if ctrl is not None else 0
            desc.cond_base_fires = 0
            desc.cond_subkind = 0
            desc.cond_channel_id = -1

            if op.name in _ANNOTATIONS:
                continue

            if isinstance(params, LeakageMeasurementParams):
                if params.targets is not None:
                    desc.kind = MOP_MEAS_FLAG
                    for q in params.targets:
                        target_qubits_list.append(int(q))
                        target_qubits_list.append(int(self.flag_of[(j, q)]))
                    desc.target_count = len(target_qubits_list) - desc.target_offset
                    desc.aux_offset = len(meas_probs_list)
                    for s, p in params.prob_for_input_state.items():
                        if _is_leaked(s) and p > 0:
                            meas_probs_list.append(MeasStateProbC(int(s), float(p)))
                    desc.aux_count = len(meas_probs_list) - desc.aux_offset
                else:
                    desc.kind = MOP_MEAS_PROJ_Z
            elif isinstance(params, LeakageTransition1Params):
                desc.kind = MOP_TRANS_1
                for t in op.targets_copy():
                    target_qubits_list.append(
                        int(t.qubit_value) if t.qubit_value is not None else -1
                    )
                desc.target_count = len(target_qubits_list) - desc.target_offset
                desc.aux_offset = len(trans1_rules_list)
                for s in all_states:
                    branches = params.args_by_input_state.get(s, ())
                    if not branches:
                        continue
                    p_unleak = sum(p for out, p in branches if not _is_leaked(out))
                    next_s = -1
                    for out, p in branches:
                        if _is_leaked(out) and p >= _ONE:
                            next_s = int(out)
                    trans1_rules_list.append(
                        Trans1RuleC(int(s), float(p_unleak), int(next_s))
                    )
                desc.aux_count = len(trans1_rules_list) - desc.aux_offset
            elif isinstance(params, LeakageTransition2Params):
                desc.kind = MOP_TRANS_2
                for t in op.targets_copy():
                    target_qubits_list.append(
                        int(t.qubit_value) if t.qubit_value is not None else -1
                    )
                desc.target_count = len(target_qubits_list) - desc.target_offset
                desc.aux_offset = len(trans2_rules_list)
                for s in all_states:
                    for leg in (0, 1):
                        key = (s, "U") if leg == 0 else ("U", s)
                        branches = params.args_by_input_state.get(key, ())
                        if not branches:
                            continue
                        move: dict[int, float] = defaultdict(float)
                        stay: dict[int, float] = defaultdict(float)
                        p_clear = 0.0
                        channel = np.zeros(4)
                        for outs, p in branches:
                            out_h, out_partner = outs[leg], outs[1 - leg]
                            if _is_leaked(out_partner):
                                if not _is_leaked(out_h):
                                    move[out_partner] += p
                                continue
                            channel += p * _pauli_dist(out_partner)
                            if _is_leaked(out_h):
                                stay[out_h] += p
                            else:
                                p_clear += p
                        p_move = sum(move.values())
                        move_offset = len(move_branches_list)
                        for st_m, p_m in move.items():
                            move_branches_list.append(
                                MoveBranchC(int(st_m), float(p_m))
                            )
                        move_count = len(move_branches_list) - move_offset
                        max_move_state = (
                            int(max(move, key=move.__getitem__)) if move else -1
                        )
                        stay_next_state = -1
                        for st_stay, p_stay in stay.items():
                            if p_stay >= _ONE:
                                stay_next_state = int(st_stay)
                        ch_id = (
                            get_channel_id(
                                "PAULI_CHANNEL_1",
                                tuple(float(c) for c in channel[1:]),
                            )
                            if channel[1:].sum() > 0
                            else -1
                        )
                        trans2_rules_list.append(
                            Trans2RuleC(
                                int(s),
                                int(leg),
                                float(p_clear),
                                float(p_move),
                                move_offset,
                                move_count,
                                max_move_state,
                                stay_next_state,
                                ch_id,
                            )
                        )
                desc.aux_count = len(trans2_rules_list) - desc.aux_offset
            elif isinstance(params, LeakageConditioningParams):
                desc.kind = MOP_CONDITIONED
                groups = _condition_groups(op, params)
                for group, subjects in groups:
                    q0 = (
                        int(group[0].qubit_value)
                        if len(group) >= 1 and group[0].qubit_value is not None
                        else -1
                    )
                    q1 = (
                        int(group[1].qubit_value)
                        if len(group) >= 2 and group[1].qubit_value is not None
                        else -1
                    )
                    subj0 = (
                        int(subjects[0])
                        if len(subjects) >= 1 and subjects[0] is not None
                        else -1
                    )
                    subj1 = (
                        int(subjects[1])
                        if len(subjects) >= 2 and subjects[1] is not None
                        else -1
                    )
                    target_qubits_list.extend((q0, q1, subj0, subj1))
                desc.target_count = len(target_qubits_list) - desc.target_offset
                desc.cond_base_fires = (
                    1 if _fires(params, [0] * len(params.args)) else 0
                )
                if op.name in _HERALDED:
                    desc.cond_subkind = 7
                elif gd.produces_measurements:
                    desc.cond_subkind = 1
                elif gd.is_reset:
                    desc.cond_subkind = 2
                elif gd.is_noisy_gate:
                    desc.cond_subkind = 3
                    desc.cond_channel_id = get_channel_id(
                        op.name, tuple(float(a) for a in op.gate_args_copy())
                    )
                elif gd.is_unitary and gd.is_single_qubit_gate:
                    desc.cond_subkind = 4
                elif gd.is_unitary and gd.is_two_qubit_gate:
                    desc.cond_subkind = 5
                else:
                    desc.cond_subkind = 0

                desc.aux_offset = len(cond_rules_list)
                if len(params.args) == 1:
                    for s in all_states:
                        cond_rules_list.append(
                            CondFireRuleC(
                                int(s), 1, 1 if _fires(params, [s]) else 0
                            )
                        )
                else:
                    for s in all_states:
                        cond_rules_list.append(
                            CondFireRuleC(
                                int(s), 1, 1 if _fires(params, [s, 0]) else 0
                            )
                        )
                        cond_rules_list.append(
                            CondFireRuleC(
                                int(s), 2, 1 if _fires(params, [0, s]) else 0
                            )
                        )
                        cond_rules_list.append(
                            CondFireRuleC(
                                int(s), 3, 1 if _fires(params, [s, s]) else 0
                            )
                        )
                desc.aux_count = len(cond_rules_list) - desc.aux_offset
            else:
                targets = op.targets_copy()
                if (
                    not self.uc
                    or gd.produces_measurements
                    or not (gd.is_noisy_gate or gd.is_unitary)
                ):
                    self._encode_bare_op(desc, op.name, gd, targets, target_qubits_list)
                elif gd.is_single_qubit_gate or op.name in (
                    "E",
                    "ELSE_CORRELATED_ERROR",
                ):
                    if gd.is_unitary:
                        desc.kind = MOP_UNTAGGED_1Q_UNITARY
                        for t in targets:
                            if t.qubit_value is not None:
                                target_qubits_list.append(int(t.qubit_value))
                        desc.target_count = (
                            len(target_qubits_list) - desc.target_offset
                        )
                    else:
                        desc.kind = MOP_BARE_NOOP
                elif gd.is_two_qubit_gate:
                    if gd.is_unitary:
                        desc.kind = MOP_UNTAGGED_2Q_UNITARY
                        for t in targets:
                            target_qubits_list.append(
                                int(t.qubit_value)
                                if t.qubit_value is not None
                                else -1
                            )
                        desc.target_count = (
                            len(target_qubits_list) - desc.target_offset
                        )
                    else:
                        desc.kind = MOP_BARE_NOOP
                else:
                    self._encode_bare_op(desc, op.name, gd, targets, target_qubits_list)

        touch_offsets = np.zeros(num_qubits + 1, dtype=np.int32)
        touch_flat: list[int] = []
        for q in range(num_qubits):
            qs = self.touch.get(q, [])
            touch_flat.extend(qs)
            touch_offsets[q + 1] = len(touch_flat)
        touch_indices = np.asarray(touch_flat, dtype=np.int32)
        target_qubits = np.asarray(target_qubits_list, dtype=np.int32)

        meas_probs_arr = (MeasStateProbC * len(meas_probs_list))(*meas_probs_list)
        trans1_rules_arr = (Trans1RuleC * len(trans1_rules_list))(*trans1_rules_list)
        trans2_rules_arr = (Trans2RuleC * len(trans2_rules_list))(*trans2_rules_list)
        move_branches_arr = (MoveBranchC * len(move_branches_list))(*move_branches_list)
        cond_rules_arr = (CondFireRuleC * len(cond_rules_list))(*cond_rules_list)

        sources_arr = (SourceDescC * len(sources))()
        for idx, src in enumerate(sources):
            s_desc = sources_arr[idx]
            s_desc.op_index = int(src.op_index)
            s_desc.group = int(src.group)
            s_desc.qubit = int(src.qubit)
            s_desc.state = int(src.state)
            s_desc.weight = float(src.weight)
            s_desc.partner = int(src.partner) if src.partner is not None else -1
            s_desc.partner_channel_id = (
                get_channel_id("PAULI_CHANNEL_1", src.partner_channel)
                if src.partner_channel is not None
                else -1
            )

        (
            err_op,
            sites,
            flow_site_offsets,
            flow_site_ids,
            flag_cand_offsets,
            flag_cand_flows,
            flag_cand_weights,
            pred_offsets,
            pred_flags,
        ) = trace_flows_cpp(
            num_ops=num_ops,
            num_qubits=num_qubits,
            num_flags=num_flags,
            ops_arr=ops_arr,
            target_qubits=target_qubits,
            touch_offsets=touch_offsets,
            touch_indices=touch_indices,
            conj_table=conj_table,
            meas_probs_arr=meas_probs_arr,
            trans1_rules_arr=trans1_rules_arr,
            trans2_rules_arr=trans2_rules_arr,
            move_branches_arr=move_branches_arr,
            cond_rules_arr=cond_rules_arr,
            sources_arr=sources_arr,
            source_mode=source_mode,
        )
        if err_op >= 0:
            raise NotImplementedError(
                f"Leakage changes whether {self.ops[err_op]} applies a measurement or reset."
            )

        return (
            sites,
            flow_site_offsets,
            flow_site_ids,
            flag_cand_offsets,
            flag_cand_flows,
            flag_cand_weights,
            pred_offsets,
            pred_flags,
            channel_list,
        )

    @staticmethod
    def _encode_bare_op(
        desc: OpDescC,
        name: str,
        gd: Any,
        targets: list[stim.GateTarget],
        target_qubits_list: list[int],
    ) -> None:
        if name in _HERALDED or (gd.is_noisy_gate and not gd.produces_measurements):
            desc.kind = MOP_BARE_NOOP
        elif gd.is_unitary and gd.is_single_qubit_gate:
            desc.kind = MOP_BARE_1Q_UNITARY
            for t in targets:
                if t.qubit_value is not None:
                    target_qubits_list.append(int(t.qubit_value))
            desc.target_count = len(target_qubits_list) - desc.target_offset
        elif gd.is_reset and not gd.produces_measurements:
            desc.kind = MOP_BARE_RESET
        else:
            desc.kind = MOP_BARE_OTHER

    def _propagate_unique_sites(
        self,
        sites: NDArray[np.int32],
        channel_list: list[tuple[str, tuple[float, ...]]],
        *,
        decompose_errors: bool = False,
        baseline: stim.DetectorErrorModel | None = None,
    ) -> tuple[
        list[Any],
        NDArray[np.int32],
        NDArray[np.int32],
        NDArray[np.float64],
    ]:
        before: dict[int, list[stim.CircuitInstruction]] = defaultdict(list)
        after: dict[int, list[stim.CircuitInstruction]] = defaultdict(list)
        for site_id in range(sites.shape[0]):
            j = int(sites[site_id, 0])
            is_before = bool(sites[site_id, 1])
            q0 = int(sites[site_id, 2])
            q1 = int(sites[site_id, 3])
            ch_id = int(sites[site_id, 4])
            name, args = channel_list[ch_id]
            qubits = [q0] if q1 < 0 else [q0, q1]
            tag = f"{_SOURCE_TAG}:{site_id}"
            (before if is_before else after)[j].append(
                stim.CircuitInstruction(name, qubits, args, tag=tag)
            )

        def interleave(ops: list[Any]) -> stim.Circuit:
            out = stim.Circuit()
            for j, op in enumerate(ops):
                for inst in before.get(j, ()):
                    out.append(inst)
                if op is not None:
                    out.append(op)
                for inst in after.get(j, ()):
                    out.append(inst)
            return out

        envelope = interleave(self.ref_ops)

        if decompose_errors:
            try:
                dem = envelope.detector_error_model(
                    decompose_errors=True,
                    approximate_disjoint_errors=True,
                    block_decomposition_from_introducing_remnant_edges=True,
                )
            except ValueError:
                # Include untagged baseline noise so Stim can use baseline
                # graphlike mechanisms when decomposing envelope hyperedges.
                env_with_base = interleave(self.base_ops)
                try:
                    dem = env_with_base.detector_error_model(
                        decompose_errors=True,
                        approximate_disjoint_errors=True,
                        block_decomposition_from_introducing_remnant_edges=True,
                    )
                except ValueError:
                    try:
                        dem = env_with_base.detector_error_model(
                            decompose_errors=True, approximate_disjoint_errors=True
                        )
                    except ValueError:
                        dem = env_with_base.detector_error_model(
                            decompose_errors=True,
                            approximate_disjoint_errors=True,
                            ignore_decomposition_failures=True,
                        )
        else:
            dem = envelope.detector_error_model(
                decompose_errors=False, approximate_disjoint_errors=True
            )

        known_graphlike: dict[tuple[int, ...], Symptom] | None = None
        sym_to_id: dict[Any, int] = {}
        symptoms: list[Any] = []
        entry_sites: list[int] = []
        entry_syms: list[int] = []
        entry_probs_list: list[float] = []

        prefix = _SOURCE_TAG + ":"
        for inst in dem.flattened():
            if inst.type != "error":
                continue
            if not inst.tag.startswith(prefix):
                if decompose_errors:
                    continue
                raise AssertionError(f"Noise left in the reference circuit: {inst}")
            site_id = int(inst.tag[len(prefix) :])
            p = float(inst.args_copy()[0])
            if decompose_errors:
                raw_comps = _raw_inst_components(inst)
                if any(len(d) > 2 for d, _ in raw_comps):
                    if known_graphlike is None:
                        known_graphlike = _collect_known_graphlike(baseline, dem)
                    comps = _graphlike_components(inst, known_graphlike)
                else:
                    comps = [(d, o) for d, o in raw_comps if d]
                sym = _canonical_symptom_from_components(comps)
            else:
                sym = _symptom(inst)
            if sym == ((), ()):
                continue
            sym_id = sym_to_id.get(sym)
            if sym_id is None:
                sym_id = len(symptoms)
                symptoms.append(sym)
                sym_to_id[sym] = sym_id
            entry_sites.append(site_id)
            entry_syms.append(sym_id)
            entry_probs_list.append(p)

        return (
            symptoms,
            np.asarray(entry_sites, dtype=np.int32),
            np.asarray(entry_syms, dtype=np.int32),
            np.asarray(entry_probs_list, dtype=np.float64),
        )

    @staticmethod
    def _build_reweight_tables(
        baseline: stim.DetectorErrorModel,
        symptoms: list[Any],
        *,
        decompose_errors: bool,
    ) -> tuple[
        NDArray[np.float64],
        NDArray[np.int32],
        NDArray[np.int32],
        NDArray[np.int32],
        NDArray[np.float64],
        stim.DetectorErrorModel,
    ]:
        num_symptoms = len(symptoms)
        sym_to_id = {sym: idx for idx, sym in enumerate(symptoms)}
        base_sym_probs = np.zeros(num_symptoms, dtype=np.float64)

        edge_to_id: dict[tuple[int, int], int] = {}
        edge_nodes_list: list[tuple[int, int]] = []
        sym_edge_offsets_list: list[int] = [0]
        sym_edge_ids_list: list[int] = []
        first_sym_for_edge: dict[int, Symptom] = {}

        for sym in symptoms:
            for dets, obs in _symptom_components(sym):
                if 1 <= len(dets) <= 2:
                    edge_key = (dets[0], -1) if len(dets) == 1 else (dets[0], dets[1])
                    eid = edge_to_id.get(edge_key)
                    if eid is None:
                        eid = len(edge_nodes_list)
                        edge_nodes_list.append(edge_key)
                        edge_to_id[edge_key] = eid
                        first_sym_for_edge[eid] = (dets, obs)
                    elif not first_sym_for_edge[eid][1] and obs:
                        first_sym_for_edge[eid] = (dets, obs)
                    sym_edge_ids_list.append(eid)
            sym_edge_offsets_list.append(len(sym_edge_ids_list))

        sym_edge_offsets = np.asarray(sym_edge_offsets_list, dtype=np.int32)
        sym_edge_ids = np.asarray(sym_edge_ids_list, dtype=np.int32)
        num_edges = len(edge_nodes_list)
        edge_nodes = (
            np.asarray(edge_nodes_list, dtype=np.int32).reshape(num_edges, 2)
            if num_edges > 0
            else np.empty((0, 2), dtype=np.int32)
        )
        base_edge_probs = np.zeros(num_edges, dtype=np.float64)
        edge_in_baseline = np.zeros(num_edges, dtype=bool)

        known_base_graphlike: dict[tuple[int, ...], Symptom] | None = None
        for inst in baseline.flattened():
            if inst.type != "error":
                continue
            p = float(inst.args_copy()[0])
            if decompose_errors:
                raw_comps = _raw_inst_components(inst)
                if any(len(d) > 2 for d, _ in raw_comps):
                    if known_base_graphlike is None:
                        known_base_graphlike = _collect_known_graphlike(baseline)
                    comps = _graphlike_components(inst, known_base_graphlike)
                else:
                    comps = [(d, o) for d, o in raw_comps if d]
                sym = _canonical_symptom_from_components(comps)
            else:
                sym = _symptom(inst)
                comps = [sym]
            sid = sym_to_id.get(sym)
            if sid is not None:
                base_sym_probs[sid] = _xor(float(base_sym_probs[sid]), p)
            for dets, _ in comps:
                if 1 <= len(dets) <= 2:
                    edge_key = (dets[0], -1) if len(dets) == 1 else (dets[0], dets[1])
                    eid = edge_to_id.get(edge_key)
                    if eid is not None:
                        base_edge_probs[eid] = _xor(float(base_edge_probs[eid]), p)
                        edge_in_baseline[eid] = True

        matching_base_dem = baseline.copy()
        if decompose_errors and num_edges > 0 and not edge_in_baseline.all():
            for eid in range(num_edges):
                if not edge_in_baseline[eid]:
                    dets, obs = first_sym_for_edge[eid]
                    matching_base_dem.append(
                        "error",
                        1e-12,
                        [stim.target_relative_detector_id(d) for d in dets]
                        + [stim.target_logical_observable_id(o) for o in obs],
                    )

        return (
            base_sym_probs,
            sym_edge_offsets,
            sym_edge_ids,
            edge_nodes,
            base_edge_probs,
            matching_base_dem,
        )

    def _base_and_ref(self, op, params):
        """(op in the unleaked noisy baseline, op in the noiseless reference)."""
        gd = self.gate_data[op.name]
        if isinstance(params, LeakageTransition1Params):
            p01 = sum(
                p
                for out, p in params.args_by_input_state.get("U", ())
                if not _is_leaked(out)
            )
            if p01 <= 0:
                return None, None
            return (
                stim.CircuitInstruction(
                    "PAULI_CHANNEL_1", op.targets_copy(), [p01 / 4] * 3
                ),
                None,
            )
        if isinstance(params, LeakageTransition2Params):
            table = np.zeros((4, 4))
            for (o0, o1), p in params.args_by_input_state.get(("U", "U"), ()):
                if not _is_leaked(o0) and not _is_leaked(o1):
                    table += p * np.outer(_pauli_dist(o0), _pauli_dist(o1))
            args = [float(a) for a in table.flatten()[1:]]  # stim order: 4 * P0 + P1
            if sum(args) <= 0:
                return None, None
            return (
                stim.CircuitInstruction("PAULI_CHANNEL_2", op.targets_copy(), args),
                None,
            )
        if isinstance(params, LeakageMeasurementParams):
            prob = params.prob_for_input_state
            if params.targets is not None:  # MPAD leakage flag on unleaked qubits
                p = (prob.get(0, 0.0) + prob.get(1, 0.0)) / 2
            else:  # LEAKAGE_PROJECTION_Z; the op's own args are ignored, like the handler
                p = (prob.get(0, 0.0) + 1.0 - prob.get(1, 1.0)) / 2
            return _stripped(op, [p] if p > 0 else []), _stripped(op, [])
        if isinstance(params, LeakageConditioningParams) and not _fires(params, [0, 0]):
            if gd.produces_measurements:
                raise NotImplementedError(
                    f"Conditioned measurement {op} that does not fire on unleaked qubits."
                )
            return None, None
        # Untagged (or foreign-tagged) ops, and conditioned ops that fire when unleaked.
        if gd.produces_measurements:
            if op.name in _HERALDED:
                return _stripped(op), _stripped(op, [0.0] * len(op.gate_args_copy()))
            return _stripped(op), _stripped(op, [])
        if gd.is_noisy_gate:
            return _stripped(op), None
        return _stripped(op), _stripped(op)

    def _find_flags(self) -> tuple[list[int], list[bool]]:
        self.flag_of: dict[tuple[int, int], int] = {}
        flag_records: list[int] = []
        flag_invert: list[bool] = []
        offset = 0
        for i, (op, params) in enumerate(zip(self.ops, self.params)):
            if (
                isinstance(params, LeakageMeasurementParams)
                and params.targets is not None
            ):
                targets = op.targets_copy()
                if len(targets) != len(params.targets):
                    raise ValueError(
                        f"{op} has {len(targets)} targets but its tag names "
                        f"{len(params.targets)} qubits."
                    )
                for k, (t, q) in enumerate(zip(targets, params.targets)):
                    self.flag_of[(i, q)] = len(flag_records)
                    flag_records.append(offset + k)
                    flag_invert.append(bool(t.value) ^ t.is_inverted_result_target)
            offset += op.num_measurements
        self.num_measurements = offset
        return flag_records, flag_invert

    def _build_touch_lists(self) -> None:
        self.touch: dict[int, list[int]] = defaultdict(list)
        for i, (op, params) in enumerate(zip(self.ops, self.params)):
            if op.name in _ANNOTATIONS:
                continue
            if op.name == "MPAD":  # MPAD targets are bits, not qubits
                if isinstance(params, LeakageMeasurementParams) and params.targets:
                    qubits = set(params.targets)
                else:
                    qubits = set()
            else:
                qubits = {
                    t.qubit_value
                    for t in op.targets_copy()
                    if t.qubit_value is not None
                }
                if isinstance(params, LeakageConditioningParams) and params.targets:
                    qubits.update(params.targets)
            for q in qubits:
                self.touch[q].append(i)

    def _find_sources(self) -> list[_Source]:
        sources = []
        for i, (op, params) in enumerate(zip(self.ops, self.params)):
            if isinstance(params, LeakageTransition1Params):
                weights: dict[int, float] = defaultdict(float)
                for out, p in params.args_by_input_state.get("U", ()):
                    if _is_leaked(out):
                        weights[out] += p
                for k, t in enumerate(op.targets_copy()):
                    for s, w in weights.items():
                        if w > 0:
                            sources.append(_Source(i, k, t.qubit_value, s, w))
            elif isinstance(params, LeakageTransition2Params):
                branches = params.args_by_input_state.get(("U", "U"), ())
                targets = op.targets_copy()
                for k in range(0, len(targets), 2):
                    pair = (targets[k].qubit_value, targets[k + 1].qubit_value)
                    for leg in (0, 1):
                        weights = defaultdict(float)
                        channels: dict[int, NDArray[np.float64]] = defaultdict(
                            lambda: np.zeros(4)
                        )
                        for outs, p in branches:
                            s, out_partner = outs[leg], outs[1 - leg]
                            if not _is_leaked(s):
                                continue
                            weights[s] += p
                            if not _is_leaked(out_partner):
                                channels[s] += p * _pauli_dist(out_partner)
                        for s, w in weights.items():
                            if w <= 0:
                                continue
                            channel = channels[s][1:] / w
                            sources.append(
                                _Source(
                                    i,
                                    k // 2,
                                    pair[leg],
                                    s,
                                    w,
                                    partner=pair[1 - leg],
                                    partner_channel=(
                                        tuple(float(c) for c in channel)
                                        if channel.sum() > 0
                                        else None
                                    ),
                                )
                            )
        return sources
