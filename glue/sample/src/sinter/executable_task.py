import pathlib
import hashlib
import json
import numpy as np
from typing import Any
from typing import Dict
from typing import Optional

import stim

from sinter.anon_task_stats import AnonTaskStats
from sinter.task_summary import JSON_TYPE, TaskSummary


class ExecutableTask:
    """Complete specification of an executable decoding problem."""

    def __init__(self, *,
                 circuit: stim.Circuit,
                 decoder: str,
                 error_model_for_decoder: Optional[stim.DetectorErrorModel] = None,
                 postselection_mask: Optional[np.ndarray] = None,
                 json_metadata: JSON_TYPE = None) -> None:
        if error_model_for_decoder is None:
            error_model_for_decoder = circuit.detector_error_model(decompose_errors=True)
        else:
            n1 = circuit.num_detectors + circuit.num_observables
            n2 = error_model_for_decoder.num_detectors + error_model_for_decoder.num_observables
            assert n1 == n2
            if postselection_mask is not None:
                assert postselection_mask.dtype == np.uint8
                assert postselection_mask.shape == ((n1 + 7) // 8,)
        self.circuit = circuit
        self.decoder_error_model = error_model_for_decoder
        self.decoder = decoder
        self.postselection_mask = postselection_mask
        self.json_metadata = json_metadata

    def to_summary(self) -> TaskSummary:
        return TaskSummary(
            decoder=self.decoder,
            json_metadata=self.json_metadata,
            strong_id=self.strong_id(),
        )

    def strong_id_value(self) -> Dict[str, Any]:
        """Contains all data that affects the strong id for this case.

        This value is converted into the actual strong id by:
            - Serializing it into text using JSON.
            - Serializing the JSON text into bytes using UTF8.
            - Hashing the UTF8 bytes using SHA256.
        """
        return {
            'circuit': str(self.circuit),
            'decoder': self.decoder,
            'decoder_error_model': str(self.decoder_error_model),
            'postselection_mask':
                None
                if self.postselection_mask is None
                else list(self.postselection_mask),
            'json_metadata': self.json_metadata,
        }

    def strong_id_text(self) -> str:
        return json.dumps(self.strong_id_value())

    def strong_id_bytes(self) -> bytes:
        return self.strong_id_text().encode('utf8')

    def strong_id(self) -> str:
        """A cryptographically unique identifier for this task."""
        return hashlib.sha256(self.strong_id_bytes()).hexdigest()

    def sample_stats(self,
                     *,
                     num_shots: int,
                     tmp_dir: Optional[pathlib.Path]) -> AnonTaskStats:
        from sinter.decoding import sample_decode
        return sample_decode(
            num_shots=num_shots,
            circuit=self.circuit,
            post_mask=self.postselection_mask,
            decoder_error_model=self.decoder_error_model,
            decoder=self.decoder,
            tmp_dir=tmp_dir,
        )
