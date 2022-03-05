import argparse
import contextlib
import hashlib
import sys
from typing import Iterator, Any, Tuple, Optional, List

import stim

from simmer.collection import iter_collect, post_selection_mask_from_last_detector_coords
from simmer.collection_case_tracker import CollectionCaseTracker
from simmer.decoding import CaseStats, DECODER_METHODS, Case
from simmer.main_combine import csv_line, ExistingData, CSV_HEADER


def csv_line_ex(problem: Case, stats: CaseStats) -> str:
    return csv_line(
        shots=stats.num_shots,
        errors=stats.num_errors,
        elapsed=stats.seconds_elapsed,
        discards=stats.num_discards,
        strong_id=problem.strong_id,
        name=problem.name)


def iter_file_path_into_collectors(circuit_paths: Iterator[str],
                                   start_batch_size: int,
                                   max_batch_size: int,
                                   max_errors: int,
                                   max_shots: int,
                                   postselect_last_coord_mins: List[Optional[int]],
                                   decoders: List[str],
                                   existing_data: 'ExistingData') -> Iterator[CollectionCaseTracker]:
    for circuit_path in circuit_paths:
        with open(circuit_path) as f:
            circuit_text = f.read()
        circuit = stim.Circuit(circuit_text)

        for postselect_last_coord_min in postselect_last_coord_mins:
            post_mask = post_selection_mask_from_last_detector_coords(
                circuit=circuit, last_coord_minimum=postselect_last_coord_min)

            for decoder in decoders:
                hash_text = (f'{postselect_last_coord_min=!r}\n'
                             f'{decoder=!r}\n'
                             f'{circuit_text=!r}\n')
                strong_id = hashlib.sha256(hash_text.encode('utf8')).hexdigest()
                name = f'{circuit_path}:{decoder}'
                if postselect_last_coord_min is not None:
                    name += f':post≥{postselect_last_coord_min}'
                finished_stats = existing_data.stats_for(circuit_name=name, circuit_sha256=strong_id)
                yield CollectionCaseTracker(
                    circuit=circuit,
                    name=name,
                    post_mask=post_mask,
                    strong_id=strong_id,
                    start_batch_size=start_batch_size,
                    max_batch_size=max_batch_size,
                    max_errors=max_errors,
                    max_shots=max_shots,
                    decoder=decoder,
                    finished_stats=finished_stats,
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
                        type=int,
                        default=4,
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
                        default=10**2,
                        help='Initial number of samples to batch together into one job.\n'
                             'Starting small prevents over-sampling of circuits above threshold.\n'
                             'The allowed batch size increases exponentially from this starting point.')
    parser.add_argument('-max_batch_size',
                        type=int,
                        default=10**5,
                        help='Maximum number of samples to batch together into one job.\n'
                             'Bigger values increase the delay between jobs finishing.\n'
                             'Smaller values decrease the amount of aggregation of results, increasing the amount of output information.')
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
            raise ValueError(f'{a.postselect_last_coord_min=!r} < -1')
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

        iter_todo = iter_file_path_into_collectors(
            circuit_paths=args.circuits,
            start_batch_size=args.start_batch_size,
            max_batch_size=args.max_batch_size,
            max_errors=args.max_errors,
            max_shots=args.max_shots,
            decoders=args.decoders,
            existing_data=existing_data,
            postselect_last_coord_mins=args.postselect_last_coord_min,
        )
        num_todo = len(args.circuits) * len(args.decoders)

        did_work = False
        for case, stats in iter_collect(num_workers=args.processes,
                                        num_todo=num_todo,
                                        iter_todo=iter_todo,
                                        max_shutdown_wait_seconds=0.5,
                                        print_progress=not args.quiet):
            # Print collected stats.
            if not did_work:
                if sys.stdout in out_files:
                    print(CSV_HEADER, flush=True)
                did_work = True
            stats_line = csv_line_ex(case, stats)
            for f in out_files:
                print(stats_line, flush=True, file=f)

        if not did_work and not args.quiet:
            print("No work to do.", file=sys.stderr)
