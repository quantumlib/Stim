import os

import contextlib
import functools
import pathlib
import subprocess
import tempfile

import math
import time
from typing import Optional, Dict, Callable, Tuple, Iterable

import numpy as np
import stim

from sinter._anon_task_stats import AnonTaskStats
from sinter._decoding_internal import decode_using_internal_decoder
from sinter._decoding_pymatching import decode_using_pymatching


DECODER_METHODS: Dict[str, Callable] = {
    'pymatching': decode_using_pymatching,
    'internal': functools.partial(decode_using_internal_decoder,
                                  use_correlated_decoding=False),
    'internal_correlated': functools.partial(decode_using_internal_decoder,
                                             use_correlated_decoding=True),
}


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
        postselected_observable_mask: Optional[np.ndarray] = None,
        obs_in: pathlib.Path,
        predictions_in: pathlib.Path) -> Tuple[int, int]:

    num_obs_bytes = math.ceil(num_obs / 8)
    num_shots_left = num_shots
    num_errors = 0
    num_discards = 0
    with open(obs_in, 'rb') as obs_in_f:
        with open(predictions_in, 'rb') as predictions_in_f:
            while num_shots_left:
                batch_size = min(num_shots_left, math.ceil(10**6 / max(num_obs, 1)))

                obs_batch = np.fromfile(obs_in_f, dtype=np.uint8, count=num_obs_bytes * batch_size)
                pred_batch = np.fromfile(predictions_in_f, dtype=np.uint8, count=num_obs_bytes * batch_size)
                obs_batch.shape = (batch_size, num_obs_bytes)
                pred_batch.shape = (batch_size, num_obs_bytes)

                cmp_table = pred_batch ^ obs_batch
                if postselected_observable_mask is None:
                    local_discards = 0
                else:
                    local_discards = np.count_nonzero(np.any(cmp_table & postselected_observable_mask, axis=1))
                local_errors_or_discards = np.count_nonzero(np.any(cmp_table, axis=1))
                num_discards += local_discards
                num_errors += local_errors_or_discards - local_discards
                num_shots_left -= batch_size
    return num_discards, num_errors


def sample_decode(*,
                  circuit: stim.Circuit,
                  decoder_error_model: stim.DetectorErrorModel,
                  post_mask: Optional[np.ndarray] = None,
                  postselected_observable_mask: Optional[np.ndarray] = None,
                  num_shots: int,
                  decoder: str,
                  tmp_dir: Optional[pathlib.Path] = None) -> AnonTaskStats:
    """Samples how many times a decoder correctly predicts the logical frame.

    Args:
        circuit: The noisy circuit to sample from and decode results for.
        decoder_error_model: The error model to give to the decoder.
        post_mask: Postselection mask. Any samples that have a non-zero result
            at a location where the mask has a 1 bit are discarded. If set to
            None, no postselection is performed.
        postselected_observable_mask: Bit packed mask indicating which observables to
            postselect on. If the decoder incorrectly predicts any of these observables, the
            shot is discarded instead of counted as an error.
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
    """
    if num_shots == 0:
        return AnonTaskStats()

    with contextlib.ExitStack() as exit_stack:
        if tmp_dir is None:
            tmp_dir = exit_stack.enter_context(tempfile.TemporaryDirectory())
        tmp_dir = pathlib.Path(tmp_dir)
        start_time = time.monotonic()

        dets_all_path = tmp_dir / 'sinter_dets.all.b8'
        obs_all_path = tmp_dir / 'sinter_obs.all.b8'
        dets_kept_path = tmp_dir / 'sinter_dets.kept.b8'
        obs_kept_path = tmp_dir / 'sinter_obs.kept.b8'
        predictions_path = tmp_dir / 'sinter_predictions.b8'

        num_dets = circuit.num_detectors
        num_obs = circuit.num_observables
        assert decoder_error_model.num_detectors == num_dets
        assert decoder_error_model.num_observables == num_obs

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
        decode_method = DECODER_METHODS.get(decoder)
        if decode_method is None:
            raise NotImplementedError(f"Unrecognized decoder: {decoder!r}")
        decode_method(
            num_shots=num_kept_shots,
            num_dets=num_dets,
            num_obs=num_obs,
            error_model=decoder_error_model,
            dets_b8_in_path=dets_used_path,
            obs_predictions_b8_out_path=predictions_path,
            tmp_dir=tmp_dir,
        )

        # Count how many predictions matched the actual observable data.
        num_obs_discards, num_errors = _streaming_count_mistakes(
            num_shots=num_kept_shots,
            num_obs=num_obs,
            obs_in=obs_used_path,
            predictions_in=predictions_path,
            postselected_observable_mask=postselected_observable_mask,
        )

        return AnonTaskStats(
            shots=num_shots,
            errors=num_errors,
            discards=num_obs_discards + num_det_discards,
            seconds=time.monotonic() - start_time,
        )
