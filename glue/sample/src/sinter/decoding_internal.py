import pathlib

import stim


def decode_using_internal_decoder(*,
                                  num_shots: int,
                                  num_dets: int,
                                  num_obs: int,
                                  error_model: stim.DetectorErrorModel,
                                  dets_b8_in_path: pathlib.Path,
                                  obs_predictions_b8_out_path: pathlib.Path,
                                  tmp_dir: pathlib.Path,
                                  use_correlated_decoding: bool,
                                  ) -> None:
    import gqec  # Internal python wheel.

    dem_file = f"{tmp_dir}/model.dem"
    with open(dem_file, "w") as f:
        print(error_model, file=f)

    gqec.run_finite_match_main(
        dem_filepath=dem_file,
        output_type="predictions:b8",
        output_filepath=str(obs_predictions_b8_out_path),
        dets_has_observables=False,
        dets_format='b8',
        dets_filepath=str(dets_b8_in_path),
        ignore_distance_1_errors=True,
        ignore_undecomposed_errors=True,
        use_correlated_decoding=use_correlated_decoding,
    )
