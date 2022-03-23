import dataclasses
import functools
import hashlib
import json
import numpy as np
from typing import Union, Dict, List, Optional

import stim

JSON_TYPE = Union[Dict[str, 'JSON_TYPE'], List['JSON_TYPE'], str, int, float]


@dataclasses.dataclass(frozen=True)
class CaseSummary:
    decoder: str
    custom: JSON_TYPE
    strong_id: str


@dataclasses.dataclass(frozen=True)
class CaseStats:
    num_shots: int = 0
    num_errors: int = 0
    num_discards: int = 0
    seconds_elapsed: float = 0

    def __post_init__(self):
        assert isinstance(self.num_errors, int)
        assert isinstance(self.num_shots, int)
        assert isinstance(self.num_discards, int)
        assert isinstance(self.seconds_elapsed, (int, float))
        assert self.num_errors >= 0
        assert self.num_discards >= 0
        assert self.seconds_elapsed >= 0
        assert self.num_shots >= self.num_errors + self.num_discards

    def __add__(self, other: 'CaseStats') -> 'CaseStats':
        if not isinstance(other, CaseStats):
            return NotImplemented
        return CaseStats(
            num_shots=self.num_shots + other.num_shots,
            num_errors=self.num_errors + other.num_errors,
            num_discards=self.num_discards + other.num_discards,
            seconds_elapsed=self.seconds_elapsed + other.seconds_elapsed,
        )


@dataclasses.dataclass(frozen=True)
class Case:
    circuit: stim.Circuit
    decoder: str
    decoder_error_model: stim.DetectorErrorModel
    postselection_mask: Optional[np.ndarray]
    custom: JSON_TYPE

    def __post_init__(self):
        n1 = self.circuit.num_detectors + self.circuit.num_observables
        n2 = self.decoder_error_model.num_detectors + self.decoder_error_model.num_observables
        assert n1 == n2
        assert self.postselection_mask.dtype == np.uint8
        assert self.postselection_mask.shape == ((n1 + 7) // 8,)

    @functools.cached_property
    def summary(self) -> CaseSummary:
        return CaseSummary(
            decoder=self.decoder,
            custom=self.custom,
            strong_id=self.strong_id,
        )

    @functools.cached_property
    def strong_id(self) -> str:
        input_text = json.dumps({
            'circuit': str(self.circuit),
            'decoder': self.decoder,
            'decoder_error_model': str(self.decoder_error_model),
            'postselection_mask': list(self.postselection_mask),
            'custom': self.custom,
        })
        return hashlib.sha256(input_text.encode('utf8')).hexdigest()

    def sample_stats(self, *, num_shots: int) -> CaseStats:
        from simmer.decoding import sample_decode
        return sample_decode(
            num_shots=num_shots,
            circuit=self.circuit,
            post_mask=self.postselection_mask,
            decoder_error_model=self.decoder_error_model,
            decoder=self.decoder,
        )


@dataclasses.dataclass(frozen=True)
class CaseGoal:
    case: Case
    max_shots: int
    max_errors: int
