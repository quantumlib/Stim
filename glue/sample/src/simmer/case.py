import dataclasses
import pathlib
from typing import Optional

import numpy as np

import stim
from simmer.case_stats import CaseStats
from simmer.decoding import sample_decode


@dataclasses.dataclass(frozen=True)
class Case:
    # Fields included in CSV data.
    name: str
    strong_id: str
    decoder: str
    num_shots: int

    # Fields not included in CSV data.
    circuit: stim.Circuit
    dem: stim.DetectorErrorModel
    post_mask: Optional[np.ndarray]

    def run(self, *, tmp_dir: pathlib.Path) -> CaseStats:
        return sample_decode(
            num_shots=self.num_shots,
            circuit=self.circuit,
            post_mask=self.post_mask,
            decoder_error_model=self.dem,
            decoder=self.decoder,
            tmp_dir=tmp_dir,
        )
