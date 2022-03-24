import sys
from typing import Iterator, Optional, Union, Iterable, List
from typing import TYPE_CHECKING

import numpy as np
import stim

from simmer.collection_work_manager import CollectionWorkManager
from simmer.existing_data import ExistingData
from simmer.task import Task

if TYPE_CHECKING:
    import simmer


def iter_collect(*,
                 num_workers: int,
                 tasks: Union[Iterator['simmer.Task'],
                              Iterable['simmer.Task']],
                 print_progress: bool = False,
                 hint_num_tasks: Optional[int] = None,
                 ) -> Iterator['simmer.SampleStats']:
    """Collects error correction statistics using multiple worker processes.

    Note: if max_batch_size and max_batch_seconds are both not used (or
    explicitly set to None), a default batch-size-limiting mechanism will be
    chosen.

    Args:
        num_workers: The number of worker processes to use.
        tasks: Decoding problems to sample.
        print_progress: When True, progress status is printed to stderr. Uses
            bash escape sequences to erase and rewrite the status as things
            progress.
        hint_num_tasks: If `tasks` is an iterator or a generator, its length
            can be given here so that progress printouts can say how many cases
            are left.

    Yields:
        simmer.SamplerStats values recording incremental statistical data
        as it is collected by workers.
    """
    if hint_num_tasks is None:
        try:
            # noinspection PyTypeChecker
            hint_num_tasks = len(tasks)
        except TypeError:
            pass

    with CollectionWorkManager(tasks=iter(tasks)) as manager:
        manager.start_workers(num_workers)

        while manager.fill_work_queue():
            # Show status on stderr.
            if print_progress:
                status = manager.status(num_circuits=hint_num_tasks)
                print(status, flush=True, file=sys.stderr, end='')

            # Wait for a worker to finish a job.
            sample = manager.wait_for_next_sample()

            # Erase stderr status message.
            if print_progress:
                erase_current_line = f"\r\033[K"
                erase_previous_line = f"\033[1A" + erase_current_line
                num_prev_lines = len(status.split('\n')) - 1
                print(erase_current_line + erase_previous_line * num_prev_lines + '\033[0m', file=sys.stderr, end='', flush=False)

            yield sample


def collect(*,
            num_workers: int,
            tasks: Union[Iterator[Task], Iterable[Task]],
            print_progress: bool = False,
            ) -> List['simmer.SampleStats']:
    """
    Args:
        num_workers: The number of worker processes to use.
        tasks: Decoding problems to sample.
        print_progress: When True, progress status is printed to stderr. Uses
            bash escape sequences to erase and rewrite the status as things
            progress.

    Returns:
        A list of sample statistics, one from each problem. The list is not in
        any specific order. This is the same data that would have been written
        to a CSV file, but aggregated so that each problem has exactly one
        sample statistic instead of potentially multiple.
    """
    result = ExistingData()
    for sample in iter_collect(
        num_workers=num_workers,
        tasks=tasks,
        print_progress=print_progress,
    ):
        result.add_sample(sample)
    return list(result.data.values())


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
