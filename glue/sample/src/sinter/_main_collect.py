import argparse
import math
import sys
from typing import Iterator, Any, Tuple, List, Callable, FrozenSet, Optional
from typing import cast

import numpy as np
import stim

import sinter
from sinter._printer import ThrottledProgressPrinter
from sinter._task import Task
from sinter._collection import collect, Progress, post_selection_mask_from_predicate
from sinter._decoding_all_built_in_decoders import BUILT_IN_DECODERS
from sinter._main_combine import ExistingData, CSV_HEADER


def iter_file_paths_into_goals(circuit_paths: Iterator[str],
                               metadata_func: Callable,
                               postselected_detectors_predicate: Optional[Callable[[int, Any, Tuple[float, ...]], bool]],
                               postselected_observables_predicate: Callable[[int, Any], bool],
                               ) -> Iterator[Task]:
    for path in circuit_paths:
        with open(path) as f:
            circuit_text = f.read()
        circuit = stim.Circuit(circuit_text)

        metadata = metadata_func(path=path, circuit=circuit)
        if postselected_detectors_predicate is not None:
            post_mask = post_selection_mask_from_predicate(circuit, metadata=metadata, postselected_detectors_predicate=postselected_detectors_predicate)
            if not np.any(post_mask):
                post_mask = None
        else:
            post_mask = None
        postselected_observables = [
            k
            for k in range(circuit.num_observables)
            if postselected_observables_predicate(k, metadata)
        ]
        if any(postselected_observables):
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
                        type=str,
                        nargs='+',
                        required=True,
                        help='The decoder to use to predict observables from detection events.')
    parser.add_argument('--custom_decoders_module_function',
                        default=None,
                        help='Use the syntax "module:function" to "import function from module" '
                             'and use the result of "function()" as the custom_decoders '
                             'dictionary. The dictionary must map strings to stim.Decoder '
                             'instances.')
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
    parser.add_argument('--postselected_detectors_predicate',
                        type=str,
                        default='''False''',
                        help='Specifies a predicate used to decide which detectors to postselect. '
                             'When a postselected detector produces a detection event, the shot is discarded instead of being given to the decoder.'
                             'The number of discarded shots is tracked as a statistic.'
                             'Available values:\n'
                             '    index: The unique number identifying the detector, determined by the order of detectors in the circuit file.\n'
                             '    coords: The coordinate data associated with the detector. An empty tuple, if the circuit file did not specify detector coordinates.\n'
                             '    metadata: The metadata associated with the task being sampled.\n'
                             'Expected expression type:\n'
                             '    Something that can be given to `bool` to get False (do not postselect) or True (yes postselect).\n'
                             'Examples:\n'
                             '''    --postselected_detectors_predicate "coords[2] == 0"\n'''
                             '''    --postselected_observables_predicate "coords[3] < metadata['postselection_level']"\n''')
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
                        help='A python expression that associates json metadata with a circuit\'s results.\n'
                             'Set to "auto" to use "sinter.comma_separated_key_values(path)"\n'
                             'Values available to the expression:\n'
                             '    path: Relative path to the circuit file, from the command line arguments.\n'
                             '    circuit: The circuit itself, parsed from the file, as a stim.Circuit.\n'
                             'Expected type:\n'
                             '    A value that can be serialized into JSON, like a Dict[str, int].\n'
                             '\n'
                             '    Note that the decoder field is already recorded separately, so storing\n'
                             '    it in the metadata as well would be redundant. But something like\n'
                             '    decoder version could be usefully added.\n'
                             'Examples:\n'
                             '''    --metadata_func "{'path': path}"\n'''
                             '''    --metadata_func "auto"\n'''
                             '''    --metadata_func "{'n': circuit.num_qubits, 'p': float(path.split('/')[-1].split('.')[0])}"\n'''
                        )
    a = parser.parse_args(args=args)
    if a.metadata_func == 'auto':
        a.metadata_func = "sinter.comma_separated_key_values(path)"
    a.metadata_func = eval(compile(
        'lambda *, path, circuit: ' + a.metadata_func,
        filename='metadata_func:command_line_arg',
        mode='eval'), {'sinter': sinter})
    a.postselected_observables_predicate = eval(compile(
        'lambda index, metadata: ' + a.postselected_observables_predicate,
        filename='postselected_observables_predicate:command_line_arg',
        mode='eval'))
    if a.postselected_detectors_predicate == 'False':
        if a.postselect_detectors_with_non_zero_4th_coord:
            a.postselected_detectors_predicate = lambda index, metadata, coords: coords[3]
        else:
            a.postselected_detectors_predicate = None
    else:
        if a.postselect_detectors_with_non_zero_4th_coord:
            raise ValueError("Can't specify both --postselect_detectors_with_non_zero_4th_coord and --postselected_detectors_predicate")
        a.postselected_detectors_predicate = eval(compile(
            'lambda index, metadata, coords: ' + cast(str, a.postselected_detectors_predicate),
            filename='postselected_detectors_predicate:command_line_arg',
            mode='eval'))
    if a.custom_decoders_module_function is not None:
        terms = a.custom_decoders_module_function.split(':')
        if len(terms) != 2:
            raise ValueError("--custom_decoders_module_function didn't have exactly one colon "
                             "separating a module name from a function name. Expected an argument "
                             "of the form --custom_decoders_module_function 'module:function'")
        module, function = terms
        vals = {'__name__': '[]'}
        exec(f"from {module} import {function} as _custom_decoders", vals)
        a.custom_decoders = vals['_custom_decoders']()
    else:
        a.custom_decoders = None
    for decoder in a.decoders:
        if decoder not in BUILT_IN_DECODERS and (a.custom_decoders is None or decoder not in a.custom_decoders):
            message = f"Not a recognized decoder: {decoder=}.\n"
            message += f"Available built-in decoders: {sorted(e for e in BUILT_IN_DECODERS.keys() if 'internal' not in e)}.\n"
            if a.custom_decoders is None:
                message += f"No custom decoders are available. --custom_decoders_module_function wasn't specified."
            else:
                message += f"Available custom decoders: {sorted(a.custom_decoders.keys())}."
            raise ValueError(message)

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
        postselected_detectors_predicate=args.postselected_detectors_predicate,
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

        msg = sample.status_message
        if msg == 'KeyboardInterrupt':
            msg = '\nInterrupted. Output is flushed. Cleaning up workers...'
        printer.show_latest_progress(msg)
        printer.flush()

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
            custom_decoders=args.custom_decoders,
        )
    except KeyboardInterrupt:
        pass
