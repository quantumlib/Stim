import contextlib
import dataclasses
import pathlib
from typing import Callable, Iterator, Optional, Union, Iterable, List, TYPE_CHECKING, Tuple

import math
import numpy as np
import stim

from sinter._collection_options import CollectionOptions
from sinter._csv_out import CSV_HEADER
from sinter._collection_work_manager import CollectionWorkManager
from sinter._existing_data import ExistingData
from sinter._printer import ThrottledProgressPrinter
from sinter._task_stats import TaskStats

if TYPE_CHECKING:
    import sinter


@dataclasses.dataclass(frozen=True)
class Progress:
    new_stats: Tuple[TaskStats, ...]
    status_message: str


def iter_collect(*,
                 num_workers: int,
                 tasks: Union[Iterator['sinter.Task'],
                              Iterable['sinter.Task']],
                 hint_num_tasks: Optional[int] = None,
                 additional_existing_data: Optional[ExistingData] = None,
                 max_shots: Optional[int] = None,
                 max_errors: Optional[int] = None,
                 decoders: Optional[Iterable[str]] = None,
                 max_batch_seconds: Optional[int] = None,
                 max_batch_size: Optional[int] = None,
                 start_batch_size: Optional[int] = None,
                 ) -> Iterator['sinter.Progress']:
    """Collects error correction statistics using multiple worker processes.

    Note: if max_batch_size and max_batch_seconds are both not used (or
    explicitly set to None), a default batch-size-limiting mechanism will be
    chosen.

    Args:
        num_workers: The number of worker processes to use.
        tasks: Decoding problems to sample.
        hint_num_tasks: If `tasks` is an iterator or a generator, its length
            can be given here so that progress printouts can say how many cases
            are left.
        additional_existing_data: Defaults to None (no additional data).
            Statistical data that has already been collected, in addition to
            anything included in each task's `previous_stats` field.
        decoders: Defaults to None (specified by each Task). The names of the
            decoders to use on each Task. It must either be the case that each
            Task specifies a decoder and this is set to None, or this is an
            iterable and each Task has its decoder set to None.
        max_shots: Defaults to None (unused). Stops the sampling process
            after this many samples have been taken from the circuit.
        max_errors: Defaults to None (unused). Stops the sampling process
            after this many errors have been seen in samples taken from the
            circuit. The actual number sampled errors may be larger due to
            batching.
        start_batch_size: Defaults to None (collector's choice). The very
            first shots taken from the circuit will use a batch of this
            size, and no other batches will be taken in parallel. Once this
            initial fact finding batch is done, batches can be taken in
            parallel and the normal batch size limiting processes take over.
        max_batch_size: Defaults to None (unused). Limits batches from
            taking more than this many shots at once. For example, this can
            be used to ensure memory usage stays below some limit.
        max_batch_seconds: Defaults to None (unused). When set, the recorded
            data from previous shots is used to estimate how much time is
            taken per shot. This information is then used to predict the
            biggest batch size that can finish in under the given number of
            seconds. Limits each batch to be no larger than that.

    Yields:
        sinter.SamplerStats values recording incremental statistical data
        as it is collected by workers.
    """
    if hint_num_tasks is None:
        try:
            # noinspection PyTypeChecker
            hint_num_tasks = len(tasks)
        except TypeError:
            pass

    with CollectionWorkManager(
            tasks=iter(tasks),
            global_collection_options=CollectionOptions(
                max_shots=max_shots,
                max_errors=max_errors,
                max_batch_seconds=max_batch_seconds,
                start_batch_size=start_batch_size,
                max_batch_size=max_batch_size,
            ),
            decoders=decoders,
            additional_existing_data=additional_existing_data) as manager:
        yield Progress(
            new_stats=(),
            status_message="Starting workers..."
        )
        manager.start_workers(num_workers)

        yield Progress(
            new_stats=(),
            status_message="Finding work..."
        )
        manager.fill_work_queue()
        yield Progress(
            new_stats=(),
            status_message=manager.status(num_circuits=hint_num_tasks)
        )

        while manager.fill_work_queue():
            # Wait for a worker to finish a job.
            sample = manager.wait_for_next_sample()
            manager.fill_work_queue()

            # Report the incremental results.
            yield Progress(
                new_stats=(sample,) if sample.shots > 0 else (),
                status_message=manager.status(num_circuits=hint_num_tasks),
            )


