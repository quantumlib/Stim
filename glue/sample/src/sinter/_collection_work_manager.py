import os

import contextlib
import multiprocessing
import pathlib
import tempfile
from typing import cast, Iterable, Optional, Iterator, Tuple, Dict, List

from sinter._decoding_decoder_class import Decoder
from sinter._collection_options import CollectionOptions
from sinter._existing_data import ExistingData
from sinter._task_stats import TaskStats
from sinter._task import Task
from sinter._anon_task_stats import AnonTaskStats
from sinter._collection_tracker_for_single_task import CollectionTrackerForSingleTask
from sinter._worker import worker_loop, WorkIn, WorkOut


class CollectionWorkManager:
    def __init__(self, *,
                 tasks_iter: Iterator[Task],
                 global_collection_options: CollectionOptions,
                 additional_existing_data: Optional[ExistingData],
                 decoders: Optional[Iterable[str]],
                 custom_decoders: Dict[str, Decoder]):
        self.custom_decoders = custom_decoders
        self.queue_from_workers: Optional[multiprocessing.Queue] = None
        self.queue_to_workers: Optional[multiprocessing.Queue] = None
        self.additional_existing_data = ExistingData() if additional_existing_data is None else additional_existing_data
        self.tmp_dir: Optional[pathlib.Path] = None
        self.exit_stack: Optional[contextlib.ExitStack] = None

        self.global_collection_options = global_collection_options
        self.decoders: Optional[Tuple[str, ...]] = None if decoders is None else tuple(decoders)
        self.did_work = False

        self.workers: List[multiprocessing.Process] = []
        self.active_collectors: Dict[int, CollectionTrackerForSingleTask] = {}
        self.next_collector_key: int = 0
        self.finished_count = 0
        self.deployed_jobs: Dict[int, WorkIn] = {}
        self.next_job_id = 0

        self.tasks_with_decoder_iter: Iterator[Task] = _iter_tasks_with_assigned_decoders(
            tasks_iter=tasks_iter,
            default_decoders=self.decoders,
            global_collections_options=self.global_collection_options)

    def start_workers(self, num_workers: int) -> None:
        assert self.tmp_dir is not None
        current_method = multiprocessing.get_start_method()
        try:
            # To ensure the child processes do not accidentally share ANY state
            # related to, we use 'spawn' instead of 'fork'.
            multiprocessing.set_start_method('spawn', force=True)
            # Create queues after setting start method to work around a deadlock
            # bug that occurs otherwise.
            self.queue_from_workers = multiprocessing.Queue()
            self.queue_to_workers = multiprocessing.Queue()
            self.queue_from_workers.cancel_join_thread()
            self.queue_to_workers.cancel_join_thread()

            num_cpus = os.cpu_count()
            for index in range(num_workers):
                w = multiprocessing.Process(
                    target=worker_loop,
                    args=(self.tmp_dir, self.queue_to_workers, self.queue_from_workers, self.custom_decoders, index % num_cpus))
                w.start()
                self.workers.append(w)
        finally:
            multiprocessing.set_start_method(current_method, force=True)

    def __enter__(self):
        self.exit_stack = contextlib.ExitStack().__enter__()
        self.tmp_dir = pathlib.Path(self.exit_stack.enter_context(tempfile.TemporaryDirectory()))
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.shut_down_workers()
        self.exit_stack.__exit__(exc_type, exc_val, exc_tb)
        self.exit_stack = None
        self.tmp_dir = None

    def shut_down_workers(self) -> None:
        removed_workers = self.workers
        self.workers = []

        # SIGKILL everything.
        for w in removed_workers:
            # This is supposed to be safe because all state on disk was put
            # in the specified tmp directory which we will handle deleting.
            w.kill()

    def fill_work_queue(self) -> bool:
        while len(self.deployed_jobs) < len(self.workers):
            work = self.provide_more_work()
            if work is None:
                break
            self.did_work = True
            self.queue_to_workers.put(work.with_work_key((self.next_job_id, work.work_key)))
            self.deployed_jobs[self.next_job_id] = work
            self.next_job_id += 1
        return bool(self.deployed_jobs)

    def wait_for_next_sample(self,
                             *,
                             timeout: Optional[float] = None,
                             ) -> TaskStats:
        result = self.queue_from_workers.get(timeout=timeout)
        assert isinstance(result, WorkOut)
        if result.msg_error is not None:
            msg, error = result.msg_error
            if isinstance(error, KeyboardInterrupt):
                raise KeyboardInterrupt()
            raise RuntimeError(f"Worker failed: {msg}") from error

        else:
            job_id, sub_key = result.work_key
            stats = result.stats
            work_in = self.deployed_jobs[job_id]

            self.work_completed(WorkOut(
                work_key=sub_key,
                stats=stats,
                strong_id=result.strong_id,
                msg_error=result.msg_error,
            ))
            del self.deployed_jobs[job_id]
            if stats is None:
                stats = AnonTaskStats()
            return TaskStats(
                strong_id=result.strong_id,
                decoder=work_in.decoder,
                json_metadata=work_in.json_metadata,
                shots=stats.shots,
                errors=stats.errors,
                discards=stats.discards,
                seconds=stats.seconds,
            )

    def _iter_draw_collectors(self, *, prefer_started: bool) -> Iterator[Tuple[int, CollectionTrackerForSingleTask]]:
        if prefer_started:
            yield from self.active_collectors.items()
        while True:
            key = self.next_collector_key
            try:
                task = next(self.tasks_with_decoder_iter)
            except StopIteration:
                break
            collector = CollectionTrackerForSingleTask(
                task=task,
                circuit_path=str((self.tmp_dir / f'circuit_{self.next_collector_key}.stim').absolute()),
                dem_path=str((self.tmp_dir / f'dem_{self.next_collector_key}.dem').absolute()),
                existing_data=self.additional_existing_data,
            )
            if collector.is_done():
                self.finished_count += 1
                continue
            self.next_collector_key += 1
            self.active_collectors[key] = collector
            yield key, collector
        if not prefer_started:
            yield from self.active_collectors.items()

    def is_done(self) -> bool:
        return len(self.active_collectors) == 0

    def work_completed(self, result: WorkOut):
        assert isinstance(result.work_key, int)
        collector_index = cast(int, result.work_key)
        collector = self.active_collectors[collector_index]
        collector.work_completed(result)
        if collector.is_done():
            self.finished_count += 1
            del self.active_collectors[collector_index]

    def provide_more_work(self) -> Optional[WorkIn]:
        iter_collectors = self._iter_draw_collectors(
                prefer_started=len(self.active_collectors) >= 2)
        for desperate in False, True:
            for collector_index, collector in iter_collectors:
                w = collector.provide_more_work(desperate=desperate)
                if w is not None:
                    assert w.work_key is None
                    return w.with_work_key(collector_index)
        return None

    def status(self, *, num_circuits: Optional[int]) -> str:
        if self.is_done():
            if self.did_work:
                main_status = 'Done collecting'
            else:
                main_status = 'There was nothing additional to collect'
        elif num_circuits is not None:
            main_status = f'{num_circuits - self.finished_count} cases left:'
        else:
            main_status = "Running..."
        collector_statuses = [
            collector.status()
            for collector in self.active_collectors.values()
        ]
        if len(collector_statuses) > 24:
            collector_statuses = collector_statuses[:24] + ['\n...']

        min_indent = 0
        while collector_statuses and all(min_indent < len(c) and c[min_indent] == ' ' for c in collector_statuses):
            min_indent += 1
        if min_indent > 4:
            collector_statuses = [c[min_indent - 4:] for c in collector_statuses]
        collector_statuses = ['\n' + c for c in collector_statuses]

        return main_status + ''.join(collector_statuses)


def _iter_tasks_with_assigned_decoders(
    *,
    tasks_iter: Iterator[Task],
    default_decoders: Optional[Iterable[str]],
    global_collections_options: CollectionOptions,
) -> Iterator[Task]:
    for task in tasks_iter:
        if task.decoder is None and default_decoders is None:
            raise ValueError("Decoders to use was not specified. decoders is None and task.decoder is None")
        task_decoders = []
        if default_decoders is not None:
            task_decoders.extend(default_decoders)
        if task.decoder is not None and task.decoder not in task_decoders:
            task_decoders.append(task.decoder)
        for decoder in task_decoders:
            yield Task(
                circuit=task.circuit,
                decoder=decoder,
                detector_error_model=task.detector_error_model,
                postselection_mask=task.postselection_mask,
                postselected_observables_mask=task.postselected_observables_mask,
                json_metadata=task.json_metadata,
                collection_options=task.collection_options.combine(global_collections_options),
                skip_validation=True,
            )
