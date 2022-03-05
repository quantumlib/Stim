import sys
from typing import Iterator, Tuple, Optional

import numpy as np
import stim

from simmer.collection_case_tracker import CollectionCaseTracker
from simmer.collection_work_manager import CollectionWorkManager
from simmer.decoding import CaseStats, Case


def iter_collect(*,
                 num_workers: int,
                 num_todo: Optional[int] = None,
                 iter_todo: Iterator[CollectionCaseTracker],
                 max_shutdown_wait_seconds: float,
                 print_progress: bool) -> Iterator[Tuple[Case, CaseStats]]:
    """Collects error correction statistics using multiple worker processes.

    Args:
        num_workers: The number of worker processes to use.
        num_todo: The length of `iter_todo`.
        iter_todo: Generates cases to sample, decode, and collect statistics from.
        max_shutdown_wait_seconds: If an exception is thrown (e.g. because the user sent a SIGINT
            to end the python program), this is the maximum amount of time the workers have to
            shut down gracefully before they are forcefully terminated.
        print_progress: When True, progress status is printed to stderr.

    Yields:
        (case, stats) tuples recording incremental statistical data collected by workers.
    """
    if num_todo is None:
        try:
            # noinspection PyTypeChecker
            num_todo = len(iter_todo)
        except TypeError:
            pass
    iter_todo = iter(iter_todo)

    with CollectionWorkManager(
        to_do=iter_todo,
        max_shutdown_wait_seconds=max_shutdown_wait_seconds,
    ) as manager:
        manager.start_workers(num_workers)

        while manager.fill_work_queue():
            # Show status on stderr.
            if print_progress:
                status = manager.status(num_circuits=num_todo)
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
