import time

from sinter._decoding import Sampler, CompiledSampler
from sinter._data import Task, AnonTaskStats


class RampThrottledSampler(Sampler):
    """Wraps a sampler to adjust requested shots to hit a target time.

    This sampler will initially only take 1 shot per call. If the time taken
    significantly undershoots the target time, the maximum number of shots per
    call is increased by a constant factor. If it exceeds the target time, the
    maximum is reduced by a constant factor. The result is that the sampler
    "ramps up" how many shots it does per call until it takes roughly the target
    time, and then dynamically adapts to stay near it.
    """

    def __init__(self, sub_sampler: Sampler, target_batch_seconds: float, max_batch_shots: int):
        self.sub_sampler = sub_sampler
        self.target_batch_seconds = target_batch_seconds
        self.max_batch_shots = max_batch_shots

    def __str__(self) -> str:
        return f'CompiledRampThrottledSampler({self.sub_sampler})'

    def compiled_sampler_for_task(self, task: Task) -> CompiledSampler:
        compiled_sub_sampler = self.sub_sampler.compiled_sampler_for_task(task)
        if compiled_sub_sampler.handles_throttling():
            return compiled_sub_sampler

        return CompiledRampThrottledSampler(
            sub_sampler=compiled_sub_sampler,
            target_batch_seconds=self.target_batch_seconds,
            max_batch_shots=self.max_batch_shots,
        )


class CompiledRampThrottledSampler(CompiledSampler):
    def __init__(self, sub_sampler: CompiledSampler, target_batch_seconds: float, max_batch_shots: int):
        self.sub_sampler = sub_sampler
        self.target_batch_seconds = target_batch_seconds
        self.batch_shots = 1
        self.max_batch_shots = max_batch_shots

    def __str__(self) -> str:
        return f'CompiledRampThrottledSampler({self.sub_sampler})'

    def sample(self, max_shots: int) -> AnonTaskStats:
        t0 = time.monotonic()
        actual_shots = min(max_shots, self.batch_shots)
        result = self.sub_sampler.sample(actual_shots)
        dt = time.monotonic() - t0

        # Rebalance number of shots.
        if self.batch_shots > 1 and dt > self.target_batch_seconds * 1.3:
            self.batch_shots //= 2
        if result.shots * 2 >= actual_shots:
            for _ in range(4):
                if self.batch_shots * 2 > self.max_batch_shots:
                    break
                if dt > self.target_batch_seconds * 0.3:
                    break
                self.batch_shots *= 2
                dt *= 2

        return result
