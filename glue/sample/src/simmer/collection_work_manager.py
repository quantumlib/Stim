import multiprocessing
import time
from typing import Optional, Iterator, Tuple, Dict, List

from simmer.collection_case_tracker import CollectionCaseTracker
from simmer.decoding import CaseStats, Case


def worker_loop(inp: multiprocessing.Queue, out: multiprocessing.Queue) -> None:
    while True:
        key_case = inp.get()
        if key_case == "end":
            return
        assert isinstance(key_case, tuple)
        key, case = key_case
        assert isinstance(key, int)
        assert isinstance(case, Case)
        try:
            stats = case.run()
        except Exception as ex:
            out.put(("error", ex))
            continue
        out.put(("result", key, case, stats))


class CollectionWorkManager:
    def __init__(self,
                 to_do: Iterator[CollectionCaseTracker],
                 max_shutdown_wait_seconds: float):
        self.results_queue: Optional[multiprocessing.Queue] = None
        self.problem_queue: Optional[multiprocessing.Queue] = None
        self.max_shutdown_wait_seconds = max_shutdown_wait_seconds

        self.workers: List[multiprocessing.Process] = []
        self.active_collectors: Dict[int, CollectionCaseTracker] = {}
        self.to_do: Iterator[CollectionCaseTracker] = to_do
        self.next_collecter_key: int = 0
        self.started_count = 0
        self.finished_count = 0
        self.deployed_jobs: Dict[int, Tuple[int, Case]] = {}
        self.next_job_id = 0

    def start_workers(self, num_workers: int) -> None:
        current_method = multiprocessing.get_start_method()
        try:
            # To ensure the child processes do not accidentally share ANY state
            # related to, we use 'spawn' instead of 'fork'.
            multiprocessing.set_start_method('spawn', force=True)
            # Create queues after setting start method to work around a deadlock
            # bug that occurs otherwise.
            self.results_queue = multiprocessing.Queue()
            self.problem_queue = multiprocessing.Queue()
            self.results_queue.cancel_join_thread()
            self.problem_queue.cancel_join_thread()

            for _ in range(num_workers):
                w = multiprocessing.Process(
                    target=worker_loop,
                    args=(self.problem_queue, self.results_queue))
                self.workers.append(w)
                w.start()
        finally:
            multiprocessing.set_start_method(current_method, force=True)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.shut_down_workers()

    def shut_down_workers(self) -> None:
        removed_workers = self.workers
        self.workers = []

        # Notify workers that they should stop.
        # (Though, in SIGINT situations the workers have separately received SIGINTs.)
        for _ in removed_workers:
            self.problem_queue.put("end")

        # Wait the maximum time, then bring the hammer down.
        deadline = time.monotonic() + self.max_shutdown_wait_seconds
        for w in removed_workers:
            max_wait_seconds = deadline - time.monotonic()
            if max_wait_seconds > 0:
                w.join(timeout=max_wait_seconds)
            if w.is_alive():
                w.terminate()

    def fill_work_queue(self) -> bool:
        while len(self.deployed_jobs) < len(self.workers):
            work = self.provide_more_work()
            if work is None:
                break
            self.problem_queue.put((self.next_job_id, work[1]))
            self.deployed_jobs[self.next_job_id] = work
            self.next_job_id += 1
        return bool(self.deployed_jobs)

    def wait_for_more_stats(self) -> Tuple[Case, CaseStats]:
        result = self.results_queue.get()
        assert isinstance(result, tuple)
        if result[0] == "error":
            raise RuntimeError("Job failed") from result[1]
        elif result[0] == "result":
            _, key, finished_problem, stats = result
            self.work_completed(self.deployed_jobs[key][0], stats)
            del self.deployed_jobs[key]
            return finished_problem, stats
        else:
            raise NotImplementedError(f'result={result!r}')

    def _iter_draw_collectors(self) -> Iterator[Tuple[int, CollectionCaseTracker]]:
        yield from self.active_collectors.items()
        while True:
            key = self.next_collecter_key
            try:
                collector = next(self.to_do)
            except StopIteration:
                return
            if collector.is_done():
                continue
            self.next_collecter_key += 1
            self.active_collectors[key] = collector
            self.started_count += 1
            yield key, collector

    def is_done(self) -> bool:
        return len(self.active_collectors) == 0

    def work_completed(self, key: int, stats: CaseStats):
        collector = self.active_collectors[key]
        collector.work_completed(stats)
        if collector.is_done():
            self.finished_count += 1
            del self.active_collectors[key]

    def provide_more_work(self) -> Optional[Tuple[int, Case]]:
        for key, collector in self._iter_draw_collectors():
            result = collector.provide_more_work()
            if result is not None:
                return key, result
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
        return main_status + ''.join(collector_statuses) + '\033[0m'
