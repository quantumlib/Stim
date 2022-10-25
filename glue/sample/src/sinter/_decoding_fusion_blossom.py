import math
import pathlib
from typing import Callable, List, TYPE_CHECKING
from typing import Tuple

import numpy as np
import stim

if TYPE_CHECKING:
    import fusion_blossom


def decode_using_fusion_blossom(*,
                                num_shots: int,
                                num_dets: int,
                                num_obs: int,
                                dem_path: pathlib.Path,
                                dets_b8_in_path: pathlib.Path,
                                obs_predictions_b8_out_path: pathlib.Path,
                                tmp_dir: pathlib.Path,
                                ) -> None:
    """Use fusion_blossom to predict observables from detection events."""

    try:
        import fusion_blossom
    except ImportError as ex:
        raise ImportError(
            "The decoder 'fusion_blossom' isn't installed\n"
            "To fix this, install the python package 'fusion-blossom' into your environment.\n"
            "For example, if you are using pip, run `pip install fusion-blossom~=0.1.4`.\n"
        ) from ex

    error_model = stim.DetectorErrorModel.from_file(dem_path)
    solver, fault_masks = detector_error_model_to_fusion_blossom_solver_and_fault_masks(error_model)
    num_det_bytes = math.ceil(num_dets / 8)

    with open(dets_b8_in_path, 'rb') as dets_in_f:
        with open(obs_predictions_b8_out_path, 'wb') as obs_out_f:
            for _ in range(num_shots):
                dets_bit_packed = np.fromfile(dets_in_f, dtype=np.uint8, count=num_det_bytes)
                dets_sparse = np.flatnonzero(np.unpackbits(dets_bit_packed, count=num_dets, bitorder='little'))
                syndrome = fusion_blossom.SyndromePattern(syndrome_vertices=dets_sparse)
                solver.solve(syndrome)
                prediction = int(np.bitwise_xor.reduce(fault_masks[solver.subgraph()]))
                obs_out_f.write(prediction.to_bytes((num_obs + 7) // 8, byteorder='little'))
                solver.clear()


def iter_flatten_model(model: stim.DetectorErrorModel,
                       handle_error: Callable[[float, List[int], List[int]], None],
                       handle_detector_coords: Callable[[int, np.ndarray], None]):
    det_offset = 0
    coords_offset = np.zeros(100, dtype=np.float64)

    def _helper(m: stim.DetectorErrorModel, reps: int):
        nonlocal det_offset
        nonlocal coords_offset
        for _ in range(reps):
            for instruction in m:
                if isinstance(instruction, stim.DemRepeatBlock):
                    _helper(instruction.body_copy(), instruction.repeat_count)
                elif isinstance(instruction, stim.DemInstruction):
                    if instruction.type == "error":
                        dets: List[int] = []
                        frames: List[int] = []
                        t: stim.DemTarget
                        p = instruction.args_copy()[0]
                        for t in instruction.targets_copy():
                            if t.is_relative_detector_id():
                                dets.append(t.val + det_offset)
                            elif t.is_logical_observable_id():
                                frames.append(t.val)
                            elif t.is_separator():
                                # Treat each component of a decomposed error as an independent error.
                                # (Ideally we could configure some sort of correlated analysis; oh well.)
                                handle_error(p, dets, frames)
                                frames = []
                                dets = []
                        # Handle last component.
                        handle_error(p, dets, frames)
                    elif instruction.type == "shift_detectors":
                        det_offset += instruction.targets_copy()[0]
                        a = np.array(instruction.args_copy())
                        coords_offset[:len(a)] += a
                    elif instruction.type == "detector":
                        a = np.array(instruction.args_copy())
                        for t in instruction.targets_copy():
                            handle_detector_coords(t.val + det_offset, a + coords_offset[:len(a)])
                    elif instruction.type == "logical_observable":
                        pass
                    else:
                        raise NotImplementedError()
                else:
                    raise NotImplementedError()
    _helper(model, 1)


def detector_error_model_to_fusion_blossom_solver_and_fault_masks(model: stim.DetectorErrorModel) -> Tuple['fusion_blossom.SolverSerial', np.ndarray]:
    """Convert a stim error model into a NetworkX graph."""

    import fusion_blossom

    def handle_error(p: float, dets: List[int], frame_changes: List[int]):
        if p == 0:
            return
        if len(dets) == 0:
            # No symptoms for this error.
            # Code probably has distance 1.
            # Accept it and keep going, though of course decoding will probably perform terribly.
            return
        if len(dets) == 1:
            dets = [dets[0], num_detectors]
        if len(dets) > 2:
            raise NotImplementedError(
                f"Error with more than 2 symptoms can't become an edge or boundary edge: {dets!r}.")
        if p > 0.5:
            # fusion_blossom doesn't support negative edge weights.
            # approximate them as weight 0.
            p = 0.5
        weight = math.log((1 - p) / p)
        mask = sum(1 << k for k in frame_changes)
        edges.append((dets[0], dets[1], weight, mask))

    def handle_detector_coords(detector: int, coords: np.ndarray):
        pass

    num_detectors = model.num_detectors
    edges: List[Tuple[int, int, float, int]] = []
    iter_flatten_model(
        model,
        handle_error=handle_error,
        handle_detector_coords=handle_detector_coords,
    )
    max_weight = max(1e-4, max((w for _, _, w, _ in edges), default=1))
    rescaled_edges = [
        (a, b, round(w * 2**10 / max_weight) * 2)
        for a, b, w, _ in edges
    ]
    fault_masks = np.array([e[3] for e in edges], dtype=np.uint64)

    initializer = fusion_blossom.SolverInitializer(
        num_detectors + 1,  # Total number of nodes.
        rescaled_edges,  # Weighted edges.
        [num_detectors],  # Boundary node.
    )

    return fusion_blossom.SolverSerial(initializer), fault_masks
