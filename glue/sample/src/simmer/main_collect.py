import argparse
import contextlib
import sys
from typing import Iterator, Any, Tuple, Optional, List

import stim

from simmer import Task
from simmer.collection import iter_collect, post_selection_mask_from_last_detector_coords
from simmer.decoding import DECODER_METHODS
from simmer.main_combine import ExistingData, CSV_HEADER


def iter_file_paths_into_goals(circuit_paths: Iterator[str],
                               max_errors: int,
                               max_shots: int,
                               max_batch_size: Optional[int],
                               start_batch_size: int,
                               max_batch_seconds: Optional[float],
                               postselect_last_coord_mins: List[Optional[int]],
                               decoders: List[str],
                               existing_data: 'ExistingData',
                               ) -> Iterator[Task]:
    for circuit_path in circuit_paths:
        with open(circuit_path) as f:
            circuit_text = f.read()
        circuit = stim.Circuit(circuit_text)

        for postselect_last_coord_min in postselect_last_coord_mins:
            post_mask = post_selection_mask_from_last_detector_coords(
                circuit=circuit, last_coord_minimum=postselect_last_coord_min)

            for decoder in decoders:
                yield Task(
                    circuit=circuit,
                    decoder=decoder,
                    postselection_mask=post_mask,
                    json_metadata={
                        'path': circuit_path,
                    },
                    max_errors=max_errors,
                    max_shots=max_shots,
                    previous_stats=existing_data,
                    max_batch_size=max_batch_size,
                    max_batch_seconds=max_batch_seconds,
                    start_batch_size=start_batch_size,
                )


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(description='Collect Monte Carlo samples.',
                                     prog='simmer collect')
    parser.add_argument('-circuits',
                        nargs='+',
                        required=True,
                        help='Circuit files to sample from and decode.\n'
                             'This parameter can be given multiple arguments.')
    parser.add_argument('-decoders',
                        choices=sorted(DECODER_METHODS.keys()),
                        nargs='+',
                        required=True,
                        help='How to combine new results with old results.')
    parser.add_argument('-max_shots',
                        type=int,
                        required=True,
                        help='Sampling of a circuit will stop if this many shots have been taken.')
    parser.add_argument('-max_errors',
                        type=int,
                        required=True,
                        help='Sampling of a circuit will stop if this many errors have been seen.')
    parser.add_argument('-processes',
                        required=True,
                        type=int,
                        help='Number of processes to use for simultaneous sampling and decoding.')
    parser.add_argument('-merge_data_location',
                        type=str,
                        default=None,
                        help='Activates MERGE mode.\n'
                             "If merge_data_location doesn't exist, initializes it with a CSV header.\n"
                             'CSV data already at merge_data_location counts towards max_shots and max_errors.\n'
                             'Collected data is appended to merge_data_location.\n'
                             'Note that MERGE mode is tolerant to failures: if the process is killed, it can simply be restarted and it will pick up where it left off.\n'
                             'Note that MERGE mode is idempotent: if sufficient data has been collected, no additional work is done when run again.')

    parser.add_argument('-start_batch_size',
                        type=int,
                        default=100,
                        help='Initial number of samples to batch together into one job.\n'
                             'Starting small prevents over-sampling of circuits above threshold.\n'
                             'The allowed batch size increases exponentially from this starting point.')
    parser.add_argument('-max_batch_size',
                        type=int,
                        default=None,
                        help='Maximum number of samples to batch together into one job.\n'
                             'Bigger values increase the delay between jobs finishing.\n'
                             'Smaller values decrease the amount of aggregation of results, increasing the amount of output information.')
    parser.add_argument('-max_batch_seconds',
                        type=int,
                        default=None,
                        help='Limits number of shots in a batch so that the estimated runtime of the batch is below this amount.')
    parser.add_argument('-postselect_last_coord_min',
                        type=int,
                        nargs='+',
                        help='Activates postselection of results. '
                             'Whenever a detecor with a last coordinate at least as large as the given '
                             'value creates a detection event, the shot is discarded.',
                        default=(None,))
    parser.add_argument('-quiet',
                        help='Disables writing progress to stderr.',
                        action='store_true')
    parser.add_argument('-existing_data_location',
                        nargs='*',
                        type=str,
                        default=(),
                        help='CSV data from these files counts towards max_shots and max_errors.\n'
                             'This parameter can be given multiple arguments.')

    a = parser.parse_args(args=args)
    for e in a.postselect_last_coord_min:
        if e is not None and e < -1:
            raise ValueError(f'a.postselect_last_coord_min={a.postselect_last_coord_min!r} < -1')
    a.postselect_last_coord_min = [None if e == -1 else e for e in
                                                       a.postselect_last_coord_min]
    if a.merge_data_location in a.existing_data_location:
        raise ValueError("Double counted data. merge_data_location in existing_data_location")

    return a


def open_merge_file(path: str) -> Tuple[Any, ExistingData]:
    try:
        existing = ExistingData.from_file(path)
        return open(path, 'a'), existing
    except FileNotFoundError:
        f = open(path, 'w')
        print(CSV_HEADER, file=f)
        return f, ExistingData()


def main_collect(*, command_line_args: List[str]):
    with contextlib.ExitStack() as ctx:
        args = parse_args(args=command_line_args)

        # Read existing data.
        existing_data = ExistingData()
        for existing_path in args.existing_data_location:
            existing_data += ExistingData.from_file(existing_path)

        # Configure merging with already recorded data at storage location.
        out_files = []
        if args.merge_data_location is not None:
            out_file, old_merge_data = open_merge_file(args.merge_data_location)
            ctx.enter_context(out_file)
            existing_data += old_merge_data
            if not args.quiet:
                out_files.append(sys.stdout)
            out_files.append(out_file)
        else:
            out_files.append(sys.stdout)

        iter_todo = iter_file_paths_into_goals(
            circuit_paths=args.circuits,
            max_errors=args.max_errors,
            max_shots=args.max_shots,
            decoders=args.decoders,
            postselect_last_coord_mins=args.postselect_last_coord_min,
            existing_data=existing_data,
            max_batch_seconds=args.max_batch_seconds,
            max_batch_size=args.max_batch_size,
            start_batch_size=args.start_batch_size,
        )
        num_todo = len(args.circuits) * len(args.decoders)

        did_work = False
        for sample in iter_collect(
                num_workers=args.processes,
                hint_num_tasks=num_todo,
                tasks=iter_todo,
                print_progress=not args.quiet,
        ):
            # Print collected stats.
            if not did_work:
                if sys.stdout in out_files:
                    print(CSV_HEADER, flush=True)
                did_work = True
            stats_line = sample.to_csv_line()
            for f in out_files:
                print(stats_line, flush=True, file=f)

        if not did_work and not args.quiet:
            print("No work to do.", file=sys.stderr)
