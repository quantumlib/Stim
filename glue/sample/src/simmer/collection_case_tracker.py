import dataclasses
import math
from typing import Optional

import numpy as np
import stim

from simmer.decoding import CaseStats, Case


@dataclasses.dataclass
class CollectionCaseTracker:
    circuit: stim.Circuit
    name: str
    post_mask: Optional[np.ndarray]
    strong_id: str
    start_batch_size: int
    max_batch_size: Optional[int]
    max_errors: int
    max_shots: int
    decoder: str

    finished_stats: CaseStats = CaseStats()
    deployed_shots: int = 0
    deployed_processes: int = 0
    dem: Optional[stim.DetectorErrorModel] = None

    def expected_shots_remaining(self, *, safety_factor_on_shots_per_error: float = 1) -> int:
        """Doesn't include deployed shots."""
        result: float = self.max_shots - self.finished_stats.num_shots

        if self.finished_stats.num_errors:
            shots_per_error = self.finished_stats.num_shots / self.finished_stats.num_errors
            errors_left = self.max_errors - self.finished_stats.num_errors
            result = min(result, errors_left * shots_per_error * safety_factor_on_shots_per_error)

        return math.ceil(result)

    def expected_time_remaining(self) -> Optional[float]:
        if self.finished_stats.num_shots == 0:
            return None
        return self.finished_stats.seconds_elapsed / self.finished_stats.num_shots * self.expected_shots_remaining()

    def work_completed(self, stats: CaseStats) -> None:
        self.deployed_shots -= stats.num_shots
        self.deployed_processes -= 1
        self.finished_stats += stats

    def is_done(self) -> bool:
        if self.finished_stats.num_shots >= self.max_shots or self.finished_stats.num_errors >= self.max_errors:
            if self.deployed_shots == 0:
                return True
        return False

    def next_shot_count(self) -> int:
        unfinished_shots = self.expected_shots_remaining(safety_factor_on_shots_per_error=1.1)
        result = unfinished_shots - self.deployed_shots
        result = min(result, max(self.start_batch_size, self.finished_stats.num_shots * 2))
        if self.max_batch_size is not None:
            result = min(result, self.max_batch_size)
        return result

    def provide_more_work(self) -> Optional[Case]:
        # Don't massively oversample when finishing.
        if self.deployed_shots > self.expected_shots_remaining():
            return None

        # Wait to have *some* data before starting to sample the same case in parallel.
        if self.finished_stats.num_shots == 0 and self.deployed_shots > 0:
            return None

        if self.dem is None:
            self.dem = self.circuit.detector_error_model(decompose_errors=True)
        num_shots = self.next_shot_count()
        if num_shots <= 0:
            return None

        self.deployed_shots += num_shots
        self.deployed_processes += 1
        return Case(
            name=self.name,
            num_shots=num_shots,
            circuit=self.circuit,
            dem=self.dem,
            post_mask=self.post_mask,
            decoder=self.decoder,
            strong_id=self.strong_id,
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
            + f'shots_left={max(0, self.max_shots - self.finished_stats.num_shots)}'.ljust(20)
            + f'errors_left={max(0, self.max_errors - self.finished_stats.num_errors)}'.ljust(20)
            + f'{self.name}')


def next_shot_count(prev_data: CaseStats,
                    cur_batch_size: int,
                    max_batch_size: Optional[int],
                    max_errors: int,
                    max_shots: int) -> int:
    result = cur_batch_size
    result = min(result, max_shots - prev_data.num_shots)
    if prev_data.num_errors:
        result = min(result, math.ceil(1.1 * (max_errors - prev_data.num_errors) * prev_data.num_shots / prev_data.num_errors))
    if max_batch_size is not None:
        result = min(result, max_batch_size)
    return result
