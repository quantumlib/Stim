import contextlib
import dataclasses
import pathlib
from typing import Any
from typing import Callable, Iterator, Optional, Union, Iterable, List, TYPE_CHECKING, Tuple, Dict

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
    """Describes statistics and status messages from ongoing sampling.

    This is the type yielded by `sinter.iter_collect`, and given to the
    `progress_callback` argument of `sinter.collect`.

    Attributes:
        new_stats: New sampled statistics collected since the last progress
            update.
        status_message: A free form human readable string describing the current
            collection status, such as the number of tasks left and the
            estimated time to completion for each task.
    """
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
                 split_errors: bool = False,
                 custom_decoders: Optional[Dict[str, 'sinter.Decoder']] = None,
                 ) -> Iterator['sinter.Progress']:
    """Iterates error correction statistics collected from worker processes.

    It is important to iterate until the sequence ends, or worker processes will
    be left alive. The values yielded during iteration are progress updates from
    the workers.

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
        split_errors: Defaults to False. When set to to True, the returned
            TaskStats instances will have a non-None `classified_errors` field
            where keys are bitmasks identifying which observables flipped and
            values are the number of errors where that happened.
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
        custom_decoders: Custom decoders that can be used if requested by name.
            If not specified, only decoders built into sinter, such as
            'pymatching' and 'fusion_blossom', can be used.

    Yields:
        sinter.Progress instances recording incremental statistical data as it
        is collected by workers.

    Examples:
        >>> import sinter
        >>> import stim
        >>> tasks = [
        ...     sinter.Task(
        ...         circuit=stim.Circuit.generated(
        ...             'repetition_code:memory',
        ...             distance=5,
        ...             rounds=5,
        ...             before_round_data_depolarization=1e-3,
        ...         ),
        ...         json_metadata={'d': 5},
        ...     ),
        ...     sinter.Task(
        ...         circuit=stim.Circuit.generated(
        ...             'repetition_code:memory',
        ...             distance=7,
        ...             rounds=5,
        ...             before_round_data_depolarization=1e-3,
        ...         ),
        ...         json_metadata={'d': 7},
        ...     ),
        ... ]
        >>> iterator = sinter.iter_collect(
        ...     tasks=tasks,
        ...     decoders=['vacuous'],
        ...     num_workers=2,
        ...     max_shots=100,
        ... )
        >>> total_shots = 0
        >>> for progress in iterator:
        ...     for stat in progress.new_stats:
        ...         total_shots += stat.shots
        >>> print(total_shots)
        200
    """
    if isinstance(decoders, str):
        decoders = [decoders]

    if hint_num_tasks is None:
        try:
            # noinspection PyTypeChecker
            hint_num_tasks = len(tasks)
        except TypeError:
            pass

    with CollectionWorkManager(
            tasks_iter=iter(tasks),
            global_collection_options=CollectionOptions(
                max_shots=max_shots,
                max_errors=max_errors,
                max_batch_seconds=max_batch_seconds,
                start_batch_size=start_batch_size,
                max_batch_size=max_batch_size,
            ),
            decoders=decoders,
            split_errors=split_errors,
            additional_existing_data=additional_existing_data,
            custom_decoders=custom_decoders) as manager:
        try:
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
        except KeyboardInterrupt:
            yield Progress(
                new_stats=(),
                status_message='KeyboardInterrupt',
            )
            raise


