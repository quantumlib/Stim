from typing import Dict

from sinter._decoding_decoder_class import Decoder
from sinter._decoding_fusion_blossom import FusionBlossomDecoder
from sinter._decoding_internal import InternalDecoder
from sinter._decoding_pymatching import PyMatchingDecoder

BUILT_IN_DECODERS: Dict[str, Decoder] = {
    'pymatching': PyMatchingDecoder(),
    'fusion_blossom': FusionBlossomDecoder(),
    'internal': InternalDecoder('internal'),
    'internal_correlated': InternalDecoder('internal_correlated'),
    'internal_2': InternalDecoder('internal_2'),
    'internal_correlated_2': InternalDecoder('internal_correlated_2'),
}
