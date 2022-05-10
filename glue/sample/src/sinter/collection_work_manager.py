import contextlib
import multiprocessing
import pathlib
import tempfile
import time
from typing import Iterable
from typing import Optional, Iterator, Tuple, Dict, List

from sinter.existing_data import ExistingData
from sinter.task_stats import TaskStats
from sinter.task import Task
from sinter.collection_tracker_for_single_task import CollectionTrackerForSingleTask
from sinter.worker import worker_loop, WorkIn, WorkOut


class CollectionWorkManager:
    def __init__(self, *,
                 tasks: Iterator[Task],
                 max_shots: Optional[int],
                 max_errors: Optional[int],
                 max_batch_seconds: Optional[int],
                 start_batch_size: Optional[int],
                 max_batch_size: Optional[int],
                 additional_existing_data: Optional[ExistingData],
                 decoders: Optional[Iterable[str]]):
        self.queue_from_workers: Optional[multiprocessing.Queue] = None
        self.queue_to_workers: Optional[multiprocessing.Queue] = None
        self.additional_existing_data = ExistingData() if additional_existing_data is None else additional_existing_data
        self.tmp_dir: Optional[pathlib.Path] = None
        self.exit_stack: Optional[contextlib.ExitStack] = None

        self.max_errors = max_errors
        self.max_shots = max_shots
        self.max_batch_seconds = max_batch_seconds
        self.start_batch_size = start_batch_size
        self.max_batch_size = max_batch_size
        self.decoders = (None,) if decoders is None else tuple(decoders)

        self.workers: List[multiprocessing.Process] = []
        self.active_collectors: Dict[int, CollectionTrackerForSingleTask] = {}
        self.tasks: Iterator[Task] = tasks
        self.next_collector_key: int = 0
        self.finished_count = 0
        self.deployed_jobs: Dict[int, WorkIn] = {}
        self.next_job_id = 0

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

            for _ in range(num_workers):
                w = multiprocessing.Process(
                    target=worker_loop,
                    args=(self.tmp_dir, self.queue_to_workers, self.queue_from_workers))
                w.start()
                self.workers.append(w)
        finally:
            multiprocessing.set_start_method(current_method, force=True)

    def __enter__(self):
        self.exit_stack = contextlib.ExitStack().__enter__()
        self.tmp_dir = self.exit_stack.enter_context(tempfile.TemporaryDirectory())
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.shut_down_workers()
        self.exit_stack.__exit__(exc_type, exc_val, exc_tb)
        self.exit_stack = None
        self.tmp_dir = None

    def shut_down_workers(self) -> None:
        removed_workers = self.workers
        self.workers = []

        # Look, we don't have all day here.
        max_shutdown_wait_seconds = 0.1
        deadline = time.monotonic() + max_shutdown_wait_seconds

        # Notify workers that they should stop.
        for _ in removed_workers:
            self.queue_to_workers.put(None)

        # Ensure the workers are stopped.
        for w in removed_workers:
            # Wait until the deadline or until the worker stops.
            max_wait_seconds = deadline - time.monotonic()
            if max_wait_seconds > 0:
                w.join(timeout=max_wait_seconds)
            # Bring the hammer down.
            if w.is_alive():
                w.terminate()

    def fill_work_queue(self) -> bool:
        while len(self.deployed_jobs) < len(self.workers):
            work = self.provide_more_work()
            if work is None:
                break
            self.queue_to_workers.put(WorkIn(key=(self.next_job_id, work.key),
                                             task=work.task,
                                             summary=work.summary,
                                             num_shots=work.num_shots))
            self.deployed_jobs[self.next_job_id] = work
            self.next_job_id += 1
        return bool(self.deployed_jobs)

    def wait_for_next_sample(self,
                             *,
                             timeout: Optional[float] = None,
                             ) -> TaskStats:
        result = self.queue_from_workers.get(timeout=timeout)
        assert isinstance(result, WorkOut)
        if result.error is not None:
            raise RuntimeError("Worker failed") from result.error
        elif result.sample is not None:
            job_id, sub_key = result.key
            self.work_completed(WorkOut(key=sub_key,
                                        sample=result.sample,
                                        error=result.error))
            del self.deployed_jobs[job_id]
            return result.sample
        else:
            raise NotImplementedError(f'result={result!r}')

    def _iter_draw_collectors(self, *, prefer_started: bool) -> Iterator[Tuple[int, CollectionTrackerForSingleTask]]:
        if prefer_started:
            yield from self.active_collectors.items()
        while True:
            key = self.next_collector_key
            try:
                task = next(self.tasks)
            except StopIteration:
                break
            for decoder in self.decoders:
                collector = CollectionTrackerForSingleTask(
                    task=task.with_merged_options(
                        decoder=decoder,
                        max_shots=self.max_shots,
                        max_errors=self.max_errors,
                        max_batch_size=self.max_batch_size,
                        max_batch_seconds=self.max_batch_seconds,
                        start_batch_size=self.start_batch_size,
                        existing_data=self.additional_existing_data,
                    ),
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
        collector_index = result.key
        assert isinstance(collector_index, int)
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
                    assert w.key is None
                    return WorkIn(key=collector_index,
                                  task=w.task,
                                  summary=w.summary,
                                  num_shots=w.num_shots)
        return None

    def status(self, *, num_circuits: Optional[int]) -> str:
        main_status = '\033[31m'
        if num_circuits is not None:
            main_status += f'{num_circuits - self.finished_count} cases not finished yet'
        else:
            main_status += "Running..."
        collector_statuses = [
            '\n    ' + collector.status()
            for collector in self.active_collectors.values()
        ]
        if len(collector_statuses) > 10:
            collector_statuses = collector_statuses[:10] + ['\n...']
        return main_status + ''.join(collector_statuses) + '\033[0m'