def collect(*,
            num_workers: int,
            tasks: Union[Iterator['sinter.Task'], Iterable['sinter.Task']],
            existing_data_filepaths: Iterable[Union[str, pathlib.Path]] = (),
            save_resume_filepath: Union[None, str, pathlib.Path] = None,
            progress_callback: Optional[Callable[['sinter.Progress'], None]] = None,
            max_shots: Optional[int] = None,
            max_errors: Optional[int] = None,
            decoders: Optional[Iterable[str]] = None,
            max_batch_seconds: Optional[int] = None,
            max_batch_size: Optional[int] = None,
            start_batch_size: Optional[int] = None,
            print_progress: bool = False,
            hint_num_tasks: Optional[int] = None,
            ) -> List['sinter.TaskStats']:
    """
    Args:
        num_workers: The number of worker processes to use.
        tasks: Decoding problems to sample.
        save_resume_filepath: Defaults to None (unused). If set to a filepath,
            results will be saved to that file while they are collected. If the
            python interpreter is stopped or killed, calling this method again
            with the same save_resume_filepath will load the previous results
            from the file so it can resume where it left off.

            The stats in this file will be counted in addition to each task's
            previous_stats field (as opposed to overriding the field).
        existing_data_filepaths: CSV data saved to these files will be loaded,
            included in the returned results, and count towards things like
            max_shots and max_errors.
        progress_callback: Defaults to None (unused). If specified, then each
            time new sample statistics are acquired from a worker this method
            will be invoked with the new sinter.SamplerStats.
        hint_num_tasks: If `tasks` is an iterator or a generator, its length
            can be given here so that progress printouts can say how many cases
            are left.
        decoders: Defaults to None (specified by each Task). The names of the
            decoders to use on each Task. It must either be the case that each
            Task specifies a decoder and this is set to None, or this is an
            iterable and each Task has its decoder set to None.
        max_shots: Defaults to None (unused). Stops the sampling process
            after this many samples have been taken from the circuit.
        max_errors: Defaults to None (unused). Stops the sampling process
            after this many errors have been seen in samples taken from the
            circuit. The actual number sampled errors may be larger due to
            batching.
        start_batch_size: Defaults to None (collector's choice). The very
            first shots taken from the circuit will use a batch of this
            size, and no other batches will be taken in parallel. Once this
            initial fact finding batch is done, batches can be taken in
            parallel and the normal batch size limiting processes take over.
        max_batch_size: Defaults to None (unused). Limits batches from
            taking more than this many shots at once. For example, this can
            be used to ensure memory usage stays below some limit.
        print_progress: When True, progress is printed to stderr while
            collection runs.
        max_batch_seconds: Defaults to None (unused). When set, the recorded
            data from previous shots is used to estimate how much time is
            taken per shot. This information is then used to predict the
            biggest batch size that can finish in under the given number of
            seconds. Limits each batch to be no larger than that.

    Returns:
        A list of sample statistics, one from each problem. The list is not in
        any specific order. This is the same data that would have been written
        to a CSV file, but aggregated so that each problem has exactly one
        sample statistic instead of potentially multiple.
    """
    # Load existing data.
    additional_existing_data = ExistingData()
    for existing in existing_data_filepaths:
        additional_existing_data += ExistingData.from_file(existing)

    if save_resume_filepath in existing_data_filepaths:
        raise ValueError("save_resume_filepath in existing_data_filepaths")

    progress_printer = ThrottledProgressPrinter(
        outs=[],
        print_progress=print_progress,
        min_progress_delay=1,
    )
    with contextlib.ExitStack() as exit_stack:
        # Open save/resume file.
        if save_resume_filepath is not None:
            save_resume_filepath = pathlib.Path(save_resume_filepath)
            if save_resume_filepath.exists():
                additional_existing_data += ExistingData.from_file(save_resume_filepath)
                save_resume_file = exit_stack.enter_context(
                        open(save_resume_filepath, 'a'))  # type: ignore
            else:
                save_resume_filepath.parent.mkdir(exist_ok=True)
                save_resume_file = exit_stack.enter_context(
                        open(save_resume_filepath, 'w'))  # type: ignore
                print(CSV_HEADER, file=save_resume_file, flush=True)
        else:
            save_resume_file = None

        # Collect data.
        result = ExistingData()
        result.data = dict(additional_existing_data.data)
        for progress in iter_collect(
            num_workers=num_workers,
            max_shots=max_shots,
            max_errors=max_errors,
            max_batch_seconds=max_batch_seconds,
            start_batch_size=start_batch_size,
            max_batch_size=max_batch_size,
            decoders=decoders,
            tasks=tasks,
            hint_num_tasks=hint_num_tasks,
            additional_existing_data=additional_existing_data,
        ):
            for stats in progress.new_stats:
                result.add_sample(stats)
                if save_resume_file is not None:
                    print(stats.to_csv_line(), file=save_resume_file, flush=True)
            if print_progress:
                progress_printer.show_latest_progress(progress.status_message)
            if progress_callback is not None:
                progress_callback(progress)

        return list(result.data.values())


def post_selection_mask_from_4th_coord(dem: Union[stim.Circuit, stim.DetectorErrorModel]) -> np.ndarray:
    num_dets = dem.num_detectors
    post_selection_mask = np.zeros(dtype=np.uint8, shape=math.ceil(num_dets / 8))
    for k, coord in dem.get_detector_coordinates().items():
        if len(coord) >= 4 and coord[3]:
            post_selection_mask[k // 8] |= 1 << (k % 8)
    return post_selection_mask
