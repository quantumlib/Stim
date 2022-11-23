import pathlib

from sinter._decoding_decoder_class import Decoder
from sinter._decoding_pymatching_v1 import decode_using_pymatching_v1
from sinter._decoding_pymatching_v2 import decode_using_pymatching_v2


class PyMatchingDecoder(Decoder):
    """Use pymatching to predict observables from detection events."""

    def __init__(self):
        self._method_to_use = None

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
        try:
            import pymatching
        except ImportError as ex:
            raise ImportError(
                "The decoder 'pymatching' isn't installed\n"
                "To fix this, install the python package 'pymatching' into your environment.\n"
                "For example, if you are using pip, run `pip install pymatching`.\n"
            ) from ex

        if self._method_to_use is None:
            if getattr(getattr(pymatching, '_cpp_pymatching', None), 'main', None) is not None:
                self._method_to_use = decode_using_pymatching_v2
            else:
                self._method_to_use = decode_using_pymatching_v1

        self._method_to_use(
            num_shots=num_shots,
            num_dets=num_dets,
            num_obs=num_obs,
            dem_path=dem_path,
            dets_b8_in_path=dets_b8_in_path,
            obs_predictions_b8_out_path=obs_predictions_b8_out_path,
            tmp_dir=tmp_dir,
        )
