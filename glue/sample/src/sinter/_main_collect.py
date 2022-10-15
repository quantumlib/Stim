import argparse
import math
import sys
from typing import Iterator, Any, Tuple, List, Callable, FrozenSet, Optional

import numpy as np
import stim

import sinter
from sinter._printer import ThrottledProgressPrinter
from sinter._task import Task
from sinter._collection import collect, Progress, post_selection_mask_from_4th_coord
from sinter._decoding import DECODER_METHODS
from sinter._main_combine import ExistingData, CSV_HEADER


def iter_file_paths_into_goals(circuit_paths: Iterator[str],
                               metadata_func: Callable,
                               postselect_4th_coord: bool,
                               postselected_observables_predicate: Callable[[int, Any], FrozenSet[int]],
                               ) -> Iterator[Task]:
    for path in circuit_paths:
        with open(path) as f:
            circuit_text = f.read()
        circuit = stim.Circuit(circuit_text)

        if postselect_4th_coord:
            post_mask = post_selection_mask_from_4th_coord(circuit)
        else:
            post_mask = None
        metadata = metadata_func(path=path, circuit=circuit)
        postselected_observables = [
            k
            for k in range(circuit.num_observables)
            if postselected_observables_predicate(k, metadata)
        ]
        if postselected_observables:
            postselected_observables_mask = np.zeros(shape=math.ceil(circuit.num_observables / 8), dtype=np.uint8)
            for k in postselected_observables:
                postselected_observables_mask[k // 8] |= 1 << (k % 8)
        else:
            postselected_observables_mask = None

        yield Task(
            circuit=circuit,
            postselection_mask=post_mask,
            postselected_observables_mask=postselected_observables_mask,
            json_metadata=metadata,
        )


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(description='Collect Monte Carlo samples.',
                                     prog='sinter collect')
    parser.add_argument('--circuits',
                        nargs='+',
                        required=True,
                        help='Circuit files to sample from and decode.\n'
                             'This parameter can be given multiple arguments.')
    parser.add_argument('--decoders',
                        choices=sorted(DECODER_METHODS.keys()),
                        nargs='+',
                        required=True,
                        help='How to combine new results with old results.')
    parser.add_argument('--max_shots',
                        type=int,
                        default=None,
                        help='Sampling of a circuit will stop if this many shots have been taken.')
    parser.add_argument('--max_errors',
                        type=int,
                        default=None,
                        help='Sampling of a circuit will stop if this many errors have been seen.')
    parser.add_argument('--processes',
                        required=True,
                        type=int,
                        help='Number of processes to use for simultaneous sampling and decoding.')
    parser.add_argument('--save_resume_filepath',
                        type=str,
                        default=None,
                        help='Activates MERGE mode.\n'
                             "If save_resume_filepath doesn't exist, initializes it with a CSV header.\n"
                             'CSV data already at save_resume_filepath counts towards max_shots and max_errors.\n'
                             'Collected data is appended to save_resume_filepath.\n'
                             'Note that MERGE mode is tolerant to failures: if the process is killed, it can simply be restarted and it will pick up where it left off.\n'
                             'Note that MERGE mode is idempotent: if sufficient data has been collected, no additional work is done when run again.')

    parser.add_argument('--start_batch_size',
                        type=int,
                        default=100,
                        help='Initial number of samples to batch together into one job.\n'
                             'Starting small prevents over-sampling of circuits above threshold.\n'
                             'The allowed batch size increases exponentially from this starting point.')
    parser.add_argument('--max_batch_size',
                        type=int,
                        default=None,
                        help='Maximum number of samples to batch together into one job.\n'
                             'Bigger values increase the delay between jobs finishing.\n'
                             'Smaller values decrease the amount of aggregation of results, increasing the amount of output information.')
    parser.add_argument('--max_batch_seconds',
                        type=int,
                        default=None,
                        help='Limits number of shots in a batch so that the estimated runtime of the batch is below this amount.')
    parser.add_argument('--postselect_detectors_with_non_zero_4th_coord',
                        help='Turns on detector postselection. '
                             'If any detector with a non-zero 4th coordinate fires, the shot is discarded.',
                        action='store_true')
    parser.add_argument('--postselected_observables_predicate',
                        type=str,
                        default='''False''',
                        help='Specifies a predicate used to decide which observables to postselect. '
                             'When a decoder mispredicts a postselected observable, the shot is discarded instead of counting as an error.'
                             'Available values:\n'
                             '    index: The index of the observable to postselect or not.\n'
                             '    metadata: The metadata associated with the task.\n'
                             'Expected expression type:\n'
                             '    Something that can be given to `bool` to get False (do not postselect) or True (yes postselect).\n'
                             'Examples:\n'
                             '''    --postselected_observables_predicate "False"\n'''
                             '''    --postselected_observables_predicate "metadata['d'] == 5 and index >= 2"\n''')
    parser.add_argument('--quiet',
                        help='Disables writing progress to stderr.',
                        action='store_true')
    parser.add_argument('--also_print_results_to_stdout',
                        help='Even if writing to a file, also write results to stdout.',
                        action='store_true')
    parser.add_argument('--existing_data_filepaths',
                        nargs='*',
                        type=str,
                        default=(),
                        help='CSV data from these files counts towards max_shots and max_errors.\n'
                             'This parameter can be given multiple arguments.')
    parser.add_argument('--metadata_func',
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
    a.metadata_func = eval(compile(
        'lambda *, path, circuit: ' + a.metadata_func,
        filename='metadata_func:command_line_arg',
        mode='eval'), {'sinter': sinter})
    a.postselected_observables_predicate = eval(compile(
        'lambda index, metadata: ' + a.postselected_observables_predicate,
        filename='postselected_observables_predicate:command_line_arg',
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
        postselect_4th_coord=args.postselect_detectors_with_non_zero_4th_coord,
        postselected_observables_predicate=args.postselected_observables_predicate,
    )
    num_tasks = len(args.circuits) * len(args.decoders)

    print_to_stdout = args.also_print_results_to_stdout or args.save_resume_filepath is None

    did_work = False
    printer = ThrottledProgressPrinter(
        outs=[],
        print_progress=not args.quiet,
        min_progress_delay=0.03 if args.also_print_results_to_stdout else 1,
    )
    if print_to_stdout:
        printer.outs.append(sys.stdout)

    def on_progress(sample: Progress) -> None:
        nonlocal did_work
        for stats in sample.new_stats:
            if not did_work:
                printer.print_out(CSV_HEADER)
                did_work = True
            printer.print_out(stats.to_csv_line())

        printer.show_latest_progress(sample.status_message)

    try:
        collect(
            num_workers=args.processes,
            hint_num_tasks=num_tasks,
            tasks=iter_tasks,
            print_progress=False,
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
    except KeyboardInterrupt:
        printer.show_latest_progress('')
        print("\033[33m\nInterrupted\033[0m")
