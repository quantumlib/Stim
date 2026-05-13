from __future__ import annotations

import collections
from typing import cast, TYPE_CHECKING

import stim

from stimflow._chunk._stabilizer_code import StabilizerCode
from stimflow._core import PauliMap

if TYPE_CHECKING:
    from stimflow._chunk._chunk_builder import ChunkBuilder
    from stimflow._chunk._chunk import Chunk
    from stimflow._chunk._chunk_reflow import ChunkReflow


def find_d1_error(
    obj: stim.Circuit | stim.DetectorErrorModel,
) -> stim.ExplainedError | stim.DemInstruction | None:
    circuit: stim.Circuit | None
    dem: stim.DetectorErrorModel
    if isinstance(obj, stim.Circuit):
        circuit = obj
        dem = circuit.detector_error_model()
    elif isinstance(obj, stim.DetectorErrorModel):
        circuit = None
        dem = obj
    else:
        raise NotImplementedError(f"{obj=}")

    for inst in dem:
        if inst.type == "error":
            dets: set[int] = set()
            obs: set[int] = set()
            for target in inst.targets_copy():
                if target.is_relative_detector_id():
                    dets ^= {target.val}
                elif target.is_logical_observable_id():
                    obs ^= {target.val}
            if obs and not dets:
                if circuit is None:
                    return inst
                filter_det = stim.DetectorErrorModel()
                filter_det.append(inst)
                return circuit.explain_detector_error_model_errors(
                    dem_filter=filter_det, reduce_to_one_representative_error=True
                )[0]

    return None


def find_d2_error(
    obj: stim.Circuit | stim.DetectorErrorModel,
) -> list[stim.ExplainedError] | stim.DetectorErrorModel | None:
    d1 = find_d1_error(obj)
    if d1 is not None:
        if isinstance(d1, stim.DemInstruction):
            result = stim.DetectorErrorModel()
            result.append(d1)
            return result
        return [d1]

    if isinstance(obj, stim.Circuit):
        circuit = obj
        dem = circuit.detector_error_model()
    elif isinstance(obj, stim.DetectorErrorModel):
        circuit = None
        dem = obj
    else:
        raise NotImplementedError(f"{obj=}")

    seen = {}
    for inst in dem.flattened():
        if inst.type == "error":
            dets_mut: set[int] = set()
            obs_mut: set[int] = set()
            for target in inst.targets_copy():
                if target.is_relative_detector_id():
                    dets_mut ^= {target.val}
                elif target.is_logical_observable_id():
                    obs_mut ^= {target.val}
            dets = frozenset(dets_mut)
            obs = frozenset(obs_mut)
            if dets not in seen:
                seen[dets] = (obs, inst)
            elif seen[dets][0] != obs:
                filter_det = stim.DetectorErrorModel()
                filter_det.append(inst)
                filter_det.append(seen[dets][1])
                if circuit is None:
                    return filter_det
                return circuit.explain_detector_error_model_errors(
                    dem_filter=filter_det, reduce_to_one_representative_error=True
                )
    return None


def verify_distance_is_at_least(obj: stim.Circuit | stim.DetectorErrorModel | StabilizerCode, minimum_distance: int):
    if minimum_distance == 2:
        _verify_distance_is_at_least_2(obj)
    elif minimum_distance == 3:
        _verify_distance_is_at_least_3(obj)
    elif minimum_distance < 2:
        return
    else:
        raise NotImplementedError("Only minimum_distance=2 and minimum_distance=3 are implemented efficiently.")

def _verify_distance_is_at_least_2(obj: stim.Circuit | stim.DetectorErrorModel | StabilizerCode):
    __tracebackhide__ = True
    if isinstance(obj, StabilizerCode):
        obj.verify_distance_is_at_least_2()
        return
    err = find_d1_error(obj)
    if err is not None:
        raise ValueError(f"Found a distance 1 error: {err}")


def _verify_distance_is_at_least_3(obj: stim.Circuit | stim.DetectorErrorModel | StabilizerCode):
    __tracebackhide__ = True
    err = find_d2_error(obj)
    if err is not None:
        raise ValueError(f"Found a distance {len(err)} error: {err}")


def transversal_code_transition_chunks(
    *, prev_code: StabilizerCode, next_code: StabilizerCode, measured: PauliMap, reset: PauliMap
) -> tuple[Chunk, ChunkReflow, Chunk]:
    from stimflow._chunk._chunk_reflow import ChunkReflow

    def clipped(original: PauliMap, dissipated: PauliMap) -> PauliMap | None:
        for q, p in original.items():
            if dissipated.get(q, p) != p:
                # Anticommutes.
                return None
        return PauliMap(
            {q: p for q, p in original.items() if q not in dissipated},
            obs_name=original.obs_name,
        )

    from stimflow._chunk._chunk_builder import ChunkBuilder
    prev_builder = ChunkBuilder(prev_code.data_set)
    prev_key2obs = {}
    for b in "XYZ":
        prev_builder.append(
            f"M{b}", prev_code.data_set & {q for q, p in measured.items() if p == b}
        )
    start: PauliMap | None
    end: PauliMap | None
    for tile in prev_code.tiles:
        start = tile.to_pauli_map()
        end = clipped(start, measured)
        if end is None:
            prev_builder.add_discarded_flow_input(tile)
        else:
            prev_builder.add_flow(start=tile, end=end, measurements=start.keys() - end.keys())
    for k, obs in enumerate(prev_code.flat_logicals):
        assert obs.obs_name is not None
        end = clipped(obs, measured)
        if end is None:
            prev_builder.add_discarded_flow_input(obs)
        else:
            prev_key2obs[obs.obs_name] = end
            prev_builder.add_flow(start=obs, end=end, measurements=obs.keys() - end.keys())

    next_builder = ChunkBuilder(next_code.data_set)
    next_obs2key = {}
    for b in "XYZ":
        next_builder.append(f"R{b}", next_code.data_set & {q for q, p in reset.items() if p == b})
    for tile in next_code.tiles:
        end = tile.to_pauli_map()
        start = clipped(end, reset)
        if start is None:
            next_builder.add_discarded_flow_output(tile)
        else:
            next_builder.add_flow(start=start, end=tile)
    for obs in next_code.flat_logicals:
        assert obs.obs_name is not None
        start = clipped(obs, reset)
        if start is None:
            next_builder.add_discarded_flow_output(obs)
        else:
            next_obs2key[obs.obs_name] = start
            next_builder.add_flow(start=start, end=obs)

    prev_chunk = prev_builder.finish_chunk(wants_to_merge_with_prev=True)
    reflow = ChunkReflow.from_auto_rewrite_transitions_using_stable(
        stable=[
            cast(PauliMap, flow.start)
            for flow in next_builder._flows
            if flow.obs_name is None
            if flow.start
        ],
        transitions=[
            (prev_key2obs[obs_name], next_obs2key[obs_name])
            for obs_name in next_obs2key.keys() & prev_key2obs.keys()
        ],
    )
    next_chunk = next_builder.finish_chunk(wants_to_merge_with_next=True)
    return prev_chunk, reflow, next_chunk
