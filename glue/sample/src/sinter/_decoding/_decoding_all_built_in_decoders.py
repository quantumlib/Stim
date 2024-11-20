from typing import Dict
from typing import Union

from sinter._decoding._decoding_decoder_class import Decoder
from sinter._decoding._decoding_fusion_blossom import FusionBlossomDecoder
from sinter._decoding._decoding_pymatching import PyMatchingDecoder
from sinter._decoding._decoding_vacuous import VacuousDecoder
from sinter._decoding._perfectionist_sampler import PerfectionistSampler
from sinter._decoding._sampler import Sampler
from sinter._decoding._decoding_mwpf import HyperUFDecoder, MwpfDecoder

BUILT_IN_DECODERS: Dict[str, Decoder] = {
    'vacuous': VacuousDecoder(),
    'pymatching': PyMatchingDecoder(),
    'fusion_blossom': FusionBlossomDecoder(),
    # an implementation of (weighted) hypergraph UF decoder (https://arxiv.org/abs/2103.08049)
    'hypergraph_union_find': HyperUFDecoder(),
    # Minimum-Weight Parity Factor using similar primal-dual method the blossom algorithm (https://pypi.org/project/mwpf/)
    'mw_parity_factor': MwpfDecoder(),
}

BUILT_IN_SAMPLERS: Dict[str, Union[Decoder, Sampler]] = {
    **BUILT_IN_DECODERS,
    'perfectionist': PerfectionistSampler(),
}
