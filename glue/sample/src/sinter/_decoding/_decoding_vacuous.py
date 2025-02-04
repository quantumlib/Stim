import numpy as np

from sinter._decoding._decoding_decoder_class import Decoder, CompiledDecoder


class VacuousDecoder(Decoder):
    """An example decoder that always predicts the observables aren't flipped.
    """

    def compile_decoder_for_dem(self, *, dem: 'stim.DetectorErrorModel') -> CompiledDecoder:
        return VacuousCompiledDecoder(shape=(dem.num_observables + 7) // 8)

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
        with open(obs_predictions_b8_out_path, 'wb') as f:
            f.write(b'\0' * (num_obs * num_shots))


class VacuousCompiledDecoder(CompiledDecoder):
    """An example decoder that always predicts the observables aren't flipped.
    """
    def __init__(self, shape: int):
        self.shape = shape

    def decode_shots_bit_packed(
            self,
            *,
            bit_packed_detection_event_data: 'np.ndarray',
    ) -> 'np.ndarray':
        return np.zeros(shape=(bit_packed_detection_event_data.shape[0], self.shape), dtype=np.uint8)
