from typing import Any, List

import argparse
import stim
import tempfile

from sinter.decoding import DECODER_METHODS


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(
        description='Predict observable flips from detector data.',
        prog='sinter predict',
    )
    parser.add_argument('-dets',
                        type=str,
                        required=True,
                        help='File to read detection event data from.')
    parser.add_argument('-dets_format',
                        type=str,
                        required=True,
                        help='Format detection event data is stored in.\n'
                             'For example: b8 or 01')
    parser.add_argument('-dem',
                        type=str,
                        required=True,
                        help='File to read detector error model from.')
    parser.add_argument('-decoder',
                        type=str,
                        required=True,
                        help='Decoder to use.')
    parser.add_argument('-out',
                        type=str,
                        required=True,
                        help='Location to write predictions from decoder.')
    parser.add_argument('-out_format',
                        type=str,
                        required=True,
                        help='Format to write predictions in.')
    return parser.parse_args(args)


def main_predict(*, command_line_args: List[str]):
    parsed = parse_args(command_line_args)

    decode_method = DECODER_METHODS.get(parsed.decoder)
    if decode_method is None:
        raise NotImplementedError(f"Unrecognized decoder: {parsed.decoder!r}")

    with open(parsed.dem) as f:
        dem = stim.DetectorErrorModel(f.read())

    dets_data = stim.read_shot_data_file(
        path=parsed.dets,
        format=parsed.dets_format,
        bit_pack=True,
        num_detectors=dem.num_detectors,
    )

    with tempfile.TemporaryDirectory() as tmp_dir:
        predictions = decode_method(
            error_model=dem,
            bit_packed_det_samples=dets_data,
            tmp_dir=tmp_dir,
        )

    stim.write_shot_data_file(
        data=predictions,
        path=parsed.out,
        format=parsed.out_format,
        num_observables=dem.num_observables,
    )
