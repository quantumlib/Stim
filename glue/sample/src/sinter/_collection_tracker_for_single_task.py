import math
from typing import Iterator
from typing import Optional

import stim

from sinter._anon_task_stats import AnonTaskStats
from sinter._existing_data import ExistingData
from sinter._task import Task
from sinter._worker import WorkIn
from sinter._worker import WorkOut


DEFAULT_MAX_BATCH_SECONDS = 120


class CollectionTrackerForSingleTask:
    def __init__(
            self,
            *,
            task: Task,
            count_observable_error_combos: bool,
            count_detection_events: bool,
            circuit_path: str,
            dem_path: str,
            existing_data: ExistingData,
            custom_error_count_key: Optional[str],
    ):
        self.unfilled_task = task
        self.count_observable_error_combos = count_observable_error_combos
        self.count_detection_events = count_detection_events
        self.task_strong_id = None
        self.circuit_path = circuit_path
        self.dem_path = dem_path
        self.finished_stats = AnonTaskStats()
        self.existing_data = existing_data
        self.deployed_shots = 0
        self.waiting_for_worker_computing_dem_and_strong_id = False
        self.deployed_processes = 0
        self.custom_error_count_key = custom_error_count_key

        task.circuit.to_file(circuit_path)
        if task.detector_error_model is not None:
            task.detector_error_model.to_file(dem_path)
            self.task_strong_id = task.strong_id()
            existing = self.existing_data.data.get(self.task_strong_id)
            if existing is not None:
                self.finished_stats += existing.to_anon_stats()
        if self.copts.max_shots is None and self.copts.max_errors is None:
            raise ValueError('Neither the task nor the collector specified max_shots or max_errors. Must specify one.')

    @property
    def copts(self):
        return self.unfilled_task.collection_options

    def expected_shots_remaining(
            self, *, safety_factor_on_shots_per_error: float = 1) -> float:
        """Doesn't include deployed shots."""
        result: float = float('inf')

        if self.copts.max_shots is not None:
            result = self.copts.max_shots - self.finished_stats.shots

        errs = self._seen_errors()
        if errs and self.copts.max_errors is not None:
            shots_per_error = self.finished_stats.shots / errs
            errors_left = self.copts.max_errors - errs
            result = min(result, errors_left * shots_per_error * safety_factor_on_shots_per_error)

        return result

    def _seen_errors(self) -> int:
        if self.custom_error_count_key is not None:
            return self.finished_stats.custom_counts.get(self.custom_error_count_key, 0)
        return self.finished_stats.errors

    def expected_time_per_shot(self) -> Optional[float]:
        if self.finished_stats.shots == 0:
            return None
        return self.finished_stats.seconds / self.finished_stats.shots

    def expected_errors_per_shot(self) -> Optional[float]:
        return (self._seen_errors() + 1) / (self.finished_stats.shots + 1)

    def expected_time_remaining(self) -> Optional[float]:
        dt = self.expected_time_per_shot()
        n = self.expected_shots_remaining()
        if dt is None or n == float('inf'):
            return None
        return dt * n

    def work_completed(self, result: WorkOut) -> None:
        if self.waiting_for_worker_computing_dem_and_strong_id:
            self.task_strong_id = result.strong_id
            existing = self.existing_data.data.get(self.task_strong_id)
            if existing is not None:
                self.finished_stats += existing.to_anon_stats()
            self.waiting_for_worker_computing_dem_and_strong_id = False
        else:
            self.deployed_shots -= result.stats.shots
            self.finished_stats += result.stats
        self.deployed_processes -= 1

    def is_done(self) -> bool:
        if self.task_strong_id is None or self.waiting_for_worker_computing_dem_and_strong_id:
            return False
        enough_shots = False
        if self.copts.max_shots is not None and self.finished_stats.shots >= self.copts.max_shots:
            enough_shots = True
        if self.copts.max_errors is not None and self._seen_errors() >= self.copts.max_errors:
            enough_shots = True
        return enough_shots and self.deployed_shots == 0

    def iter_batch_size_limits(self, *, desperate: bool) -> Iterator[float]:
        if self.finished_stats.shots == 0:
            if self.deployed_shots > 0:
                yield 0
            elif self.copts.start_batch_size is None:
                yield 100
            else:
                yield self.copts.start_batch_size
            return

        # Do exponential ramp-up of batch sizes.
        yield self.finished_stats.shots * 2

        # Don't go super parallel before reaching other maximums.
        if not desperate:
            yield self.finished_stats.shots * 5 - self.deployed_shots

        # Don't take more shots than requested.
        if self.copts.max_shots is not None:
            yield self.copts.max_shots - self.finished_stats.shots - self.deployed_shots

        # Don't take more errors than requested.
        if self.copts.max_errors is not None:
            errors_left = self.copts.max_errors - self._seen_errors()
            errors_left += 2  # oversample once count gets low
            de = self.expected_errors_per_shot()
            yield errors_left / de - self.deployed_shots

        # Don't exceed max batch size.
        if self.copts.max_batch_size is not None:
            yield self.copts.max_batch_size

        # If no maximum on batch size is specified, default to 30s maximum.
        max_batch_seconds = self.copts.max_batch_seconds
        if max_batch_seconds is None and self.copts.max_batch_size is None:
            max_batch_seconds = DEFAULT_MAX_BATCH_SECONDS

        # Try not to exceed max batch duration.
        if max_batch_seconds is not None:
            dt = self.expected_time_per_shot()
            if dt is not None and dt > 0:
                yield max(1, math.floor(max_batch_seconds / dt))

    def next_shot_count(self, *, desperate: bool) -> int:
        return math.ceil(min(self.iter_batch_size_limits(desperate=desperate)))

    def provide_more_work(self, *, desperate: bool) -> Optional[WorkIn]:
        if self.task_strong_id is None:
            if self.waiting_for_worker_computing_dem_and_strong_id:
                return None
            self.waiting_for_worker_computing_dem_and_strong_id = True
            self.deployed_processes += 1
            return WorkIn(
                work_key=None,
                circuit_path=self.circuit_path,
                dem_path=self.dem_path,
                decoder=self.unfilled_task.decoder,
                postselection_mask=self.unfilled_task.postselection_mask,
                postselected_observables_mask=self.unfilled_task.postselected_observables_mask,
                json_metadata=self.unfilled_task.json_metadata,
                strong_id=None,
                num_shots=-1,
                count_observable_error_combos=self.count_observable_error_combos,
                count_detection_events=self.count_detection_events,
            )

        # Wait to have *some* data before starting to sample in parallel.
        num_shots = self.next_shot_count(desperate=desperate)
        if num_shots <= 0:
            return None

        self.deployed_shots += num_shots
        self.deployed_processes += 1
        return WorkIn(
            work_key=None,
            strong_id=self.task_strong_id,
            circuit_path=self.circuit_path,
            dem_path=self.dem_path,
            decoder=self.unfilled_task.decoder,
            postselection_mask=self.unfilled_task.postselection_mask,
            postselected_observables_mask=self.unfilled_task.postselected_observables_mask,
            json_metadata=self.unfilled_task.json_metadata,
            num_shots=num_shots,
            count_observable_error_combos=self.count_observable_error_combos,
            count_detection_events=self.count_detection_events,
        )

    def status(self) -> str:
        t = self.expected_time_remaining()
        if t is not None:
            t /= 60
            t = math.ceil(t)
            t = f'{t}'
        terms = [
            f'{self.unfilled_task.decoder} '.rjust(22),
            f'processes={self.deployed_processes}'.ljust(13),
            f'~core_mins_left={t}'.ljust(24),
        ]
        if self.task_strong_id is None:
            terms.append(f'(initializing...) ')
        else:
            if self.copts.max_shots is not None:
                terms.append(f'shots_left={max(0, self.copts.max_shots - self.finished_stats.shots)}'.ljust(20))
            if self.copts.max_errors is not None:
                terms.append(f'errors_left={max(0, self.copts.max_errors - self._seen_errors())}'.ljust(20))
        if isinstance(self.unfilled_task.json_metadata, dict):
            keys = self.unfilled_task.json_metadata.keys()
            try:
                keys = sorted(keys)
            except:
                keys = list(keys)
            meta_desc = '{' + ','.join(f'{k}={self.unfilled_task.json_metadata[k]}' for k in keys) + '}'
        else:
            meta_desc = f'{self.unfilled_task.json_metadata}'
        terms.append(meta_desc)

        return ''.join(terms)
