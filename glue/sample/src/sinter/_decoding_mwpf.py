import math
import pathlib
from typing import Callable, List, TYPE_CHECKING, Tuple

import numpy as np
import stim

from sinter._decoding_decoder_class import Decoder, CompiledDecoder

if TYPE_CHECKING:
    import mwpf

DEFAULT_TIMEOUT: float = 10.0  # decoder timeout in seconds


class MwpfCompiledDecoder(CompiledDecoder):
    def __init__(
        self,
        solver: "mwpf.SolverSerialJointSingleHair",
        fault_masks: "np.ndarray",
        num_dets: int,
        num_obs: int,
    ):
        self.solver = solver
        self.fault_masks = fault_masks
        self.num_dets = num_dets
        self.num_obs = num_obs

    def decode_shots_bit_packed(
        self,
        *,
        bit_packed_detection_event_data: "np.ndarray",
    ) -> "np.ndarray":
        num_shots = bit_packed_detection_event_data.shape[0]
        predictions = np.zeros(shape=(num_shots, self.num_obs), dtype=np.uint8)
        import mwpf

        for shot in range(num_shots):
            dets_sparse = np.flatnonzero(
                np.unpackbits(
                    bit_packed_detection_event_data[shot],
                    count=self.num_dets,
                    bitorder="little",
                )
            )
            syndrome = mwpf.SyndromePattern(defect_vertices=dets_sparse)
            self.solver.solve(syndrome)
            prediction = int(
                np.bitwise_xor.reduce(self.fault_masks[self.solver.subgraph()])
            )
            predictions[shot] = np.packbits(prediction, bitorder="little")
            self.solver.clear()
        return predictions


