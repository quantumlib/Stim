import os.path
import pathlib

import math
import numpy as np
import stim
import tempfile
from typing import Optional, Union

from sinter import post_selection_mask_from_4th_coord
from sinter._decoding import DECODER_METHODS, streaming_post_select


def _converted_on_disk(
        in_path: pathlib.Path,
        out_path: pathlib.Path,
        num_dets: int,
        num_obs: int,
        in_format: str,
        out_format: str) -> pathlib.Path:
    if in_format == out_format:
        return in_path
    raw = stim.read_shot_data_file(
        path=str(in_path),
        format=in_format,
        bit_pack=True,
        num_detectors=num_dets,
        num_observables=num_obs,
    )
    stim.write_shot_data_file(
        data=raw,
        path=str(out_path),
        format=out_format,
        num_detectors=num_dets,
        num_observables=num_obs,
    )
    return out_path


def predict_on_disk(
    *,
    decoder: str,
    dem_path: Union[str, pathlib.Path],
    dets_path: Union[str, pathlib.Path],
    dets_format: str,
    obs_out_path: Union[str, pathlib.Path],
    obs_out_format: str,
    postselect_detectors_with_non_zero_4th_coord: bool = False,
    discards_out_path: Optional[Union[str, pathlib.Path]] = None,
    discards_out_format: Optional[str] = None,
) -> None:
    """Performs decoding and postselection on disk.

    Args:
        decoder: The decoder to use for decoding.
        dem_path: The detector error model to use to configure the decoder.
        dets_path: Where the detection event data is stored on disk.
        dets_format: The format the detection event data is stored in (e.g. '01' or 'b8').
        obs_out_path: Where to write predicted observable flip data on disk.
            Note that the predicted observable flip data will not included data from shots discarded by postselection.
            Use the data in discards_out_path to determine which shots were discarded.
        obs_out_format: The format to write the observable flip data in (e.g. '01' or 'b8').
        postselect_detectors_with_non_zero_4th_coord: Activates postselection. Detectors that have a non-zero 4th
            coordinate will be postselected. Any shot where a postselected detector fires will be discarded.
            Requires specifying discards_out_path, for indicating which shots were discarded.
        discards_out_path: Only used if postselection is being used. Where to write discard data on disk.
        discards_out_format: The format to write discard data in (e.g. '01' or 'b8').
    """
    if (discards_out_path is not None) != (discards_out_format is not None):
        raise ValueError('(discards_out_path is not None) != (discards_out_format is not None)')
    if (discards_out_path is not None) != postselect_detectors_with_non_zero_4th_coord:
        raise ValueError('(discards_out_path is not None) != postselect_detectors_with_non_zero_4th_coord')

    dem_path = pathlib.Path(dem_path)
    dets_path = pathlib.Path(dets_path)
    obs_out_path = pathlib.Path(obs_out_path)
    if discards_out_path is not None:
        discards_out_path = pathlib.Path(discards_out_path)

    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_dir = pathlib.Path(tmp_dir)
        decode_method = DECODER_METHODS.get(decoder)
        if decode_method is None:
            raise NotImplementedError(f"Unrecognized decoder: {decoder!r}")

        with open(dem_path) as f:
            dem = stim.DetectorErrorModel(f.read())

        num_dets = dem.num_detectors
        num_det_bytes = math.ceil(num_dets / 8)
        num_obs = dem.num_observables

        dets_b8_path = _converted_on_disk(
            in_path=dets_path,
            out_path=tmp_dir / 'sinter_dets.b8',
            in_format=dets_format,
            out_format='b8',
            num_dets=num_dets,
            num_obs=0)
        if num_det_bytes == 0:
            raise NotImplementedError("Don't know how many shots there are, because num_det_bytes=0.")
        num_shots = os.path.getsize(dets_b8_path) // num_det_bytes

        if discards_out_path is not None:
            if discards_out_format == 'b8':
                discards_b8_path = discards_out_path
            else:
                discards_b8_path = tmp_dir / 'sinter_discards.b8'
            post_selection_mask = np.zeros(dtype=np.uint8, shape=math.ceil(num_dets / 8))
            if postselect_detectors_with_non_zero_4th_coord:
                post_selection_mask = post_selection_mask_from_4th_coord(dem)
            kept_dets_b8_path = tmp_dir / 'sinter_dets.kept.b8'
            num_discards = streaming_post_select(
                num_shots=num_shots,
                num_dets=num_dets,
                num_obs=num_obs,
                dets_in_b8=dets_b8_path,
                obs_in_b8=None,
                obs_out_b8=None,
                discards_out_b8=discards_b8_path,
                dets_out_b8=kept_dets_b8_path,
                post_mask=post_selection_mask,
            )
            assert discards_out_format is not None
            _converted_on_disk(
                in_path=discards_b8_path,
                out_path=discards_out_path,
                out_format=discards_out_format,
                in_format='b8',
                num_dets=1,
                num_obs=0,
            )
            num_kept_shots = num_shots - num_discards
        else:
            kept_dets_b8_path = dets_b8_path
            num_kept_shots = num_shots
            if postselect_detectors_with_non_zero_4th_coord:
                raise ValueError('postselect_detectors_with_non_zero_4th_coord and discards_out_path is None')

        if obs_out_format != 'b8':
            obs_inter = tmp_dir / 'sinter_obs_inter.b8'
        else:
            obs_inter = obs_out_path
        decode_method(
            num_shots=num_kept_shots,
            num_dets=num_dets,
            num_obs=num_obs,
            error_model=dem,
            dets_b8_in_path=kept_dets_b8_path,
            obs_predictions_b8_out_path=obs_inter,
            tmp_dir=tmp_dir,
        )
        _converted_on_disk(
            in_path=obs_inter,
            out_path=obs_out_path,
            out_format=obs_out_format,
            in_format='b8',
            num_dets=0,
            num_obs=num_obs,
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


def predict_observables(
    *,
    dem: stim.DetectorErrorModel,
    dets: np.ndarray,
    decoder: str,
    bit_pack_result: bool = False,
) -> np.ndarray:
    """Predicts which observables were flipped based on detection event data by using a decoder.

    Args:
        dem: The detector error model the detector data applies to.
            This is also where coordinate data is read from, in order to determine
            which detectors to postselect as not having fired.
        dets: The detection event data. Can be bit packed or not bit packed.
            If dtype=np.bool8 then shape=(num_shots, num_detectors).
            If dtype=np.uint8 then shape=(num_shots, math.ceil(num_detectors / 8)).
        decoder: The decoder to use for decoding, e.g. "pymatching".
        bit_pack_result: Defaults to False. Determines if the result is bit packed
            or not.

    Returns:
        If bit_packed_result=False (default):
            dtype=np.bool8
            shape=(num_shots, num_observables)
        If bit_packed_result=True:
            dtype=np.uint8
            shape=(num_shots, math.ceil(num_observables / 8))
    """

    if dets.dtype == np.bool8:
        dets = np.packbits(dets, axis=1, bitorder='little')
    result = predict_observables_bit_packed(dem=dem, dets_bit_packed=dets, decoder=decoder)
    if not bit_pack_result:
        result = np.unpackbits(result, axis=1, bitorder='little', count=dem.num_observables)
    return result


def predict_observables_bit_packed(
    *,
    dem: stim.DetectorErrorModel,
    dets_bit_packed: np.ndarray,
    decoder: str,
) -> np.ndarray:
    """Predicts which observables were flipped based on detection event data by using a decoder.

    This is a specialization of `sinter.predict_observables`, without optional bit packing.

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
        tmp_dir = pathlib.Path(tmp_dir)
        dets_b8_path = tmp_dir / 'sinter_dets.b8'
        pred_b8_path = tmp_dir / 'sinter_predictions.b8'
        num_dets = dem.num_detectors
        num_obs = dem.num_observables

        stim.write_shot_data_file(
            data=dets_bit_packed,
            path=str(dets_b8_path),
            format='b8',
            num_detectors=num_dets,
            num_observables=0,
        )

        decode_method(
            num_shots=dets_bit_packed.shape[0],
            num_dets=num_dets,
            num_obs=num_obs,
            error_model=dem,
            dets_b8_in_path=dets_b8_path,
            obs_predictions_b8_out_path=pred_b8_path,
            tmp_dir=tmp_dir,
        )

        return stim.read_shot_data_file(
            path=str(pred_b8_path),
            format='b8',
            bit_pack=True,
            num_detectors=0,
            num_observables=num_obs,
        )
