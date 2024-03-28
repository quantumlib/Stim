from typing import Dict

from sinter._sampling_sampler_class import Sampler
from sinter._sampling_stim import StimDetectorSampler
from sinter._sampling_vacuous import VacuousSampler

BUILT_IN_SAMPLERS: Dict[str, Sampler] = {
    "stim": StimDetectorSampler(),
    "vacuous": VacuousSampler(),
}
