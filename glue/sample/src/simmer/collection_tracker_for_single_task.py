import math
from typing import Iterator
from typing import Optional

from simmer.existing_data import ExistingData
from simmer.task import Task
from simmer.case_stats import CaseStats
from simmer.worker import WorkIn
from simmer.worker import WorkOut


class CollectionTrackerForSingleTask:
    def __init__(self,
                 *,
                 task: Task,
                 additional_existing_data: ExistingData):
        self.task = task
        self.problem_summary = task.to_case_executable().to_summary()
        self.finished_stats = task.previous_stats + additional_existing_data.stats_for(self.problem_summary)
        self.deployed_shots = 0
        self.deployed_processes = 0

    def expected_shots_remaining(
            self, *, safety_factor_on_shots_per_error: float = 1) -> int:
        """Doesn't include deployed shots."""
        result: float = self.task.max_shots - self.finished_stats.shots

        if self.finished_stats.errors:
            shots_per_error = self.finished_stats.shots / self.finished_stats.errors
            errors_left = self.task.max_errors - self.finished_stats.errors
            result = min(result, errors_left * shots_per_error * safety_factor_on_shots_per_error)

        return math.ceil(result)

    def expected_time_per_shot(self) -> Optional[float]:
        if self.finished_stats.shots == 0:
            return None
        return self.finished_stats.seconds / self.finished_stats.shots

    def expected_time_remaining(self) -> Optional[float]:
        dt = self.expected_time_per_shot()
        if dt is None:
            return None
        return dt * self.expected_shots_remaining()

    def work_completed(self, result: WorkOut) -> None:
        self.deployed_shots -= result.sample.shots
        self.deployed_processes -= 1
        self.finished_stats += result.sample.to_case_stats()

    def is_done(self) -> bool:
        if self.finished_stats.shots >= self.task.max_shots or self.finished_stats.errors >= self.task.max_errors:
            if self.deployed_shots == 0:
                return True
        return False

    def iter_batch_size_limits(self) -> Iterator[int]:
        yield max(self.task.start_batch_size, self.finished_stats.shots * 2)
        unfinished_shots = self.expected_shots_remaining(
                safety_factor_on_shots_per_error=1.1)
        yield unfinished_shots - self.deployed_shots
        if self.task.max_batch_size is not None:
            yield self.task.max_batch_size
        if self.task.max_batch_seconds is not None:
            dt = self.expected_time_per_shot()
            if dt is not None:
                yield max(1, math.floor(self.task.max_batch_seconds / dt))

    def next_shot_count(self) -> int:
        return min(self.iter_batch_size_limits())

    def provide_more_work(self) -> Optional[WorkIn]:
        # Don't massively oversample when finishing.
        if self.deployed_shots > self.expected_shots_remaining():
            return None

        # Wait to have *some* data before starting to sample in parallel.
        if self.finished_stats.shots == 0 and self.deployed_shots > 0:
            return None

        num_shots = self.next_shot_count()
        if num_shots <= 0:
            return None

        self.deployed_shots += num_shots
        self.deployed_processes += 1
        return WorkIn(
            key=None,
            case=self.task.to_case_executable(),
            summary=self.problem_summary,
            num_shots=num_shots,
        )

    def status(self) -> str:
        t = self.expected_time_remaining()
        if t is not None:
            t /= 60
            t = math.ceil(t)
            t = f'{t}'
        return (
            'case: '
            + f'processes={self.deployed_processes}'.ljust(13)
            + f'~core_mins_left={t}'.ljust(24)
            + f'shots_left={max(0, self.task.max_shots - self.finished_stats.shots)}'.ljust(20)
            + f'errors_left={max(0, self.task.max_errors - self.finished_stats.errors)}'.ljust(20)
            + f'{self.task.json_metadata}')


def next_shot_count(prev_data: CaseStats,
                    cur_batch_size: int,
                    max_batch_size: Optional[int],
                    max_errors: int,
                    max_shots: int) -> int:
    result = cur_batch_size
    result = min(result, max_shots - prev_data.shots)
    if prev_data.errors:
        result = min(result, math.ceil(1.1 * (max_errors - prev_data.errors) * prev_data.shots / prev_data.errors))
    if max_batch_size is not None:
        result = min(result, max_batch_size)
    return result