class MwpfDecoder(Decoder):
    """Use MWPF to predict observables from detection events."""

    def compile_decoder_for_dem(
        self, *, dem: "stim.DetectorErrorModel", timeout: float = DEFAULT_TIMEOUT
    ) -> CompiledDecoder:
        try:
            import mwpf
        except ImportError as ex:
            raise ImportError(
                "The decoder 'MWPF' isn't installed\n"
                "To fix this, install the python package 'MWPF' into your environment.\n"
                "For example, if you are using pip, run `pip install MWPF`.\n"
            ) from ex

        solver, fault_masks = detector_error_model_to_mwpf_solver_and_fault_masks(
            dem, timeout=timeout
        )
        return MwpfCompiledDecoder(
            solver, fault_masks, dem.num_detectors, dem.num_observables
        )

    def decode_via_files(
        self,
        *,
        num_shots: int,
        num_dets: int,
        num_obs: int,
        dem_path: pathlib.Path,
        dets_b8_in_path: pathlib.Path,
        obs_predictions_b8_out_path: pathlib.Path,
        tmp_dir: pathlib.Path,
        timeout: float = DEFAULT_TIMEOUT,
    ) -> None:
        try:
            import mwpf
        except ImportError as ex:
            raise ImportError(
                "The decoder 'MWPF' isn't installed\n"
                "To fix this, install the python package 'MWPF' into your environment.\n"
                "For example, if you are using pip, run `pip install MWPF~=0.1.1`.\n"
            ) from ex

        error_model = stim.DetectorErrorModel.from_file(dem_path)
        solver, fault_masks = detector_error_model_to_mwpf_solver_and_fault_masks(
            error_model, timeout=timeout
        )
        num_det_bytes = math.ceil(num_dets / 8)
        with open(dets_b8_in_path, "rb") as dets_in_f:
            with open(obs_predictions_b8_out_path, "wb") as obs_out_f:
                for _ in range(num_shots):
                    dets_bit_packed = np.fromfile(
                        dets_in_f, dtype=np.uint8, count=num_det_bytes
                    )
                    if dets_bit_packed.shape != (num_det_bytes,):
                        raise IOError("Missing dets data.")
                    dets_sparse = np.flatnonzero(
                        np.unpackbits(
                            dets_bit_packed, count=num_dets, bitorder="little"
                        )
                    )
                    syndrome = mwpf.SyndromePattern(defect_vertices=dets_sparse)
                    solver.solve(syndrome)
                    prediction = int(
                        np.bitwise_xor.reduce(fault_masks[solver.subgraph()])
                    )
                    obs_out_f.write(
                        prediction.to_bytes((num_obs + 7) // 8, byteorder="little")
                    )
                    solver.clear()


class HyperUFDecoder(MwpfDecoder):
    """Setting timeout to 0 becomes effectively a hypergraph UF decoder"""

    def compile_decoder_for_dem(
        self, *, dem: "stim.DetectorErrorModel"
    ) -> CompiledDecoder:
        return super().compile_decoder_for_dem(dem=dem, timeout=0.0)

    def decode_via_files(
        self,
        *,
        num_shots: int,
        num_dets: int,
        num_obs: int,
        dem_path: pathlib.Path,
        dets_b8_in_path: pathlib.Path,
        obs_predictions_b8_out_path: pathlib.Path,
        tmp_dir: pathlib.Path,
    ) -> None:
        return super().decode_via_files(
            num_shots=num_shots,
            num_dets=num_dets,
            num_obs=num_obs,
            dem_path=dem_path,
            dets_b8_in_path=dets_b8_in_path,
            obs_predictions_b8_out_path=obs_predictions_b8_out_path,
            tmp_dir=tmp_dir,
            timeout=0.0,
        )


def iter_flatten_model(
    model: stim.DetectorErrorModel,
    handle_error: Callable[[float, List[int], List[int]], None],
    handle_detector_coords: Callable[[int, np.ndarray], None],
):
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
                        handle_error(p, dets, frames)
                    elif instruction.type == "shift_detectors":
                        det_offset += instruction.targets_copy()[0]
                        a = np.array(instruction.args_copy())
                        coords_offset[: len(a)] += a
                    elif instruction.type == "detector":
                        a = np.array(instruction.args_copy())
                        for t in instruction.targets_copy():
                            handle_detector_coords(
                                t.val + det_offset, a + coords_offset[: len(a)]
                            )
                    elif instruction.type == "logical_observable":
                        pass
                    else:
                        raise NotImplementedError()
                else:
                    raise NotImplementedError()

    _helper(model, 1)


def detector_error_model_to_mwpf_solver_and_fault_masks(
    model: stim.DetectorErrorModel, timeout: float = DEFAULT_TIMEOUT
) -> Tuple["mwpf.SolverSerialJointSingleHair", np.ndarray]:
    """Convert a stim error model into a NetworkX graph."""

    import mwpf

    num_detectors = model.num_detectors
    is_detector_connected = np.full(num_detectors, False, dtype=bool)
    hyperedges: List[Tuple[List[int], float, int]] = []

    def handle_error(p: float, dets: List[int], frame_changes: List[int]):
        if p == 0:
            return
        if len(dets) == 0:
            # No symptoms for this error.
            # Code probably has distance 1.
            # Accept it and keep going, though of course decoding will probably perform terribly.
            return
        if p > 0.5:
            # mwpf doesn't support negative edge weights.
            # approximate them as weight 0.
            p = 0.5
        weight = math.log((1 - p) / p)
        mask = sum(1 << k for k in frame_changes)
        is_detector_connected[dets] = True
        hyperedges.append((dets, weight, mask))

    def handle_detector_coords(detector: int, coords: np.ndarray):
        pass

    iter_flatten_model(
        model,
        handle_error=handle_error,
        handle_detector_coords=handle_detector_coords,
    )

    # fix the input by connecting an edge to all isolated vertices
    for idx in range(num_detectors):
        if not is_detector_connected[idx]:
            hyperedges.append(([idx], 0, 0))

    max_weight = max(1e-4, max((w for _, w, _ in hyperedges), default=1))
    rescaled_edges = [
        mwpf.HyperEdge(v, round(w * 2**10 / max_weight) * 2) for v, w, _ in hyperedges
    ]
    fault_masks = np.array([e[2] for e in hyperedges], dtype=np.uint64)

    initializer = mwpf.SolverInitializer(
        num_detectors,  # Total number of nodes.
        rescaled_edges,  # Weighted edges.
    )

    return (
        mwpf.SolverSerialJointSingleHair(initializer, {"primal": {"timeout": timeout}}),
        fault_masks,
    )
