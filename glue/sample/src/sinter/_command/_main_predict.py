import argparse
from typing import Any
from typing import List

from sinter._predict import predict_on_disk


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(
        description='Predict observable flips from detector data.',
        prog='sinter predict',
    )
    parser.add_argument('--dets',
                        type=str,
                        required=True,
                        help='File to read detection event data from.')
    parser.add_argument('--dets_format',
                        type=str,
                        required=True,
                        help='Format detection event data is stored in.\n'
                             'For example: b8 or 01')
    parser.add_argument('--dem',
                        type=str,
                        required=True,
                        help='File to read detector error model from.')
    parser.add_argument('--decoder',
                        type=str,
                        required=True,
                        help='Decoder to use.')
    parser.add_argument('--obs_out',
                        type=str,
                        required=True,
                        help='Location to write predictions from decoder.')
    parser.add_argument('--obs_out_format',
                        type=str,
                        required=True,
                        help='Format to write predictions in.')

    parser.add_argument('--postselect_detectors_with_non_zero_4th_coord',
                        help='Turns on postselection. '
                             'If any detector with a non-zero 4th coordinate fires, the shot is discarded.',
                        action='store_true')
    parser.add_argument('--discards_out',
                        type=str,
                        default=None,
                        help='Location to write whether each shot should be discarded.'
                             'Specified if and only if --postselect_detectors_with_non_zero_4th_coord.')
    parser.add_argument('--discards_out_format',
                        type=str,
                        default=None,
                        help='Format to write discard data in.'
                             'Specified if and only if --postselect_detectors_with_non_zero_4th_coord.')
    result = parser.parse_args(args)

    if result.postselect_detectors_with_non_zero_4th_coord and result.discards_out is None:
        raise ValueError("Must specify --discards_out to record results of --postselect_detectors_with_non_zero_4th_coord.")
    if result.discards_out is not None and result.discards_out_format is None:
        raise ValueError("Must specify --discards_out_format to specify how to record results of --discards_out.")

    return result


def main_predict(*, command_line_args: List[str]):
    parsed = parse_args(command_line_args)
    predict_on_disk(
        decoder=parsed.decoder,
        dem_path=parsed.dem,
        dets_path=parsed.dets,
        dets_format=parsed.dets_format,
        obs_out_path=parsed.obs_out,
        obs_out_format=parsed.obs_out_format,
        postselect_detectors_with_non_zero_4th_coord=parsed.postselect_detectors_with_non_zero_4th_coord,
        discards_out_path=parsed.discards_out,
        discards_out_format=parsed.discards_out_format,
    )
