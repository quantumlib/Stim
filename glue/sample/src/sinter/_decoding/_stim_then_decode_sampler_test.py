import collections

import numpy as np

from sinter._decoding._stim_then_decode_sampler import \
    classify_discards_and_errors


def test_classify_discards_and_errors():
    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (0, 0)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0],
            [2, 2],
            [3, 2],
            [4, 1],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (0, 2)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 0],
            [2, 2, 0],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (0, 2)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 0],
            [2, 2, 1],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (1, 2)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 1],
            [2, 2, 0],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (1, 1)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 1],
            [2, 2, 1],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (2, 1)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [2, 3],
            [1, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 1],
            [2, 2, 1],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=np.array([1, 0]),
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (3, 2)

    counter = collections.Counter()
    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [1, 2],
        ], dtype=np.uint8),
        predictions=np.array([
            [1, 0],
            [1, 2],
        ], dtype=np.uint8),
        postselected_observables_mask=np.array([1, 0]),
        out_count_observable_error_combos=counter,
        num_obs=13,
    ) == (0, 1)
    assert counter == collections.Counter(["obs_mistake_mask=_________E___"])


def test_disk_decoder_reports_discards():
    # DiskDecoder (the compiled-wrapper for file-based decoders) must forward
    # discards_b8_out_path to decode_via_files and return the discard bytes as
    # a trailing column of prediction data.
    import pathlib
    import tempfile

    import stim

    import sinter
    from sinter._decoding._stim_then_decode_sampler import DiskDecoder, StimThenDecodeSampler

    class DiscardingFileDecoder(sinter.Decoder):
        def decode_via_files(self, *, num_shots, num_dets, num_obs, dem_path, dets_b8_in_path, obs_predictions_b8_out_path, tmp_dir, discards_b8_out_path=None):
            num_det_bytes = -(-num_dets // 8)
            num_obs_bytes = -(-num_obs // 8)
            dets = np.fromfile(dets_b8_in_path, dtype=np.uint8, count=num_shots * num_det_bytes).reshape(num_shots, num_det_bytes)
            np.zeros(num_shots * num_obs_bytes, dtype=np.uint8).tofile(obs_predictions_b8_out_path)
            if discards_b8_out_path is not None:
                discards = np.zeros(num_shots, dtype=np.uint8)
                discards[::2] = 1
                discards.tofile(discards_b8_out_path)

    circuit = stim.Circuit.generated(
        "repetition_code:memory",
        rounds=3,
        distance=3,
        before_round_data_depolarization=0,
        before_measure_flip_probability=0,
        after_clifford_depolarization=0,
    )
    dem = circuit.detector_error_model()
    task = sinter.Task(circuit=circuit, detector_error_model=dem)
    num_det_bytes = -(-dem.num_detectors // 8)
    num_obs_bytes = -(-dem.num_observables // 8)

    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)

        # The wrapper returns the discard bytes as a trailing column.
        disk_decoder = DiskDecoder(DiscardingFileDecoder(), task, d)
        dets = np.zeros((10, num_det_bytes), dtype=np.uint8)
        result = disk_decoder.decode_shots_bit_packed(bit_packed_detection_event_data=dets)
        assert result.shape == (10, num_obs_bytes + 1)
        assert np.all(result[:, :-1] == 0)
        assert np.count_nonzero(result[:, -1]) == 5

        # The end-to-end sampler counts the discarded shots, and excludes them
        # from error counting.
        sampler = StimThenDecodeSampler(
            decoder=DiscardingFileDecoder(),
            count_observable_error_combos=False,
            count_detection_events=False,
            tmp_dir=d,
        )
        stats = sampler.compiled_sampler_for_task(task).sample(max_shots=10)
        assert stats.shots == 10
        assert stats.discards == 5
        assert stats.errors == 0
