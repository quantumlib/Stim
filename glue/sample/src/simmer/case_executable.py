import pathlib
import hashlib
import json
import numpy as np
from typing import Optional

import stim

from simmer.case_stats import CaseStats
from simmer.case_summary import JSON_TYPE, CaseSummary


class CaseExecutable:
    """Complete specification of an executable decoding problem."""

    def __init__(self, *,
                 circuit: stim.Circuit,
                 decoder: str,
                 decoder_error_model: Optional[stim.DetectorErrorModel] = None,
                 postselection_mask: Optional[np.ndarray] = None,
                 custom: JSON_TYPE = None) -> None:
        if decoder_error_model is None:
            decoder_error_model = circuit.detector_error_model(decompose_errors=True)
        else:
            n1 = circuit.num_detectors + circuit.num_observables
            n2 = decoder_error_model.num_detectors + decoder_error_model.num_observables
            assert n1 == n2
            if postselection_mask is not None:
                assert postselection_mask.dtype == np.uint8
                assert postselection_mask.shape == ((n1 + 7) // 8,)
        self.circuit = circuit
        self.decoder_error_model = decoder_error_model
        self.decoder = decoder
        self.postselection_mask = postselection_mask
        self.custom = custom

    def to_summary(self) -> CaseSummary:
        return CaseSummary(
            decoder=self.decoder,
            custom=self.custom,
            strong_id=self.to_strong_id(),
        )

    def to_strong_id(self) -> str:
        input_text = json.dumps({
            'circuit': str(self.circuit),
            'decoder': self.decoder,
            'decoder_error_model': str(self.decoder_error_model),
            'postselection_mask':
                None
                if self.postselection_mask is None
                else list(self.postselection_mask),
            'custom': self.custom,
        })
        return hashlib.sha256(input_text.encode('utf8')).hexdigest()

    def sample_stats(self,
                     *,
                     num_shots: int,
                     tmp_dir: Optional[pathlib.Path]) -> CaseStats:
        from simmer.decoding import sample_decode
        return sample_decode(
            num_shots=num_shots,
            circuit=self.circuit,
            post_mask=self.postselection_mask,
            decoder_error_model=self.decoder_error_model,
            decoder=self.decoder,
            tmp_dir=tmp_dir,
        )
