import math
import numpy as np
import stim
import tempfile
from typing import Optional

from sinter.decoding import DECODER_METHODS


def predict_on_disk(
    *,
    decoder: str,
    dem_path: str,
    dets_path: str,
    dets_format: str,
    obs_out_path: str,
    obs_out_format: str,
    postselect_detectors_with_non_zero_4th_coord: bool = False,
    discards_out_path: Optional[str] = None,
    discards_out_format: Optional[str] = None,
):

    decode_method = DECODER_METHODS.get(decoder)
    if decode_method is None:
        raise NotImplementedError(f"Unrecognized decoder: {decoder!r}")

    with open(dem_path) as f:
        dem = stim.DetectorErrorModel(f.read())

    num_dets = dem.num_detectors
    num_obs = dem.num_observables

    dets_data = stim.read_shot_data_file(
        path=dets_path,
        format=dets_format,
        bit_pack=True,
        num_detectors=num_dets,
    )

    if discards_out_path is not None:
        if discards_out_format is None:
            raise ValueError('discards_out_path is not None and discards_out_format is None')

        post_selection_mask = np.zeros(dtype=np.uint8, shape=math.ceil(num_dets / 8))
        if postselect_detectors_with_non_zero_4th_coord:
            for k, coord in dem.get_detector_coordinates().items():
                if len(coord) >= 4 and coord[3]:
                    post_selection_mask[k // 8] |= 1 << (k % 8)
        discards = np.any(dets_data & post_selection_mask, axis=1)
        discards.shape = (discards.shape[0], 1)
        stim.write_shot_data_file(
            data=discards,
            path=discards_out_path,
            format=discards_out_format,
            num_detectors=1,
        )
    elif postselect_detectors_with_non_zero_4th_coord:
        raise ValueError('postselect_detectors_with_non_zero_4th_coord and discards_out_path is None')

    with tempfile.TemporaryDirectory() as tmp_dir:
        predictions = decode_method(
            error_model=dem,
            bit_packed_det_samples=dets_data,
            tmp_dir=tmp_dir,
        )

    stim.write_shot_data_file(
        data=predictions,
        path=obs_out_path,
        format=obs_out_format,
        num_observables=num_obs,
    )


def predict_discards_bit_packed(
    *,
    dem: stim.DetectorErrorModel,
    dets_bit_packed: np.ndarray,
    postselect_detectors_with_non_zero_4th_coord: bool,
) -> np.ndarray:
    """Determines which shots to discard due to postselected detectors firing.

    Args:
        dem: The detector error model the detector data applies to.
            This is also where coordinate data is read from, in order to determine
            which detectors to postselect as not having fired.
        dets_bit_packed: A uint8 numpy array with shape (num_shots, math.ceil(num_dets / 8)).
            Contains bit packed detection event data.
        postselect_detectors_with_non_zero_4th_coord: Determines how postselection is done.
            Currently, this is the only option so it has to be set to True.
            Any detector from the detector error model that specifies coordinate data with
            at least four coordinates where the fourth coordinate (coord index 3) is non-zero
            will be postselected.

    Returns:
        A numpy bool8 array with shape (num_shots,) where False means not discarded and
        True means yes discarded.
    """
    if not postselect_detectors_with_non_zero_4th_coord:
        raise ValueError("not postselect_detectors_with_non_zero_4th_coord")
    num_dets = dem.num_detectors
    nb = math.ceil(num_dets / 8)
    if len(dets_bit_packed.shape) != 2:
        raise ValueError(f'len(dets_data_bit_packed.shape={dets_bit_packed.shape}) != 2')
    if dets_bit_packed.shape[1] != nb:
        raise ValueError(f'dets_data_bit_packed.shape[1]={dets_bit_packed.shape[1]} != math.ceil(dem.num_detectors={dem.num_detectors} / 8)')
    if dets_bit_packed.dtype != np.uint8:
        raise ValueError(f'dets_data_bit_packed.dtype={dets_bit_packed.dtype} != np.uint8')

    post_selection_mask = np.zeros(dtype=np.uint8, shape=nb)
    if postselect_detectors_with_non_zero_4th_coord:
        for k, coord in dem.get_detector_coordinates().items():
            if len(coord) >= 4 and coord[3]:
                post_selection_mask[k // 8] |= 1 << (k % 8)
    return np.any(dets_bit_packed & post_selection_mask, axis=1)


def predict_observables_bit_packed(
    *,
    dem: stim.DetectorErrorModel,
    dets_bit_packed: np.ndarray,
    decoder: str,
) -> np.ndarray:
    """Predicts which observables were flipped based on detection event data by using a decoder.

    Args:
        dem: The detector error model the detector data applies to.
            This is also where coordinate data is read from, in order to determine
            which detectors to postselect as not having fired.
        dets_bit_packed: A uint8 numpy array with shape (num_shots, math.ceil(num_dets / 8)).
            Contains bit packed detection event data.
        decoder: The decoder to use for decoding, e.g. "pymatching".

    Returns:
        A numpy uint8 array with shape (num_shots, math.ceil(num_obs / 8)).
        Contains bit packed observable prediction data.
    """

    decode_method = DECODER_METHODS.get(decoder)
    if decode_method is None:
        raise NotImplementedError(f"Unrecognized decoder: {decoder!r}")

    with tempfile.TemporaryDirectory() as tmp_dir:
        predictions = decode_method(
            error_model=dem,
            bit_packed_det_samples=dets_bit_packed,
            tmp_dir=tmp_dir,
        )
    assert predictions.dtype == np.bool8
    return np.packbits(predictions, axis=1, bitorder='little')
