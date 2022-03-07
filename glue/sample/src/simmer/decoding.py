import dataclasses
import functools
import time
from typing import Optional, Dict, Callable, Tuple

import numpy as np
import stim

from simmer.decoding_internal import decode_using_internal_decoder
from simmer.decoding_pymatching import decode_using_pymatching


DECODER_METHODS: Dict[str, Callable] = {
    'pymatching': decode_using_pymatching,
    'internal': functools.partial(decode_using_internal_decoder, use_correlated_decoding=False),
    'internal_correlated': functools.partial(decode_using_internal_decoder, use_correlated_decoding=True),
}


@dataclasses.dataclass(frozen=True)
class CaseStats:
    num_shots: int = 0
    num_errors: int = 0
    num_discards: int = 0
    seconds_elapsed: float = 0

    def __post_init__(self):
        assert isinstance(self.num_errors, int)
        assert isinstance(self.num_shots, int)
        assert isinstance(self.num_discards, int)
        assert isinstance(self.seconds_elapsed, (int, float))
        assert self.num_errors >= 0
        assert self.num_discards >= 0
        assert self.seconds_elapsed >= 0
        assert self.num_shots >= self.num_errors + self.num_discards

    def __add__(self, other: 'CaseStats') -> 'CaseStats':
        if not isinstance(other, CaseStats):
            return NotImplemented
        return CaseStats(
            num_shots=self.num_shots + other.num_shots,
            num_errors=self.num_errors + other.num_errors,
            num_discards=self.num_discards + other.num_discards,
            seconds_elapsed=self.seconds_elapsed + other.seconds_elapsed,
        )


@dataclasses.dataclass(frozen=True)
class Case:
    # Fields included in CSV data.
    name: str
    strong_id: str
    decoder: str
    num_shots: int

    # Fields not included in CSV data.
    circuit: stim.Circuit
    dem: stim.DetectorErrorModel
    post_mask: Optional[np.ndarray]

    def run(self) -> CaseStats:
        return sample_decode(
            num_shots=self.num_shots,
            circuit=self.circuit,
            post_mask=self.post_mask,
            decoder_error_model=self.dem,
            decoder=self.decoder,
        )


def _post_select(data: np.ndarray, *, post_mask: Optional[np.ndarray] = None) -> Tuple[int, np.ndarray]:
    if post_mask is None:
        return 0, data
    assert post_mask.shape == (data.shape[1],)
    assert post_mask.dtype == np.uint8
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
                  decoder_error_model: Optional[stim.DetectorErrorModel] = None,
                  post_mask: Optional[np.ndarray] = None,
                  num_shots: int,
                  decoder: str) -> CaseStats:
    """Counts how many times a decoder correctly predicts the logical frame of simulated runs.

    Args:
        circuit: The noisy circuit to sample from and decode results for.
        decoder_error_model: The error model to give to the decoder. If set to none, it is derived
            from the circuit.
        post_mask: Post selection mask. Any samples that have a non-zero result at a location where
            the mask has a 1 bit are discarded.
        num_shots: The number of sample shots to take from the cirucit.
        decoder: The name of the decoder to use. Allowed values are:
            "pymatching": Use pymatching.
            "internal": Use an internal decoder at `src/internal_decoder.binary` (not publically available).
            "internal_correlated": Use the internal decoder and tell it to do correlated decoding.
    """
    start_time = time.monotonic()

    num_dets = circuit.num_detectors
    num_obs = circuit.num_observables
    if decoder_error_model is None:
        decoder_error_model = circuit.detector_error_model(decompose_errors=True)
    else:
        assert decoder_error_model.num_detectors == num_dets
        assert decoder_error_model.num_observables == num_obs

    # Sample data using Stim.
    sampler = circuit.compile_detector_sampler()
    concat_data = sampler.sample_bit_packed(num_shots, append_observables=True)

    # Postselect, then split into detection event data and actual observable data.
    num_discards, concat_data = _post_select(concat_data, post_mask=post_mask)
    det_data, obs_data = _split_det_obs_data(num_dets=num_dets, num_obs=num_obs, data=concat_data)

    # Perform syndrome decoding to predict observable data from detection event data.
    decode_method = DECODER_METHODS.get(decoder)
    if decode_method is None:
        raise NotImplementedError(f"Unrecognized decoder: {decoder!r}")
    predictions = decode_method(
        error_model=decoder_error_model,
        bit_packed_det_samples=det_data,
    )
    assert predictions.shape == obs_data.shape
    assert predictions.dtype == obs_data.dtype == np.bool8

    # Count how many predictions matched the actual observable data.
    num_errors = np.count_nonzero(np.any(predictions != obs_data, axis=1))
    return CaseStats(
        num_shots=num_shots,
        num_errors=num_errors,
        num_discards=num_discards,
        seconds_elapsed=time.monotonic() - start_time,
    )
