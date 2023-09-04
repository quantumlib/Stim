import os

from typing import Any, Optional, Tuple, TYPE_CHECKING, Dict
import tempfile

if TYPE_CHECKING:
    import multiprocessing
    import numpy as np
    import pathlib
    import sinter
    import stim


class WorkIn:
    def __init__(
            self,
            *,
            work_key: Any,
            circuit_path: str,
            dem_path: str,
            decoder: str,
            strong_id: Optional[str],
            postselection_mask: 'Optional[np.ndarray]',
            postselected_observables_mask: 'Optional[np.ndarray]',
            json_metadata: Any,
            count_observable_error_combos: bool,
            count_detection_events: bool,
            num_shots: int):
        self.work_key = work_key
        self.circuit_path = circuit_path
        self.dem_path = dem_path
        self.decoder = decoder
        self.strong_id = strong_id
        self.postselection_mask = postselection_mask
        self.postselected_observables_mask = postselected_observables_mask
        self.json_metadata = json_metadata
        self.count_observable_error_combos = count_observable_error_combos
        self.count_detection_events = count_detection_events
        self.num_shots = num_shots

    def with_work_key(self, work_key: Any) -> 'WorkIn':
        return WorkIn(
            work_key=work_key,
            circuit_path=self.circuit_path,
            dem_path=self.dem_path,
            decoder=self.decoder,
            postselection_mask=self.postselection_mask,
            postselected_observables_mask=self.postselected_observables_mask,
            json_metadata=self.json_metadata,
            strong_id=self.strong_id,
            num_shots=self.num_shots,
            count_observable_error_combos=self.count_observable_error_combos,
            count_detection_events=self.count_detection_events,
        )


def auto_dem(circuit: 'stim.Circuit') -> 'stim.DetectorErrorModel':
    """Converts a circuit into a detector error model, with some fallbacks.

    First attempts to do it with folding and decomposition, then tries
    giving up on the folding, then tries giving up on the decomposition.
    """
    try:
        return circuit.detector_error_model(
            allow_gauge_detectors=False,
            approximate_disjoint_errors=True,
            block_decomposition_from_introducing_remnant_edges=False,
            decompose_errors=True,
            flatten_loops=False,
            ignore_decomposition_failures=False,
        )
    except ValueError:
        pass

    # This might be https://github.com/quantumlib/Stim/issues/393
    # Try turning off loop flattening.
    try:
        return circuit.detector_error_model(
            allow_gauge_detectors=False,
            approximate_disjoint_errors=True,
            block_decomposition_from_introducing_remnant_edges=False,
            decompose_errors=True,
            flatten_loops=False,
            ignore_decomposition_failures=False,
        )
    except ValueError:
        pass

    # Maybe decomposition is impossible, but the decoder might not need it.
    # Try turning off error decomposition.
    try:
        return circuit.detector_error_model(
            allow_gauge_detectors=False,
            approximate_disjoint_errors=True,
            block_decomposition_from_introducing_remnant_edges=False,
            decompose_errors=False,
            flatten_loops=True,
            ignore_decomposition_failures=False,
        )
    except ValueError:
        pass

    # Okay turn them both off...
    return circuit.detector_error_model(
        allow_gauge_detectors=False,
        approximate_disjoint_errors=True,
        block_decomposition_from_introducing_remnant_edges=False,
        decompose_errors=False,
        flatten_loops=True,
        ignore_decomposition_failures=False,
    )


class WorkOut:
    def __init__(
            self,
            *,
            work_key: Any,
            stats: Optional['sinter.AnonTaskStats'],
            strong_id: str,
            msg_error: Optional[Tuple[str, BaseException]]):
        self.work_key = work_key
        self.stats = stats
        self.strong_id = strong_id
        self.msg_error = msg_error


def worker_loop(tmp_dir: 'pathlib.Path',
                inp: 'multiprocessing.Queue',
                out: 'multiprocessing.Queue',
                custom_decoders: Optional[Dict[str, 'sinter.Decoder']],
                core_affinity: Optional[int]) -> None:
    try:
        if core_affinity is not None and hasattr(os, 'sched_setaffinity'):
            os.sched_setaffinity(0, {core_affinity})
    except:
        # If setting the core affinity fails, we keep going regardless.
        pass

    try:
        with tempfile.TemporaryDirectory(dir=tmp_dir) as child_dir:
            while True:
                work: Optional[WorkIn] = inp.get()
                if work is None:
                    return
                out.put(do_work_safely(work, child_dir, custom_decoders))
    except KeyboardInterrupt:
        pass


def do_work_safely(work: WorkIn, child_dir: str, custom_decoders: Dict[str, 'sinter.Decoder']) -> WorkOut:
    try:
        return do_work(work, child_dir, custom_decoders)
    except BaseException as ex:
        import traceback
        return WorkOut(
            work_key=work.work_key,
            stats=None,
            strong_id=work.strong_id,
            msg_error=(traceback.format_exc(), ex),
        )


def do_work(work: WorkIn, child_dir: str, custom_decoders: Dict[str, 'sinter.Decoder']) -> WorkOut:
    import stim
    from sinter._task import Task
    from sinter._decoding import sample_decode

    if work.strong_id is None:
        # The work is to compute the DEM, as opposed to taking shots.

        circuit = stim.Circuit.from_file(work.circuit_path)
        dem = auto_dem(circuit)
        dem.to_file(work.dem_path)

        task = Task(
            circuit=circuit,
            decoder=work.decoder,
            detector_error_model=dem,
            postselection_mask=work.postselection_mask,
            postselected_observables_mask=work.postselected_observables_mask,
            json_metadata=work.json_metadata,
        )

        return WorkOut(
            work_key=work.work_key,
            stats=None,
            strong_id=task.strong_id(),
            msg_error=None,
        )

    stats: 'sinter.AnonTaskStats' = sample_decode(
        num_shots=work.num_shots,
        circuit_path=work.circuit_path,
        circuit_obj=None,
        dem_path=work.dem_path,
        dem_obj=None,
        post_mask=work.postselection_mask,
        postselected_observable_mask=work.postselected_observables_mask,
        decoder=work.decoder,
        count_observable_error_combos=work.count_observable_error_combos,
        count_detection_events=work.count_detection_events,
        tmp_dir=child_dir,
        custom_decoders=custom_decoders,
    )

    return WorkOut(
        stats=stats,
        work_key=work.work_key,
        strong_id=work.strong_id,
        msg_error=None,
    )
