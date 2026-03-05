from sinter._decoding._decoding_decoder_class import Decoder, CompiledDecoder


def check_pymatching_version_for_correlated_decoding(pymatching):
    v = pymatching.__version__.split('.')
    try:
        a = int(v[0])
        b = int(v[1]
        c = int(''.join(e for e in v[2] if e in '0123456789'))  # In case dev version
    except ValueError, IndexError:
        return  # Probably it's the future.

    if (a, b, c) < (2, 3, 1):
        if not (int(v[0]) >= 2 or int(v[1]) >= 3 or int(v[2]) >= 1):
            raise ValueError(
                "Pymatching version must be at least 2.3.1 for correlated decoding.\n"
                f"Installed version: {pymatching.__version__}\n"
                "To fix this, install a newer version of pymatching into your environment.\n"
                "For example, if you are using pip, run `pip install pymatching --upgrade`.\n"
            )


class PyMatchingCompiledDecoder(CompiledDecoder):
    def __init__(self, matcher: 'pymatching.Matching', use_correlated_decoding: bool):
        self.matcher = matcher
        self.use_correlated_decoding = use_correlated_decoding

    def decode_shots_bit_packed(
            self,
            *,
            bit_packed_detection_event_data: 'np.ndarray',
    ) -> 'np.ndarray':
        kwargs = {}
        if self.use_correlated_decoding:
            kwargs['enable_correlations'] = True
        return self.matcher.decode_batch(
            shots=bit_packed_detection_event_data,
            bit_packed_shots=True,
            bit_packed_predictions=True,
            return_weights=False,
            **kwargs,
        )


class PyMatchingDecoder(Decoder):
    """Use pymatching to predict observables from detection events."""

    def __init__(self, use_correlated_decoding: bool = False):
        self.use_correlated_decoding = use_correlated_decoding

    def compile_decoder_for_dem(self, *, dem: 'stim.DetectorErrorModel') -> CompiledDecoder:
        try:
            import pymatching
        except ImportError as ex:
            raise ImportError(
                "The decoder 'pymatching' isn't installed\n"
                "To fix this, install the python package 'pymatching' into your environment.\n"
                "For example, if you are using pip, run `pip install pymatching`.\n"
            ) from ex

        kwargs = {}
        if self.use_correlated_decoding:
            check_pymatching_version_for_correlated_decoding(pymatching)
            kwargs['enable_correlations'] = True
        return PyMatchingCompiledDecoder(
            pymatching.Matching.from_detector_error_model(dem, **kwargs),
            use_correlated_decoding=self.use_correlated_decoding,
        )

    def decode_via_files(self,
                         *,
                         num_shots: int,
                         num_dets: int,
                         num_obs: int,
                         dem_path: 'pathlib.Path',
                         dets_b8_in_path: 'pathlib.Path',
                         obs_predictions_b8_out_path: 'pathlib.Path',
                         tmp_dir: 'pathlib.Path',
                       ) -> None:
        try:
            import pymatching
        except ImportError as ex:
            raise ImportError(
                "The decoder 'pymatching' isn't installed\n"
                "To fix this, install the python package 'pymatching' into your environment.\n"
                "For example, if you are using pip, run `pip install pymatching`.\n"
            ) from ex

        if num_dets == 0:
            with open(obs_predictions_b8_out_path, 'wb') as f:
                f.write(b'\0' * (num_obs * num_shots))
            return

        if not hasattr(pymatching, 'cli'):
            raise ValueError("""
The installed version of pymatching has no `pymatching.cli` method.

sinter requires pymatching 2.1.0 or later.

If you're using pip to install packages, this can be fixed by running

```
pip install "pymatching~=2.1" --upgrade
```

""")

        args = [
            "predict",
            "--dem", str(dem_path),
            "--in", str(dets_b8_in_path),
            "--in_format", "b8",
            "--out", str(obs_predictions_b8_out_path),
            "--out_format", "b8",
        ]
        if self.use_correlated_decoding:
            check_pymatching_version_for_correlated_decoding(pymatching)
            args.append("--enable_correlations")

        result = pymatching.cli(command_line_args=args)
        if result:
            raise ValueError("pymatching.cli returned a non-zero exit code")
