import collections
import pathlib
import random
import time
from typing import Optional
from typing import Union

import numpy as np

from sinter._data import Task, AnonTaskStats
from sinter._decoding._sampler import Sampler, CompiledSampler
from sinter._decoding._decoding_decoder_class import Decoder, CompiledDecoder


class StimThenDecodeSampler(Sampler):
    """Samples shots using stim, then decodes using the given decoder.

    This is the default sampler; the one used to wrap decoders with no
    specified sampler.

    The decoder's goal is to predict the observable flips given the detection
    event data. Errors are when the prediction is wrong. Discards are when the
    decoder returns an extra byte of prediction data for each shot, and the
    extra byte is not zero.
    """
    def __init__(
        self,
        *,
        decoder: Decoder,
        count_observable_error_combos: bool,
        count_detection_events: bool,
        tmp_dir: Optional[pathlib.Path],
    ):
        self.decoder = decoder
        self.count_observable_error_combos = count_observable_error_combos
        self.count_detection_events = count_detection_events
        self.tmp_dir = tmp_dir

    def compiled_sampler_for_task(self, task: Task) -> CompiledSampler:
        return _CompiledStimThenDecodeSampler(
            decoder=self.decoder,
            task=task,
            count_detection_events=self.count_detection_events,
            count_observable_error_combos=self.count_observable_error_combos,
            tmp_dir=self.tmp_dir,
        )


def classify_discards_and_errors(
    *,
    actual_obs: np.ndarray,
    predictions: np.ndarray,
    postselected_observables_mask: Union[np.ndarray, None],
    out_count_observable_error_combos: Union[None, collections.Counter[str]],
    num_obs: int,
) -> tuple[int, int]:
    num_discards = 0

    # Added bytes are used for signalling discards.
    if predictions.shape[1] == actual_obs.shape[1] + 1:
        discard_mask = predictions[:, -1] != 0
        predictions = predictions[:, :-1]
        num_discards += np.count_nonzero(discard_mask)
        discard_mask ^= True
        actual_obs = actual_obs[discard_mask]
        predictions = predictions[discard_mask]

    # Mispredicted observables can be used for signalling discards.
    if postselected_observables_mask is not None:
        discard_mask = np.any((actual_obs ^ predictions) & postselected_observables_mask, axis=1)
        num_discards += np.count_nonzero(discard_mask)
        discard_mask ^= True
        actual_obs = actual_obs[discard_mask]
        predictions = predictions[discard_mask]

    fail_mask = np.any(actual_obs != predictions, axis=1)
    if out_count_observable_error_combos is not None:
        for k in np.flatnonzero(fail_mask):
            mistakes = np.unpackbits(actual_obs[k] ^ predictions[k], count=num_obs, bitorder='little')
            err_key = "obs_mistake_mask=" + ''.join('_E'[b] for b in mistakes)
            out_count_observable_error_combos[err_key] += 1

    num_errors = np.count_nonzero(fail_mask)
    return num_discards, num_errors


class DiskDecoder(CompiledDecoder):
    def __init__(self, decoder: Decoder, task: Task, tmp_dir: pathlib.Path):
        self.decoder = decoder
        self.task = task
        self.top_tmp_dir: pathlib.Path = tmp_dir

        while True:
            k = random.randint(0, 2**64)
            self.top_tmp_dir = tmp_dir / f'disk_decoder_{k}'
            try:
                self.top_tmp_dir.mkdir()
                break
            except FileExistsError:
                pass
        self.decoder_tmp_dir: pathlib.Path = self.top_tmp_dir / 'dec'
        self.decoder_tmp_dir.mkdir()
        self.num_obs = task.detector_error_model.num_observables
        self.num_dets = task.detector_error_model.num_detectors
        self.dem_path = self.top_tmp_dir / 'dem.dem'
        self.dets_b8_in_path = self.top_tmp_dir / 'dets.b8'
        self.obs_predictions_b8_out_path = self.top_tmp_dir / 'obs.b8'
        self.task.detector_error_model.to_file(self.dem_path)

    def decode_shots_bit_packed(
            self,
            *,
            bit_packed_detection_event_data: np.ndarray,
    ) -> np.ndarray:
        num_shots = bit_packed_detection_event_data.shape[0]
        with open(self.dets_b8_in_path, 'wb') as f:
            bit_packed_detection_event_data.tofile(f)
        self.decoder.decode_via_files(
            num_shots=num_shots,
            num_obs=self.num_obs,
            num_dets=self.num_dets,
            dem_path=self.dem_path,
            dets_b8_in_path=self.dets_b8_in_path,
            obs_predictions_b8_out_path=self.obs_predictions_b8_out_path,
            tmp_dir=self.decoder_tmp_dir,
        )
        num_obs_bytes = (self.num_obs + 7) // 8
        with open(self.obs_predictions_b8_out_path, 'rb') as f:
            prediction = np.fromfile(f, dtype=np.uint8, count=num_obs_bytes * num_shots)
            assert prediction.shape == (num_obs_bytes * num_shots,)
        self.obs_predictions_b8_out_path.unlink()
        self.dets_b8_in_path.unlink()
        return prediction.reshape((num_shots, num_obs_bytes))


