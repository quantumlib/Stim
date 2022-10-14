import pathlib

import stim


def decode_using_internal_decoder(*,
                                  num_shots: int,
                                  num_dets: int,
                                  num_obs: int,
                                  dem_path: pathlib.Path,
                                  dets_b8_in_path: pathlib.Path,
                                  obs_predictions_b8_out_path: pathlib.Path,
                                  tmp_dir: pathlib.Path,
                                  use_correlated_decoding: bool,
                                  ) -> None:
    import gqec  # Internal python wheel.

    gqec.run_finite_match_main(
        dem_filepath=str(dem_path),
        output_type="predictions:b8",
        output_filepath=str(obs_predictions_b8_out_path),
        dets_has_observables=False,
        dets_format='b8',
        dets_filepath=str(dets_b8_in_path),
        ignore_distance_1_errors=True,
        ignore_undecomposed_errors=True,
        use_correlated_decoding=use_correlated_decoding,
    )
