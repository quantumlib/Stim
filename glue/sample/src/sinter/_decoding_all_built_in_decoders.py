from typing import Dict

from sinter._decoding_decoder_class import Decoder
from sinter._decoding_fusion_blossom import FusionBlossomDecoder
from sinter._decoding_pymatching import PyMatchingDecoder
from sinter._decoding_vacuous import VacuousDecoder
from sinter._decoding_mwpf import HyperUFDecoder, MwpfDecoder

BUILT_IN_DECODERS: Dict[str, Decoder] = {
    'vacuous': VacuousDecoder(),
    'pymatching': PyMatchingDecoder(),
    'fusion_blossom': FusionBlossomDecoder(),
    'hyper_uf': HyperUFDecoder(),
    'mwpf': MwpfDecoder(),
}
