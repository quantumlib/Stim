import pathlib

import numpy as np
import stim


def decode_using_internal_decoder(*,
                                  error_model: stim.DetectorErrorModel,
                                  bit_packed_det_samples: np.ndarray,
                                  use_correlated_decoding: bool,
                                  tmp_dir: pathlib.Path,
                                  ) -> np.ndarray:
    import gqec  # Internal python wheel.

    num_shots = bit_packed_det_samples.shape[0]
    num_obs = error_model.num_observables
    assert bit_packed_det_samples.shape[1] == (error_model.num_detectors + 7) // 8
    assert bit_packed_det_samples.dtype == np.uint8

    dem_file = f"{tmp_dir}/model.dem"
    dets_file = f"{tmp_dir}/dets.b8"
    out_file = f"{tmp_dir}/predictions.01"

    with open(dem_file, "w") as f:
        print(error_model, file=f)
    with open(dets_file, "wb") as f:
        bit_packed_det_samples.tofile(f)

    gqec.run_finite_match_main(
        dem_filepath=dem_file,
        output_type="predictions",
        output_filepath=out_file,
        dets_has_observables=False,
        dets_format='b8',
        dets_filepath=dets_file,
        ignore_distance_1_errors=True,
        ignore_undecomposed_errors=True,
        use_correlated_decoding=use_correlated_decoding,
    )

    with open(out_file, "rb") as f:
        # Read text data of expected size as if it were binary.
        predictions = np.fromfile(f, dtype=np.uint8)
        predictions.shape = (num_shots, num_obs + 1)

        # Verify newlines are present, then remove them.
        assert np.all(predictions[:, -1] == ord('\n'))
        predictions = predictions[:, :-1]

        # Verify remaining data is 01 data, then convert to bool8.
        assert np.all(ord('0') <= predictions)
        assert np.all(predictions <= ord('1'))
        predictions = predictions == ord('1')

    return predictions
