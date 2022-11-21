import pathlib

from sinter._decoding_decoder_class import Decoder


class InternalDecoder(Decoder):
    """Use internal decoders to predict observables from detection events."""

    def __init__(self, name: str):
        self.name = name

    def decode_via_files(self,
                         *,
                         num_shots: int,
                         num_dets: int,
                         num_obs: int,
                         dem_path: pathlib.Path,
                         dets_b8_in_path: pathlib.Path,
                         obs_predictions_b8_out_path: pathlib.Path,
                         tmp_dir: pathlib.Path,
                       ) -> None:

        if num_dets == 0:
            with open(obs_predictions_b8_out_path, 'wb') as f:
                f.write(b'\0' * (num_obs * num_shots))
            return

        try:
            import gqec  # Internal python wheel.
        except ImportError as ex:
            raise ImportError(
                "The decoder 'internal*' isn't installed.\n"
                "These decoders aren't publicly available.\n"
            ) from ex

        gqec.run_for_sinter(str(dem_path), str(dets_b8_in_path), "b8", str(obs_predictions_b8_out_path), self.name)
