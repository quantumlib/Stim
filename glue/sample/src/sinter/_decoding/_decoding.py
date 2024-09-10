import collections
from typing import Iterable
from typing import Optional, Dict, Tuple, TYPE_CHECKING, Union

import contextlib
import pathlib
import tempfile
import math
import time

import numpy as np
import stim

from sinter._data import AnonTaskStats
from sinter._decoding._decoding_all_built_in_decoders import BUILT_IN_DECODERS
from sinter._decoding._decoding_decoder_class import CompiledDecoder, Decoder

if TYPE_CHECKING:
    import sinter


def streaming_post_select(*,
                          num_dets: int,
                          num_obs: int,
                          dets_in_b8: pathlib.Path,
                          obs_in_b8: Optional[pathlib.Path],
                          dets_out_b8: pathlib.Path,
                          obs_out_b8: Optional[pathlib.Path],
                          discards_out_b8: Optional[pathlib.Path],
                          num_shots: int,
                          post_mask: np.ndarray) -> int:
    if post_mask.shape != ((num_dets + 7) // 8,):
        raise ValueError(f"post_mask.shape={post_mask.shape} != (math.ceil(num_detectors / 8),)")
    if post_mask.dtype != np.uint8:
        raise ValueError(f"post_mask.dtype={post_mask.dtype} != np.uint8")
    assert (obs_in_b8 is None) == (obs_out_b8 is None)

    num_det_bytes = math.ceil(num_dets / 8)
    num_obs_bytes = math.ceil(num_obs / 8)
    num_shots_left = num_shots
    num_discards = 0

    with contextlib.ExitStack() as ctx:
        dets_in_f = ctx.enter_context(open(dets_in_b8, 'rb'))
        dets_out_f = ctx.enter_context(open(dets_out_b8, 'wb'))
        if obs_in_b8 is not None and obs_out_b8 is not None:
            obs_in_f = ctx.enter_context(open(obs_in_b8, 'rb'))
            obs_out_f = ctx.enter_context(open(obs_out_b8, 'wb'))
        else:
            obs_in_f = None
            obs_out_f = None
        if discards_out_b8 is not None:
            discards_out_f = ctx.enter_context(open(discards_out_b8, 'wb'))
        else:
            discards_out_f = None

        while num_shots_left:
            batch_size = min(num_shots_left, math.ceil(10 ** 6 / max(1, num_dets)))

            det_batch = np.fromfile(dets_in_f, dtype=np.uint8, count=num_det_bytes * batch_size)
            det_batch.shape = (batch_size, num_det_bytes)
            discarded = np.any(det_batch & post_mask, axis=1)
            det_left = det_batch[~discarded, :]
            det_left.tofile(dets_out_f)

            if obs_in_f is not None and obs_out_f is not None:
                obs_batch = np.fromfile(obs_in_f, dtype=np.uint8, count=num_obs_bytes * batch_size)
                obs_batch.shape = (batch_size, num_obs_bytes)
                obs_left = obs_batch[~discarded, :]
                obs_left.tofile(obs_out_f)
            if discards_out_f is not None:
                discarded.tofile(discards_out_f)

            num_discards += np.count_nonzero(discarded)
            num_shots_left -= batch_size

    return num_discards


def _streaming_count_mistakes(
        *,
        num_shots: int,
        num_obs: int,
        num_det: int,
        postselected_observable_mask: Optional[np.ndarray] = None,
        dets_in: pathlib.Path,
        obs_in: pathlib.Path,
        predictions_in: pathlib.Path,
        count_detection_events: bool,
        count_observable_error_combos: bool,
) -> Tuple[int, int, collections.Counter]:

    num_det_bytes = math.ceil(num_det / 8)
    num_obs_bytes = math.ceil(num_obs / 8)
    num_errors = 0
    num_discards = 0
    custom_counts = collections.Counter()
    if count_detection_events:
        with open(dets_in, 'rb') as dets_in_f:
            num_shots_left = num_shots
            while num_shots_left:
                batch_size = min(num_shots_left, math.ceil(10**6 / max(num_obs, 1)))
                det_data = np.fromfile(dets_in_f, dtype=np.uint8, count=num_det_bytes * batch_size)
                for b in range(8):
                    custom_counts['detection_events'] += np.count_nonzero(det_data & (1 << b))
                num_shots_left -= batch_size
        custom_counts['detectors_checked'] += num_shots * num_det

    with open(obs_in, 'rb') as obs_in_f:
        with open(predictions_in, 'rb') as predictions_in_f:
            num_shots_left = num_shots
            while num_shots_left:
                batch_size = min(num_shots_left, math.ceil(10**6 / max(num_obs, 1)))

                obs_batch = np.fromfile(obs_in_f, dtype=np.uint8, count=num_obs_bytes * batch_size)
                pred_batch = np.fromfile(predictions_in_f, dtype=np.uint8, count=num_obs_bytes * batch_size)
                obs_batch.shape = (batch_size, num_obs_bytes)
                pred_batch.shape = (batch_size, num_obs_bytes)

                cmp_table = pred_batch ^ obs_batch
                err_mask = np.any(cmp_table, axis=1)
                if postselected_observable_mask is not None:
                    discard_mask = np.any(cmp_table & postselected_observable_mask, axis=1)
                    err_mask &= ~discard_mask
                    num_discards += np.count_nonzero(discard_mask)

                if count_observable_error_combos:
                    for misprediction_arr in cmp_table[err_mask]:
                        err_key = "obs_mistake_mask=" + ''.join('_E'[b] for b in np.unpackbits(misprediction_arr, count=num_obs, bitorder='little'))
                        custom_counts[err_key] += 1

                num_errors += np.count_nonzero(err_mask)
                num_shots_left -= batch_size
    return num_discards, num_errors, custom_counts


def sample_decode(*,
                  circuit_obj: Optional[stim.Circuit],
                  circuit_path: Union[None, str, pathlib.Path],
                  dem_obj: Optional[stim.DetectorErrorModel],
                  dem_path: Union[None, str, pathlib.Path],
                  post_mask: Optional[np.ndarray] = None,
                  postselected_observable_mask: Optional[np.ndarray] = None,
                  count_observable_error_combos: bool = False,
                  count_detection_events: bool = False,
                  num_shots: int,
                  decoder: str,
                  tmp_dir: Union[str, pathlib.Path, None] = None,
                  custom_decoders: Optional[Dict[str, 'sinter.Decoder']] = None,
                  __private__unstable__force_decode_on_disk: Optional[bool] = None,
                  ) -> AnonTaskStats:
    """Samples how many times a decoder correctly predicts the logical frame.

    Args:
        circuit_obj: The noisy circuit to sample from and decode results for.
            Must specify circuit_obj XOR circuit_path.
        circuit_path: The file storing the circuit to sample from.
            Must specify circuit_obj XOR circuit_path.
        dem_obj: The error model to give to the decoder.
            Must specify dem_obj XOR dem_path.
        dem_path: The file storing the error model to give to the decoder.
            Must specify dem_obj XOR dem_path.
        post_mask: Postselection mask. Any samples that have a non-zero result
            at a location where the mask has a 1 bit are discarded. If set to
            None, no postselection is performed.
        postselected_observable_mask: Bit packed mask indicating which observables to
            postselect on. If the decoder incorrectly predicts any of these observables, the
            shot is discarded instead of counted as an error.
        count_observable_error_combos: Defaults to False. When set to to True,
            the returned AnonTaskStats will have a custom counts field with keys
            like `obs_mistake_mask=E_E__` counting how many times specific
            combinations of observables were mispredicted by the decoder.
        count_detection_events: Defaults to False. When set to True, the
            returned AnonTaskStats will have a custom counts field withs the
            key `detection_events` counting the number of times a detector fired
            and also `detectors_checked` counting the number of detectors that
            were executed. The detection fraction is the ratio of these two
            numbers.
        num_shots: The number of sample shots to take from the circuit.
        decoder: The name of the decoder to use. Allowed values are:
            "pymatching":
                Use pymatching min-weight-perfect-match decoder.
            "internal":
                Use internal decoder with uncorrelated decoding.
            "internal_correlated":
                Use internal decoder with correlated decoding.
        tmp_dir: An existing directory that is currently empty where temporary
            files can be written as part of performing decoding. If set to
            None, one is created using the tempfile package.
        custom_decoders: Custom decoders that can be used if requested by name.
            If not specified, only decoders built into sinter, such as
            'pymatching' and 'fusion_blossom', can be used.
    """
    if (circuit_obj is None) == (circuit_path is None):
        raise ValueError('(circuit_obj is None) == (circuit_path is None)')
    if (dem_obj is None) == (dem_path is None):
        raise ValueError('(dem_obj is None) == (dem_path is None)')
    if num_shots == 0:
        return AnonTaskStats()

    decoder_obj: Optional[Decoder] = None
    if custom_decoders is not None:
        decoder_obj = custom_decoders.get(decoder)
    if decoder_obj is None:
        decoder_obj = BUILT_IN_DECODERS.get(decoder)
    if decoder_obj is None:
        raise NotImplementedError(f"Unrecognized decoder: {decoder!r}")

    dem: stim.DetectorErrorModel
    if dem_obj is None:
        dem = stim.DetectorErrorModel.from_file(dem_path)
    else:
        dem = dem_obj

    circuit: stim.Circuit
    if circuit_path is not None:
        circuit = stim.Circuit.from_file(circuit_path)
    else:
        circuit = circuit_obj

    start_time = time.monotonic()
    try:
        if __private__unstable__force_decode_on_disk:
            raise NotImplementedError()
        compiled_decoder = decoder_obj.compile_decoder_for_dem(dem=dem)
        return _sample_decode_helper_using_memory(
            circuit=circuit,
            post_mask=post_mask,
            postselected_observable_mask=postselected_observable_mask,
            compiled_decoder=compiled_decoder,
            total_num_shots=num_shots,
            num_det=circuit.num_detectors,
            mini_batch_size=1024,
            start_time_monotonic=start_time,
            num_obs=circuit.num_observables,
            count_observable_error_combos=count_observable_error_combos,
            count_detection_events=count_detection_events,
        )
    except NotImplementedError:
        assert __private__unstable__force_decode_on_disk or __private__unstable__force_decode_on_disk is None
        pass
    return _sample_decode_helper_using_disk(
        circuit=circuit,
        dem=dem,
        dem_path=dem_path,
        post_mask=post_mask,
        postselected_observable_mask=postselected_observable_mask,
        num_shots=num_shots,
        decoder_obj=decoder_obj,
        tmp_dir=tmp_dir,
        start_time_monotonic=start_time,
        count_observable_error_combos=count_observable_error_combos,
        count_detection_events=count_detection_events,
    )


def _sample_decode_helper_using_memory(
    *,
    circuit: stim.Circuit,
    post_mask: Optional[np.ndarray],
    postselected_observable_mask: Optional[np.ndarray],
    num_obs: int,
    num_det: int,
    total_num_shots: int,
    mini_batch_size: int,
    compiled_decoder: CompiledDecoder,
    start_time_monotonic: float,
    count_observable_error_combos: bool,
    count_detection_events: bool,
) -> AnonTaskStats:
    sampler: stim.CompiledDetectorSampler = circuit.compile_detector_sampler()

    out_num_discards = 0
    out_num_errors = 0
    shots_left = total_num_shots
    custom_counts = collections.Counter()
    while shots_left > 0:
        cur_num_shots = min(shots_left, mini_batch_size)
        dets_data, obs_data = sampler.sample(shots=cur_num_shots, separate_observables=True, bit_packed=True)

        # Discard any shots that contain a postselected detection events.
        if post_mask is not None:
            discarded_flags = np.any(dets_data & post_mask, axis=1)
            cur_num_discarded_shots = np.count_nonzero(discarded_flags)
            if cur_num_discarded_shots:
                out_num_discards += cur_num_discarded_shots
                dets_data = dets_data[~discarded_flags, :]
                obs_data = obs_data[~discarded_flags, :]

        # Have the decoder predict which observables are flipped.
        predict_data = compiled_decoder.decode_shots_bit_packed(bit_packed_detection_event_data=dets_data)

        # Discard any shots where the decoder predicts a flipped postselected observable.
        if postselected_observable_mask is not None:
            discarded_flags = np.any(postselected_observable_mask & (predict_data ^ obs_data), axis=1)
            cur_num_discarded_shots = np.count_nonzero(discarded_flags)
            if cur_num_discarded_shots:
                out_num_discards += cur_num_discarded_shots
                obs_data = obs_data[~discarded_flags, :]
                predict_data = predict_data[~discarded_flags, :]

        # Count how many mistakes the decoder made on non-discarded shots.
        mispredictions = obs_data ^ predict_data
        err_mask = np.any(mispredictions, axis=1)
        if count_detection_events:
            for b in range(8):
                custom_counts['detection_events'] += np.count_nonzero(dets_data & (1 << b))
        if count_observable_error_combos:
            for misprediction_arr in mispredictions[err_mask]:
                err_key = "obs_mistake_mask=" + ''.join('_E'[b] for b in np.unpackbits(misprediction_arr, count=num_obs, bitorder='little'))
                custom_counts[err_key] += 1
        out_num_errors += np.count_nonzero(err_mask)
        shots_left -= cur_num_shots

    if count_detection_events:
        custom_counts['detectors_checked'] += num_det * total_num_shots
    return AnonTaskStats(
        shots=total_num_shots,
        errors=out_num_errors,
        discards=out_num_discards,
        seconds=time.monotonic() - start_time_monotonic,
        custom_counts=custom_counts,
    )


def _sample_decode_helper_using_disk(
    *,
    circuit: stim.Circuit,
    dem: stim.DetectorErrorModel,
    dem_path: Union[str, pathlib.Path],
    post_mask: Optional[np.ndarray],
    postselected_observable_mask: Optional[np.ndarray],
    num_shots: int,
    decoder_obj: Decoder,
    tmp_dir: Union[str, pathlib.Path, None],
    start_time_monotonic: float,
    count_observable_error_combos: bool,
    count_detection_events: bool,
) -> AnonTaskStats:
    with contextlib.ExitStack() as exit_stack:
        if tmp_dir is None:
            tmp_dir = exit_stack.enter_context(tempfile.TemporaryDirectory())
        tmp_dir = pathlib.Path(tmp_dir)
        if dem_path is None:
            dem_path = tmp_dir / 'tmp.dem'
            dem.to_file(dem_path)
        dem_path = pathlib.Path(dem_path)

        dets_all_path = tmp_dir / 'sinter_dets.all.b8'
        obs_all_path = tmp_dir / 'sinter_obs.all.b8'
        dets_kept_path = tmp_dir / 'sinter_dets.kept.b8'
        obs_kept_path = tmp_dir / 'sinter_obs.kept.b8'
        predictions_path = tmp_dir / 'sinter_predictions.b8'

        num_dets = circuit.num_detectors
        num_obs = circuit.num_observables

        # Sample data using Stim.
        sampler: stim.CompiledDetectorSampler = circuit.compile_detector_sampler()
        sampler.sample_write(
            num_shots,
            filepath=str(dets_all_path),
            obs_out_filepath=str(obs_all_path),
            format='b8',
            obs_out_format='b8',
        )

        # Postselect, then split into detection event data and observable data.
        if post_mask is None:
            num_det_discards = 0
            dets_used_path = dets_all_path
            obs_used_path = obs_all_path
        else:
            num_det_discards = streaming_post_select(
                num_shots=num_shots,
                num_dets=num_dets,
                num_obs=num_obs,
                dets_in_b8=dets_all_path,
                dets_out_b8=dets_kept_path,
                obs_in_b8=obs_all_path,
                obs_out_b8=obs_kept_path,
                post_mask=post_mask,
                discards_out_b8=None,
            )
            dets_used_path = dets_kept_path
            obs_used_path = obs_kept_path
        num_kept_shots = num_shots - num_det_discards

        # Perform syndrome decoding to predict observables from detection events.
        decoder_obj.decode_via_files(
            num_shots=num_kept_shots,
            num_dets=num_dets,
            num_obs=num_obs,
            dem_path=dem_path,
            dets_b8_in_path=dets_used_path,
            obs_predictions_b8_out_path=predictions_path,
            tmp_dir=tmp_dir,
        )

        # Count how many predictions matched the actual observable data.
        num_obs_discards, num_errors, custom_counts = _streaming_count_mistakes(
            num_shots=num_kept_shots,
            num_obs=num_obs,
            num_det=num_dets,
            dets_in=dets_all_path,
            obs_in=obs_used_path,
            predictions_in=predictions_path,
            postselected_observable_mask=postselected_observable_mask,
            count_detection_events=count_detection_events,
            count_observable_error_combos=count_observable_error_combos,
        )

        return AnonTaskStats(
            shots=num_shots,
            errors=num_errors,
            discards=num_obs_discards + num_det_discards,
            seconds=time.monotonic() - start_time_monotonic,
            custom_counts=custom_counts,
        )
