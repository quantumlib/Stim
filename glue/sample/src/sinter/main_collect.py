import argparse
import sys
from typing import Iterator, Any, Tuple, Optional, List, Callable

import stim

from sinter.task_stats import TaskStats
from sinter.task import Task
from sinter.collection import collect, post_selection_mask_from_last_detector_coords
from sinter.decoding import DECODER_METHODS
from sinter.main_combine import ExistingData, CSV_HEADER


def iter_file_paths_into_goals(circuit_paths: Iterator[str],
                               metadata_func: Callable,
                               postselect_last_coord_mins: List[Optional[int]],
                               ) -> Iterator[Task]:
    for path in circuit_paths:
        with open(path) as f:
            circuit_text = f.read()
        circuit = stim.Circuit(circuit_text)

        for postselect_last_coord_min in postselect_last_coord_mins:
            post_mask = post_selection_mask_from_last_detector_coords(
                circuit=circuit, last_coord_minimum=postselect_last_coord_min)

            yield Task(
                circuit=circuit,
                postselection_mask=post_mask,
                json_metadata=metadata_func(path=path, circuit=circuit),
            )


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(description='Collect Monte Carlo samples.',
                                     prog='sinter collect')
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
                        default=None,
                        help='Sampling of a circuit will stop if this many shots have been taken.')
    parser.add_argument('-max_errors',
                        type=int,
                        default=None,
                        help='Sampling of a circuit will stop if this many errors have been seen.')
    parser.add_argument('-processes',
                        required=True,
                        type=int,
                        help='Number of processes to use for simultaneous sampling and decoding.')
    parser.add_argument('-save_resume_filepath',
                        type=str,
                        default=None,
                        help='Activates MERGE mode.\n'
                             "If save_resume_filepath doesn't exist, initializes it with a CSV header.\n"
                             'CSV data already at save_resume_filepath counts towards max_shots and max_errors.\n'
                             'Collected data is appended to save_resume_filepath.\n'
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
    parser.add_argument('-existing_data_filepaths',
                        nargs='*',
                        type=str,
                        default=(),
                        help='CSV data from these files counts towards max_shots and max_errors.\n'
                             'This parameter can be given multiple arguments.')
    parser.add_argument('-metadata_func',
                        type=str,
                        default="{'path': path}",
                        help='A python expression that determines whether a case is kept or not.\n'
                             'Available values:\n'
                             '    path: Relative path to the circuit file, from the command line arguments.\n'
                             '    circuit: The circuit itself, parsed from the file, as a stim.Circuit.\n'
                             'Expected type:\n'
                             '    A value that can be serialized into JSON, like a Dict[str, int].\n'
                             '\n'
                             '    Note that the decoder field is already recorded separately, so storing\n'
                             '    it in the metadata as well would be redundant. But something like\n'
                             '    decoder version could be usefully added.\n'
                             'Examples:\n'
                             '''    -metadata_func "{'path': path}"\n'''
                             '''    -metadata_func "{'n': circuit.num_qubits, 'p': float(path.split('/')[-1].split('.')[0])}"\n'''
                        )
    a = parser.parse_args(args=args)
    for e in a.postselect_last_coord_min:
        if e is not None and e < -1:
            raise ValueError(f'a.postselect_last_coord_min={a.postselect_last_coord_min!r} < -1')
    a.postselect_last_coord_min = [None if e == -1 else e for e in
                                                       a.postselect_last_coord_min]

    a.metadata_func = eval(compile(
        'lambda *, path, circuit: ' + a.metadata_func,
        filename='metadata_func:command_line_arg',
        mode='eval'))
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
    args = parse_args(args=command_line_args)

    iter_tasks = iter_file_paths_into_goals(
        circuit_paths=args.circuits,
        metadata_func=args.metadata_func,
        postselect_last_coord_mins=args.postselect_last_coord_min,
    )
    num_tasks = len(args.circuits) * len(args.decoders)

    print_to_stdout = args.save_resume_filepath is not None or not args.quiet

    did_work = False

    def on_progress(sample: TaskStats) -> None:
        nonlocal did_work
        if print_to_stdout:
            if not did_work:
                print(CSV_HEADER, flush=True)
            print(sample.to_csv_line(), flush=True)
        did_work = True

    collect(
        num_workers=args.processes,
        hint_num_tasks=num_tasks,
        tasks=iter_tasks,
        print_progress=not args.quiet,
        save_resume_filepath=args.save_resume_filepath,
        existing_data_filepaths=args.existing_data_filepaths,
        progress_callback=on_progress,
        max_errors=args.max_errors,
        max_shots=args.max_shots,
        decoders=args.decoders,
        max_batch_seconds=args.max_batch_seconds,
        max_batch_size=args.max_batch_size,
        start_batch_size=args.start_batch_size,
    )

    if not did_work and not args.quiet:
        print("No work to do.", file=sys.stderr)
