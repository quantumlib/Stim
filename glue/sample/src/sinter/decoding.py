import contextlib
import functools
import pathlib
import tempfile
import time
from typing import Optional, Dict, Callable, Tuple

import numpy as np
import stim

from sinter.anon_task_stats import AnonTaskStats
from sinter.decoding_internal import decode_using_internal_decoder
from sinter.decoding_pymatching import decode_using_pymatching


DECODER_METHODS: Dict[str, Callable] = {
    'pymatching': decode_using_pymatching,
    'internal': functools.partial(decode_using_internal_decoder,
                                  use_correlated_decoding=False),
    'internal_correlated': functools.partial(decode_using_internal_decoder,
                                             use_correlated_decoding=True),
}


def _post_select(data: np.ndarray, *, post_mask: Optional[np.ndarray] = None) -> Tuple[int, np.ndarray]:
    if post_mask is None:
        return 0, data
    if post_mask.shape != (data.shape[1],):
        raise ValueError(f"post_mask.shape={post_mask.shape} != (data.shape[1]={data.shape[1]},)")
    if post_mask.dtype != np.uint8:
        raise ValueError(f"post_mask.dtype={post_mask.dtype} != np.uint8")
    discarded = np.any(data & post_mask, axis=1)
    num_discards = np.count_nonzero(discarded)
    return num_discards, data[~discarded]


def _split_det_obs_data(*, num_dets: int, num_obs: int, data: np.ndarray):
    if num_obs == 0:
        det_data = data
        obs_data = np.zeros(shape=(det_data.shape[0], 0), dtype=np.bool8)
    else:
        num_det_bytes = (num_dets + 7) // 8
        num_obs_bytes = (num_dets % 8 + num_obs + 7) // 8
        obs_data = data[:, -num_obs_bytes:]
        obs_data = np.unpackbits(obs_data, axis=1, count=num_obs_bytes * 8, bitorder='little')
        obs_data = obs_data[:, num_dets % 8:][:, :num_obs]
        obs_data = obs_data != 0
        det_data = data[:, :num_det_bytes]
        rem = num_dets % 8
        if rem:
            det_data[:, -1] &= np.uint8((1 << rem) - 1)
    assert obs_data.shape[0] == det_data.shape[0]
    assert obs_data.shape[1] == num_obs
    assert det_data.shape[1] == (num_dets + 7) // 8
    return det_data, obs_data


def sample_decode(*,
                  circuit: stim.Circuit,
                  decoder_error_model: stim.DetectorErrorModel,
                  post_mask: Optional[np.ndarray] = None,
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
    with contextlib.ExitStack() as exit_stack:
        if tmp_dir is None:
            tmp_dir = exit_stack.enter_context(tempfile.TemporaryDirectory())
        start_time = time.monotonic()

        num_dets = circuit.num_detectors
        num_obs = circuit.num_observables
        assert decoder_error_model.num_detectors == num_dets
        assert decoder_error_model.num_observables == num_obs

        # Sample data using Stim.
        sampler = circuit.compile_detector_sampler()
        concat_data = sampler.sample_bit_packed(num_shots, append_observables=True)

        # Postselect, then split into detection event data and observable data.
        num_discards, concat_data = _post_select(concat_data, post_mask=post_mask)
        det_data, obs_data = _split_det_obs_data(num_dets=num_dets,
                                                 num_obs=num_obs,
                                                 data=concat_data)

        # Perform syndrome decoding to predict observables from detection events.
        decode_method = DECODER_METHODS.get(decoder)
        if decode_method is None:
            raise NotImplementedError(f"Unrecognized decoder: {decoder!r}")
        predictions = decode_method(
            error_model=decoder_error_model,
            bit_packed_det_samples=det_data,
            tmp_dir=tmp_dir,
        )
        assert predictions.shape == obs_data.shape
        assert predictions.dtype == obs_data.dtype == np.bool8

        # Count how many predictions matched the actual observable data.
        num_errors = np.count_nonzero(np.any(predictions != obs_data, axis=1))
        return AnonTaskStats(
            shots=num_shots,
            errors=num_errors,
            discards=num_discards,
            seconds=time.monotonic() - start_time,
        )
