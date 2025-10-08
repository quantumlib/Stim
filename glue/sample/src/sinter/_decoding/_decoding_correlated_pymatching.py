from packaging import version

from sinter._decoding._decoding_decoder_class import Decoder, CompiledDecoder


class CorrelatedPyMatchingCompiledDecoder(CompiledDecoder):
    def __init__(self, matcher: "pymatching.Matching"):
        self.matcher = matcher

    def decode_shots_bit_packed(
        self,
        *,
        bit_packed_detection_event_data: "np.ndarray",
    ) -> "np.ndarray":
        return self.matcher.decode_batch(
            shots=bit_packed_detection_event_data,
            bit_packed_shots=True,
            bit_packed_predictions=True,
            return_weights=False,
        )


class CorrelatedPyMatchingDecoder(Decoder):
    """Use correlated pymatching to predict observables from detection events."""

    def compile_decoder_for_dem(
        self, *, dem: "stim.DetectorErrorModel"
    ) -> CompiledDecoder:
        try:
            import pymatching
        except ImportError as ex:
            raise ImportError(
                "The decoder 'pymatching' isn't installed\n"
                "To fix this, install the python package 'pymatching' into your environment.\n"
                "For example, if you are using pip, run `pip install pymatching`.\n"
            ) from ex

        # correlated matching requires pymatching 2.3.1 or later
        if version.parse(pymatching.__version__) < version.parse("2.3.1"):
            raise ValueError("""
The correlated pymatching decoder requires pymatching 2.3.1 or later.

If you're using pip to install packages, this can be fixed by running
```
pip install "pymatching~=2.3.1" --upgrade
```
""")

        return CorrelatedPyMatchingCompiledDecoder(
            pymatching.Matching.from_detector_error_model(dem, enable_correlations=True)
        )

    def decode_via_files(
        self,
        *,
        num_shots: int,
        num_dets: int,
        num_obs: int,
        dem_path: "pathlib.Path",
        dets_b8_in_path: "pathlib.Path",
        obs_predictions_b8_out_path: "pathlib.Path",
        tmp_dir: "pathlib.Path",
    ) -> None:
        try:
            import pymatching
        except ImportError as ex:
            raise ImportError(
                "The decoder 'pymatching' isn't installed\n"
                "To fix this, install the python package 'pymatching' into your environment.\n"
                "For example, if you are using pip, run `pip install pymatching`.\n"
            ) from ex

        # correlated matching requires pymatching 2.3.1 or later
        if version.parse(pymatching.__version__) < version.parse("2.3.1"):
            raise ValueError("""
The correlated pymatching decoder requires pymatching 2.3.1 or later.

If you're using pip to install packages, this can be fixed by running
```
pip install "pymatching~=2.3.1" --upgrade
```
""")

        if num_dets == 0:
            with open(obs_predictions_b8_out_path, "wb") as f:
                f.write(b"\0" * (num_obs * num_shots))
            return

        result = pymatching.cli(
            command_line_args=[
                "predict",
                "--dem",
                str(dem_path),
                "--in",
                str(dets_b8_in_path),
                "--in_format",
                "b8",
                "--out",
                str(obs_predictions_b8_out_path),
                "--out_format",
                "b8",
                "--enable_correlations",
            ]
        )
        if result:
            raise ValueError("pymatching.cli returned a non-zero exit code")
