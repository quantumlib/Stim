import sys
from typing import Iterator, Tuple, Optional, Union, Iterable

import numpy as np
import stim

from simmer.collection_work_manager import CollectionWorkManager
from simmer.case_executable import CaseExecutable
from simmer.case_stats import CaseStats
from simmer.case_goal import CaseGoal


def iter_collect(*,
                 num_workers: int,
                 start_batch_size: int = 100,
                 max_batch_size: Optional[int] = None,
                 max_batch_seconds: Optional[float] = None,
                 num_goals: Optional[int] = None,
                 goals: Union[Iterator[CaseGoal], Iterable[CaseGoal]],
                 max_shutdown_wait_seconds: float,
                 print_progress: bool) -> Iterator[Tuple[CaseExecutable, CaseStats]]:
    """Collects error correction statistics using multiple worker processes.

    Note: if max_batch_size and max_batch_seconds are both not used (or
    explicitly set to None), a default batch-size-limiting mechanism will be
    chosen.

    Args:
        num_workers: The number of worker processes to use.
        num_goals: The length of `iter_todo`.
        goals: Generates cases to sample, decode, and collect statistics from.
        start_batch_size: When first running a case, this is how many shots will
            be run as part of a single batch. The number of shots will then be
            dynamically adjusted upward as confidence about how long batches
            take increases.
        max_batch_size: Defaults to unused (None). Limits the maximum number of
            shots that can be put into a single batch.
        max_batch_seconds: Defaults to unused (None). Limits the maximum number
            of shots that can be put into a single batch by using recorded data
            to estimate how long a batch will take and picking a corresponding
            batch size that stays below the given maximum time.

            Note that this is not a *strict* maximum limit. Batches that take
            longer than this will not be forcefully terminated. Systemic
            effects that result in the estimated time taken by a batch to be
            wrong, such as running on a laptop that has just switched to battery
            power, will result in batches taking amounts of time that deviate
            from this target.
        max_shutdown_wait_seconds: If an exception is thrown (e.g. because the
            user sent a SIGINT to end the python program), this is the maximum
            amount of time the workers have to shut down gracefully before they
            are forcefully terminated.

            Note that workers create all temporary files within a specified
            directory, so that their temporary files can be cleaned up for them
            when they are forcefully terminated without the chance to do so for
            themselves.
        print_progress: When True, progress status is printed to stderr. Uses
            bash escape sequences to erase and rewrite the status as things
            progress.

    Yields:
        (case, stats) tuples recording incremental statistical data collected by workers.
    """
    if max_batch_size is None and max_batch_seconds is None:
        max_batch_seconds = 30

    if num_goals is None:
        try:
            # noinspection PyTypeChecker
            num_todo = len(goals)
        except TypeError:
            pass

    with CollectionWorkManager(
        to_do=iter(goals),
        max_shutdown_wait_seconds=max_shutdown_wait_seconds,
        start_batch_size=start_batch_size,
        max_batch_size=max_batch_size,
        max_batch_seconds=max_batch_seconds,
    ) as manager:
        manager.start_workers(num_workers)

        while manager.fill_work_queue():
            # Show status on stderr.
            if print_progress:
                status = manager.status(num_circuits=num_goals)
                print(status, flush=True, file=sys.stderr, end='')

            # Wait for a worker to finish a job.
            case, stats = manager.wait_for_more_stats()

            # Erase stderr status message.
            if print_progress:
                erase_current_line = f"\r\033[K"
                erase_previous_line = f"\033[1A" + erase_current_line
                num_prev_lines = len(status.split('\n')) - 1
                print(erase_current_line + erase_previous_line * num_prev_lines + '\033[0m', file=sys.stderr, end='')

            yield case, stats


def post_selection_mask_from_last_detector_coords(
        *,
        circuit: stim.Circuit,
        last_coord_minimum: Optional[int]) -> Optional[np.ndarray]:
    lvl = last_coord_minimum
    if lvl is None:
        return None
    coords = circuit.get_detector_coordinates()
    n = circuit.num_detectors + circuit.num_observables
    result = np.zeros(shape=(n + 7) // 8, dtype=np.uint8)
    for k, v in coords.items():
        if len(v) and v[-1] >= lvl:
            result[k >> 3] |= 1 << (k & 7)
    return result
