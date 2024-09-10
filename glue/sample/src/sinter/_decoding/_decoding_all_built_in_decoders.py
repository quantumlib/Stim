from typing import Dict
from typing import Union

from sinter._decoding._decoding_decoder_class import Decoder
from sinter._decoding._decoding_fusion_blossom import FusionBlossomDecoder
from sinter._decoding._decoding_pymatching import PyMatchingDecoder
from sinter._decoding._decoding_vacuous import VacuousDecoder
from sinter._decoding._perfectionist_sampler import PerfectionistSampler
from sinter._decoding._sampler import Sampler

BUILT_IN_DECODERS: Dict[str, Decoder] = {
    'vacuous': VacuousDecoder(),
    'pymatching': PyMatchingDecoder(),
    'fusion_blossom': FusionBlossomDecoder(),
}

BUILT_IN_SAMPLERS: Dict[str, Union[Decoder, Sampler]] = {
    **BUILT_IN_DECODERS,
    'perfectionist': PerfectionistSampler(),
}
