import pathlib


def decode_using_pymatching_v2(*,
                               num_shots: int,
                               num_dets: int,
                               num_obs: int,
                               dem_path: pathlib.Path,
                               dets_b8_in_path: pathlib.Path,
                               obs_predictions_b8_out_path: pathlib.Path,
                               tmp_dir: pathlib.Path,
                               ) -> None:
    """Use pymatching to predict observables from detection events."""

    import pymatching
    if num_dets == 0:
        with open(obs_predictions_b8_out_path, 'wb') as f:
            f.write(b'\0' * (num_obs * num_shots))
        return

    cli_method = getattr(pymatching, 'cli')
    if cli_method is None:
        # Backwards compat for pymatching 2.0.0
        cli_method = pymatching._cpp_pymatching.main

    result = cli_method(command_line_args=[
        "predict",
        "--dem", str(dem_path),
        "--in", str(dets_b8_in_path),
        "--in_format", "b8",
        "--out", str(obs_predictions_b8_out_path),
        "--out_format", "b8",
    ])
    if result:
        raise ValueError("pymatching.main returned a non-zero exit code")