def _compile_decoder_with_disk_fallback(
    decoder: Decoder,
    task: Task,
    tmp_dir: Optional[pathlib.Path],
) -> CompiledDecoder:
    try:
        return decoder.compile_decoder_for_dem(dem=task.detector_error_model)
    except (NotImplementedError, ValueError):
        pass
    if tmp_dir is None:
        raise ValueError(f"Decoder {task.decoder=} didn't implement `compile_decoder_for_dem`, but no temporary directory was provided for falling back to `decode_via_files`.")
    return DiskDecoder(decoder, task, tmp_dir)


class _CompiledStimThenDecodeSampler(CompiledSampler):
    def __init__(
        self,
        *,
        decoder: Decoder,
        task: Task,
        count_observable_error_combos: bool,
        count_detection_events: bool,
        tmp_dir: Optional[pathlib.Path],
    ):
        self.task = task
        self.compiled_decoder = _compile_decoder_with_disk_fallback(decoder, task, tmp_dir)
        self.stim_sampler = task.circuit.compile_detector_sampler()
        self.count_observable_error_combos = count_observable_error_combos
        self.count_detection_events = count_detection_events
        self.num_det = self.task.circuit.num_detectors
        self.num_obs = self.task.circuit.num_observables

    def sample(self, max_shots: int) -> AnonTaskStats:
        t0 = time.monotonic()
        dets, actual_obs = self.stim_sampler.sample(
            shots=max_shots,
            bit_packed=True,
            separate_observables=True,
        )
        num_shots = dets.shape[0]

        custom_counts = collections.Counter()
        if self.count_detection_events:
            custom_counts['detectors_checked'] += self.num_det * num_shots
            for b in range(8):
                custom_counts['detection_events'] += np.count_nonzero(dets & (1 << b))

        # Discard any shots that contain a postselected detection events.
        if self.task.postselection_mask is not None:
            discarded_flags = np.any(dets & self.task.postselection_mask, axis=1)
            num_discards_1 = np.count_nonzero(discarded_flags)
            if num_discards_1:
                dets = dets[~discarded_flags, :]
                actual_obs = actual_obs[~discarded_flags, :]
        else:
            num_discards_1 = 0

        predictions = self.compiled_decoder.decode_shots_bit_packed(bit_packed_detection_event_data=dets)
        if not isinstance(predictions, np.ndarray):
            raise ValueError("not isinstance(predictions, np.ndarray)")
        if predictions.dtype != np.uint8:
            raise ValueError("predictions.dtype != np.uint8")
        if len(predictions.shape) != 2:
            raise ValueError("len(predictions.shape) != 2")
        if predictions.shape[0] != num_shots:
            raise ValueError("predictions.shape[0] != num_shots")
        if predictions.shape[1] < actual_obs.shape[1]:
            raise ValueError("predictions.shape[1] < actual_obs.shape[1]")
        if predictions.shape[1] > actual_obs.shape[1] + 1:
            raise ValueError("predictions.shape[1] > actual_obs.shape[1] + 1")

        num_discards_2, num_errors = classify_discards_and_errors(
            actual_obs=actual_obs,
            predictions=predictions,
            postselected_observables_mask=self.task.postselected_observables_mask,
            out_count_observable_error_combos=custom_counts if self.count_observable_error_combos else None,
            num_obs=self.num_obs,
        )
        t1 = time.monotonic()

        return AnonTaskStats(
            shots=num_shots,
            errors=num_errors,
            discards=num_discards_1 + num_discards_2,
            seconds=t1 - t0,
            custom_counts=custom_counts,
        )
