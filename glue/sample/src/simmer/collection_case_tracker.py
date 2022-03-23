import dataclasses
import math
from typing import Optional

import stim

from simmer.case_goal import CaseGoal
from simmer.case_stats import CaseStats
from simmer.worker import WorkIn


class CollectionCaseTracker:
    def __init__(self,
                 *,
                 case_goal: CaseGoal,
                 start_batch_size: int,
                 max_batch_size: Optional[int]):
        self.case_goal = case_goal
        self.start_batch_size = start_batch_size
        self.max_batch_size = max_batch_size
        self.finished_stats = case_goal.previous_stats
        self.deployed_shots = 0
        self.deployed_processes = 0

    def expected_shots_remaining(self, *, safety_factor_on_shots_per_error: float = 1) -> int:
        """Doesn't include deployed shots."""
        result: float = self.case_goal.max_shots - self.finished_stats.shots

        if self.finished_stats.errors:
            shots_per_error = self.finished_stats.shots / self.finished_stats.errors
            errors_left = self.case_goal.max_errors - self.finished_stats.errors
            result = min(result, errors_left * shots_per_error * safety_factor_on_shots_per_error)

        return math.ceil(result)

    def expected_time_remaining(self) -> Optional[float]:
        if self.finished_stats.shots == 0:
            return None
        return self.finished_stats.seconds / self.finished_stats.shots * self.expected_shots_remaining()

    def work_completed(self, stats: CaseStats) -> None:
        self.deployed_shots -= stats.shots
        self.deployed_processes -= 1
        self.finished_stats += stats

    def is_done(self) -> bool:
        if self.finished_stats.shots >= self.case_goal.max_shots or self.finished_stats.errors >= self.case_goal.max_errors:
            if self.deployed_shots == 0:
                return True
        return False

    def next_shot_count(self) -> int:
        unfinished_shots = self.expected_shots_remaining(safety_factor_on_shots_per_error=1.1)
        result = unfinished_shots - self.deployed_shots
        result = min(result, max(self.start_batch_size, self.finished_stats.shots * 2))
        if self.max_batch_size is not None:
            result = min(result, self.max_batch_size)
        return result

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
            case=self.case_goal.case,
            num_shots=num_shots,
        )

    def status(self) -> str:
        t = self.expected_time_remaining()
        if self.deployed_processes == 0:
            t = None
        if t is not None:
            t /= 60
            t /= self.deployed_processes
            t = math.ceil(t)
            t = f'{t}min'
        return (
            'case: '
            + f'processes={self.deployed_processes}'.ljust(13)
            + f'eta={t}'.ljust(12)
            + f'shots_left={max(0, self.case_goal.max_shots - self.finished_stats.shots)}'.ljust(20)
            + f'errors_left={max(0, self.case_goal.max_errors - self.finished_stats.errors)}'.ljust(20)
            + f'{self.case_goal.case.custom}')


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