def collect(*,
            num_workers: int,
            tasks: Union[Iterator['sinter.Task'], Iterable['sinter.Task']],
            existing_data_filepaths: Iterable[Union[str, pathlib.Path]] = (),
            save_resume_filepath: Union[None, str, pathlib.Path] = None,
            progress_callback: Optional[Callable[['sinter.Progress'], None]] = None,
            max_shots: Optional[int] = None,
            max_errors: Optional[int] = None,
            split_errors: bool = False,
            decoders: Optional[Iterable[str]] = None,
            max_batch_seconds: Optional[int] = None,
            max_batch_size: Optional[int] = None,
            start_batch_size: Optional[int] = None,
            print_progress: bool = False,
            hint_num_tasks: Optional[int] = None,
            custom_decoders: Optional[Dict[str, 'sinter.Decoder']] = None,
            ) -> List['sinter.TaskStats']:
    """Collects statistics from the given tasks, using multiprocessing.

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
        split_errors: Defaults to False. When set to to True, the returned
            TaskStats instances will have a non-None `classified_errors` field
            where keys are bitmasks identifying which observables flipped and
            values are the number of errors where that happened.
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
        custom_decoders: Named child classes of `sinter.decoder`, that can be
            used if requested by name by a task or by the decoders list.
            If not specified, only decoders with support built into sinter, such
            as 'pymatching' and 'fusion_blossom', can be used.

    Returns:
        A list of sample statistics, one from each problem. The list is not in
        any specific order. This is the same data that would have been written
        to a CSV file, but aggregated so that each problem has exactly one
        sample statistic instead of potentially multiple.

    Examples:
        >>> import sinter
        >>> import stim
        >>> tasks = [
        ...     sinter.Task(
        ...         circuit=stim.Circuit.generated(
        ...             'repetition_code:memory',
        ...             distance=5,
        ...             rounds=5,
        ...             before_round_data_depolarization=1e-3,
        ...         ),
        ...         json_metadata={'d': 5},
        ...     ),
        ...     sinter.Task(
        ...         circuit=stim.Circuit.generated(
        ...             'repetition_code:memory',
        ...             distance=7,
        ...             rounds=5,
        ...             before_round_data_depolarization=1e-3,
        ...         ),
        ...         json_metadata={'d': 7},
        ...     ),
        ... ]
        >>> stats = sinter.collect(
        ...     tasks=tasks,
        ...     decoders=['vacuous'],
        ...     num_workers=2,
        ...     max_shots=100,
        ... )
        >>> for stat in sorted(stats, key=lambda e: e.json_metadata['d']):
        ...     print(stat.json_metadata, stat.shots)
        {'d': 5} 100
        {'d': 7} 100
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
            split_errors=split_errors,
            decoders=decoders,
            tasks=tasks,
            hint_num_tasks=hint_num_tasks,
            additional_existing_data=additional_existing_data,
            custom_decoders=custom_decoders,
        ):
            for stats in progress.new_stats:
                result.add_sample(stats)
                if save_resume_file is not None:
                    print(stats.to_csv_line(), file=save_resume_file, flush=True)
            if print_progress:
                progress_printer.show_latest_progress(progress.status_message)
            if progress_callback is not None:
                progress_callback(progress)
        if print_progress:
            progress_printer.flush()
        return list(result.data.values())


def post_selection_mask_from_predicate(
    circuit_or_dem: Union[stim.Circuit, stim.DetectorErrorModel],
    *,
    postselected_detectors_predicate: Callable[[int, Any, Tuple[float, ...]], bool],
    metadata: Any,
) -> np.ndarray:
    num_dets = circuit_or_dem.num_detectors
    post_selection_mask = np.zeros(dtype=np.uint8, shape=math.ceil(num_dets / 8))
    for k, coord in circuit_or_dem.get_detector_coordinates().items():
        if postselected_detectors_predicate(k, metadata, coord):
            post_selection_mask[k // 8] |= 1 << (k % 8)
    return post_selection_mask


def post_selection_mask_from_4th_coord(dem: Union[stim.Circuit, stim.DetectorErrorModel]) -> np.ndarray:
    """Returns a mask that postselects detector's with non-zero 4th coordinate.

    This method is a leftover from before the existence of the command line
    argument `--postselected_detectors_predicate`, when
    `--postselect_detectors_with_non_zero_4th_coord` was the only way to do
    post selection of detectors.

    Args:
        dem: The detector error model to pull coordinate data from.

    Returns:
        A bit packed numpy array where detectors with non-zero 4th coordinate
        data have a True bit at their corresponding index.

    Examples:
        >>> import sinter
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...     detector(1, 2, 3) D0
        ...     detector(1, 1, 1, 1) D1
        ...     detector(1, 1, 1, 0) D2
        ...     detector(1, 1, 1, 999) D80
        ... ''')
        >>> sinter.post_selection_mask_from_4th_coord(dem)
        array([2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], dtype=uint8)
    """
    num_dets = dem.num_detectors
    post_selection_mask = np.zeros(dtype=np.uint8, shape=math.ceil(num_dets / 8))
    for k, coord in dem.get_detector_coordinates().items():
        if len(coord) >= 4 and coord[3]:
            post_selection_mask[k // 8] |= 1 << (k % 8)
    return post_selection_mask
