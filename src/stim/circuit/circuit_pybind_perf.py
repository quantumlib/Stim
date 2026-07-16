import math
import time
from typing import Callable, Iterable

import stim


def si2(val: float) -> str:
    """Describe quantity as an SI-prefixed value with two significant figures."""
    unit = ' '
    if val < 1:
        if val < 1:
            val *= 1000
            unit = 'm'
        if val < 1:
            val *= 1000
            unit = 'u'
        if val < 1:
            val *= 1000
            unit = 'n'
        if val < 1:
            val *= 1000
            unit = 'p'
    else:
        if val > 1000:
            val /= 1000
            unit = 'k'
        if val > 1000:
            val /= 1000
            unit = 'M'
        if val > 1000:
            val /= 1000
            unit = 'G'
        if val > 1000:
            val /= 1000
            unit = 'T'
    if 1 <= val < 10:
        return f'''{math.floor(val)}.{math.floor(val * 10) % 10} {unit}'''
    elif 10 <= val < 100:
        return f''' {math.floor(val)} {unit}'''
    elif 100 <= val < 1000:
        return f'''{math.floor(val / 10) * 10} {unit}'''
    else:
        return f'''{val} {unit}'''

BENCHMARK_CONFIG_TARGET_SECONDS = 0.5

class BenchmarkResult:
    def __init__(
            self,
            *,
            name: str | None,
            total_seconds: float,
            total_reps: int,
            units: Iterable[tuple[str, float]],
            goal_seconds: None | float,
    ):
        self.name = name
        self.total_seconds = total_seconds
        self.total_reps = total_reps
        self.goal_seconds: float | None = goal_seconds
        self.units: tuple[tuple[str, float], ...] = tuple(units)

    def __str__(self) -> str:
        parts = []
        actual_seconds_per_rep = self.total_seconds / self.total_reps
        if self.goal_seconds is not None:
            deviation = round((math.log(self.goal_seconds) - math.log(actual_seconds_per_rep)) / (math.log(10) / 10.0))
            parts.append("[")
            for k in range(-20, 21):
                if (k < deviation and k < 0) or (k > deviation and k > 0):
                    parts.append('.')
                elif k == deviation:
                    parts.append('*')
                elif k == 0:
                    parts.append('|')
                elif deviation < 0:
                    parts.append('<')
                else:
                    parts.append('>')
            parts.append("] ")
            parts.append(si2(actual_seconds_per_rep))
            parts.append("s")
            parts.append(" (vs ")
            parts.append(si2(self.goal_seconds))
            parts.append("s) ")
        else:
            parts.append(si2(actual_seconds_per_rep))
            parts.append("s ")
        for unit, multiplier in self.units:
            parts.append("(")
            parts.append(si2(self.total_reps / self.total_seconds * multiplier))
            parts.append(unit)
            parts.append("/s) ")
        if self.name is not None:
            parts.append(self.name)
        return ''.join(parts)


def benchmark_go(
        body: Callable,
        *,
        target_wait_time_seconds: float = BENCHMARK_CONFIG_TARGET_SECONDS,
) -> BenchmarkResult:
    """Benchmarks how long it takes to run a method.

    Args:
        body: The method to time.
        target_wait_time_seconds: How long to spend timing.

    Returns:
        The number of shots completed and the amount of time spent.
    """

    total_reps: int = 0
    total_seconds: float = 0.0

    rep_limit = 1
    while total_seconds < target_wait_time_seconds:
        remaining_time: float = target_wait_time_seconds - total_seconds
        reps: int = rep_limit
        if total_seconds > 0:
            reps = int(remaining_time * total_reps // total_seconds)
            if reps < total_reps * 0.1:
                break
            if reps > rep_limit:
                reps = rep_limit
            if reps < 1:
                reps = 1
        start_s = time.monotonic()
        for _ in range(reps):
            body()
        end_s = time.monotonic()
        dt_s = end_s - start_s
        total_reps += reps
        total_seconds += dt_s
        rep_limit *= 100

    return BenchmarkResult(
        total_seconds=total_seconds,
        total_reps=total_reps,
        name=None,
        units=(),
        goal_seconds=None,
    )


_REGISTERED_BENCHMARKS = []

def benchmark(
        original_method: Callable[[], BenchmarkResult] | None = None,
        /,
        *,
        goal_nanos: float | None = None,
        goal_micros: float | None = None,
        goal_millis: float | None = None,
        units: dict[str, float | int] | None = None,
):
    """A decorator marking a method as a benchmark."""
    assert (goal_micros is not None) + (goal_millis is not None) + (goal_nanos is not None) <= 1, "Specified mulitple goal units."
    goal_seconds: float | None = None
    if goal_micros is not None:
        goal_seconds = goal_micros * 1e-6
    if goal_millis is not None:
        goal_seconds = goal_millis * 1e-3
    if goal_nanos is not None:
        goal_seconds = goal_nanos * 1e-9

    def wrap(method: Callable[[], BenchmarkResult], /):
        def run():
            result = method()
            return BenchmarkResult(
                name=method.__name__,
                total_reps=result.total_reps,
                total_seconds=result.total_seconds,
                units=() if units is None else units.items(),
                goal_seconds=goal_seconds,
            )
        _REGISTERED_BENCHMARKS.append(run)
        return run

    if original_method is None:
        return wrap
    else:
        return wrap(original_method)


@benchmark(goal_micros=27, units={"targets": 2000})
def benchmark_circuit_append_int_in_large_chunks():
    c = stim.Circuit()
    targets = [0, 1] * 1000

    def run():
        c.clear()
        c.append("CX", targets)
    return benchmark_go(run)


@benchmark(goal_micros=320, units={"targets": 2000})
def benchmark_circuit_append_int_in_small_chunks():
    c = stim.Circuit()
    targets = [0, 1]

    def run():
        c.clear()
        for _ in range(1000):
            c.append("CX", targets)
    return benchmark_go(run)


@benchmark(goal_micros=25, units={"targets": 2000})
def benchmark_circuit_append_gate_targets_in_large_chunks():
    c = stim.Circuit()
    targets = [stim.GateTarget(0), stim.GateTarget(1)] * 1000

    def run():
        c.clear()
        c.append("CX", targets)
    return benchmark_go(run)


@benchmark(goal_micros=46, units={"targets": 2000})
def benchmark_circuit_append_pauli_strings_in_large_chunks():
    c = stim.Circuit()
    targets = [stim.PauliString("XX")] * 1000

    def run():
        c.clear()
        c.append("MPP", targets)
    return benchmark_go(run)


@benchmark(goal_micros=15, units={"targets": 2000})
def benchmark_circuit_append_text_in_large_chunks():
    c = stim.Circuit()
    content = "CX" + " 0 1" * 1000

    def run():
        c.clear()
        c.append_from_stim_program_text(content)
    return benchmark_go(run)


@benchmark(goal_micros=190, units={"targets": 2000})
def benchmark_circuit_append_text_in_small_chunks():
    c = stim.Circuit()
    content = "CX 0 1"

    def run():
        c.clear()
        for _ in range(1000):
            c.append_from_stim_program_text(content)
    return benchmark_go(run)


def main():
    for e in _REGISTERED_BENCHMARKS:
        print(e())


if __name__ == '__main__':
    main()