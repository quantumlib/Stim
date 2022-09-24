import os

from typing import Any, Optional, Tuple, TYPE_CHECKING
import tempfile

if TYPE_CHECKING:
    import multiprocessing
    import pathlib
    import sinter
    import stim


def auto_dem(circuit: 'stim.Circuit') -> 'stim.DetectorErrorModel':
    return circuit.detector_error_model(
        allow_gauge_detectors=False,
        approximate_disjoint_errors=True,
        block_decomposition_from_introducing_remnant_edges=False,
        decompose_errors=True,
        flatten_loops=True,
        ignore_decomposition_failures=False,
    )


class WorkIn:
    def __init__(
            self,
            *,
            work_key: Any,
            task: 'sinter.Task',
            num_shots: int):
        self.work_key = work_key
        self.task = task
        self.num_shots = num_shots

    def with_work_key(self, work_key: Any) -> 'WorkIn':
        return WorkIn(
            work_key=work_key,
            task=self.task,
            num_shots=self.num_shots,
        )


class WorkOut:
    def __init__(
            self,
            *,
            work_key: Any,
            stats: Optional['sinter.AnonTaskStats'],
            filled_in_dem: Optional['stim.DetectorErrorModel'],
            filled_in_strong_id: Optional[str],
            msg_error: Optional[Tuple[str, BaseException]]):
        self.work_key = work_key
        self.stats = stats
        self.filled_in_dem = filled_in_dem
        self.filled_in_strong_id = filled_in_strong_id
        self.msg_error = msg_error


def worker_loop(tmp_dir: 'pathlib.Path',
                inp: 'multiprocessing.Queue',
                out: 'multiprocessing.Queue',
                core_affinity: Optional[int]) -> None:
    try:
        if core_affinity is not None:
            os.sched_setaffinity(0, {core_affinity})
        with tempfile.TemporaryDirectory(dir=tmp_dir) as child_dir:
            while True:
                work: Optional[WorkIn] = inp.get()
                if work is None:
                    return
                assert isinstance(work, WorkIn)

                used_task = work.task
                if work.task.detector_error_model is None:
                    from sinter._task import Task
                    used_task = Task(
                        circuit=work.task.circuit,
                        decoder=work.task.decoder,
                        detector_error_model=auto_dem(work.task.circuit),
                        postselection_mask=work.task.postselection_mask,
                        postselected_observables_mask=work.task.postselected_observables_mask,
                        json_metadata=work.task.json_metadata,
                        collection_options=work.task.collection_options,
                        skip_validation=True,
                    )

                try:
                    from sinter._decoding import sample_decode
                    stats: 'sinter.AnonTaskStats' = sample_decode(
                        num_shots=work.num_shots,
                        circuit=used_task.circuit,
                        post_mask=used_task.postselection_mask,
                        postselected_observable_mask=used_task.postselected_observables_mask,
                        decoder_error_model=used_task.detector_error_model,
                        decoder=used_task.decoder,
                        tmp_dir=child_dir,
                    )
                except BaseException as ex:
                    import traceback
                    out.put(WorkOut(
                        work_key=work.work_key,
                        stats=None,
                        filled_in_dem=None,
                        filled_in_strong_id=None,
                        msg_error=(traceback.format_exc(), ex),
                    ))
                    continue

                from sinter._anon_task_stats import AnonTaskStats
                stats = AnonTaskStats(
                    shots=stats.shots,
                    discards=stats.discards,
                    errors=stats.errors,
                    seconds=stats.seconds,
                )
                out.put(WorkOut(
                    stats=stats,
                    filled_in_dem=None if used_task is work.task else used_task.detector_error_model,
                    filled_in_strong_id=None if used_task is work.task else used_task.strong_id(),
                    work_key=work.work_key,
                    msg_error=None,
                ))
    except KeyboardInterrupt:
        pass
