import math
from typing import Iterator
from typing import Optional

from sinter.task import Task
from sinter.anon_task_stats import AnonTaskStats
from sinter.worker import WorkIn
from sinter.worker import WorkOut


class CollectionTrackerForSingleTask:
    def __init__(self, *, task: Task):
        self.task = task
        self.task_summary = task.to_case_executable().to_summary()
        self.finished_stats = task.previous_stats
        self.deployed_shots = 0
        self.deployed_processes = 0
        if self.task.max_shots is None and self.task.max_errors is None:
            raise ValueError('Neither the task nor the collector specified max_shots or max_errors. Mus specify one.')

    def expected_shots_remaining(
            self, *, safety_factor_on_shots_per_error: float = 1) -> float:
        """Doesn't include deployed shots."""
        result: float = float('inf')

        if self.task.max_shots is not None:
            result = self.task.max_shots - self.finished_stats.shots

        if self.finished_stats.errors and self.task.max_errors is not None:
            shots_per_error = self.finished_stats.shots / self.finished_stats.errors
            errors_left = self.task.max_errors - self.finished_stats.errors
            result = min(result, errors_left * shots_per_error * safety_factor_on_shots_per_error)

        return result

    def expected_time_per_shot(self) -> Optional[float]:
        if self.finished_stats.shots == 0:
            return None
        return self.finished_stats.seconds / self.finished_stats.shots

    def expected_errors_per_shot(self) -> Optional[float]:
        return (self.finished_stats.errors + 1) / (self.finished_stats.shots + 1)

    def expected_time_remaining(self) -> Optional[float]:
        dt = self.expected_time_per_shot()
        n = self.expected_shots_remaining()
        if dt is None or n == float('inf'):
            return None
        return dt * n

    def work_completed(self, result: WorkOut) -> None:
        self.deployed_shots -= result.sample.shots
        self.deployed_processes -= 1
        self.finished_stats += result.sample.to_case_stats()

    def is_done(self) -> bool:
        enough_shots = False
        if self.task.max_shots is not None and self.finished_stats.shots >= self.task.max_shots:
            enough_shots = True
        if self.task.max_errors is not None and self.finished_stats.errors >= self.task.max_errors:
            enough_shots = True
        return enough_shots and self.deployed_shots == 0

    def iter_batch_size_limits(self) -> Iterator[float]:
        if self.finished_stats.shots == 0:
            if self.deployed_shots > 0:
                yield 0
            elif self.task.start_batch_size is None:
                yield 100
            else:
                yield self.task.start_batch_size
            return

        # Do exponential ramp-up of batch sizes.
        yield self.finished_stats.shots * 2

        # Don't go super parallel before reaching other maximums.
        yield self.finished_stats.shots * 30 - self.deployed_shots

        # Don't take more shots than requested.
        if self.task.max_shots is not None:
            yield self.task.max_shots - self.finished_stats.shots - self.deployed_shots

        # Don't take more errors than requested.
        if self.task.max_errors is not None:
            errors_left = self.task.max_errors - self.finished_stats.errors
            errors_left += 2  # oversample once count gets low
            de = self.expected_errors_per_shot()
            yield errors_left / de - self.deployed_shots

        # Don't exceed max batch size.
        if self.task.max_batch_size is not None:
            yield self.task.max_batch_size

        # If no maximum on batch size is specified, default to 30s maximum.
        max_batch_seconds = self.task.max_batch_seconds
        if max_batch_seconds is None and self.task.max_batch_size is None:
            max_batch_seconds = 10

        # Try not to exceed max batch duration.
        if max_batch_seconds is not None:
            dt = self.expected_time_per_shot()
            if dt is not None and dt > 0:
                yield max(1, math.floor(max_batch_seconds / dt))

    def next_shot_count(self) -> int:
        return math.ceil(min(self.iter_batch_size_limits()))

    def provide_more_work(self) -> Optional[WorkIn]:
        # Wait to have *some* data before starting to sample in parallel.
        num_shots = self.next_shot_count()
        if num_shots <= 0:
            return None

        self.deployed_shots += num_shots
        self.deployed_processes += 1
        return WorkIn(
            key=None,
            task=self.task.to_case_executable(),
            summary=self.task_summary,
            num_shots=num_shots,
        )

    def status(self) -> str:
        t = self.expected_time_remaining()
        if t is not None:
            t /= 60
            t = math.ceil(t)
            t = f'{t}'
        terms = [
            'case: ',
            f'processes={self.deployed_processes}'.ljust(13),
            f'~core_mins_left={t}'.ljust(24),
        ]
        if self.task.max_shots is not None:
            terms.append(f'shots_left={max(0, self.task.max_shots - self.finished_stats.shots)}'.ljust(20))
        if self.task.max_errors is not None:
            terms.append(f'errors_left={max(0, self.task.max_errors - self.finished_stats.errors)}'.ljust(20))
        terms.append(f'{self.task.json_metadata}')
        return ''.join(terms)


def next_shot_count(prev_data: AnonTaskStats,
                    cur_batch_size: int,
                    max_batch_size: Optional[int],
                    max_errors: Optional[int],
                    max_shots: Optional[int]) -> int:
    result = cur_batch_size
    if max_shots is not None:
        result = min(result, max_shots - prev_data.shots)
    if prev_data.errors and max_errors is not None:
        result = min(result, math.ceil(1.1 * (max_errors - prev_data.errors) * prev_data.shots / prev_data.errors))
    if max_batch_size is not None:
        result = min(result, max_batch_size)
    return result
