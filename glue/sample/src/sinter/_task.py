from typing import Any, Dict, Optional, TYPE_CHECKING

import hashlib
import json
import math
import numpy as np

from sinter._collection_options import CollectionOptions
from sinter._json_type import JSON_TYPE

if TYPE_CHECKING:
    import sinter
    import stim


class Task:
    """A decoding problem that can be sampled from."""

    def __init__(
        self,
        *,
        circuit: 'stim.Circuit',
        decoder: Optional[str] = None,
        detector_error_model: Optional['stim.DetectorErrorModel'] = None,
        postselection_mask: Optional[np.ndarray] = None,
        json_metadata: JSON_TYPE = None,
        collection_options: 'sinter.CollectionOptions' = CollectionOptions(),
        skip_validation: bool = False,
        _unvalidated_strong_id: Optional[str] = None,
    ) -> None:
        """
        Args:
            circuit: The annotated noisy circuit to sample detection event data
                and logical observable data form.
            decoder: The decoder to use to predict the logical observable data
                from the detection event data. This can be set to None if it
                will be specified later (e.g. by the call to `collect`).
            detector_error_model: Specifies the error model to give to the decoder.
                Defaults to None, indicating that it should be automatically derived
                using `stim.Circuit.detector_error_model`.
            postselection_mask: Defaults to None (unused). A bit packed bitmask
                identifying detectors that must not fire. Shots where the
                indicated detectors fire are discarded.
            json_metadata: Defaults to None. Custom additional data describing
                the problem. Must be JSON serializable. For example, this could
                be a dictionary with "physical_error_rate" and "code_distance"
                keys.
            collection_options: Specifies custom options for collecting this
                single task. These options are merged with the global options
                to determine what happens.

                For example, if a task has `collection_options` set to
                `sinter.CollectionOptions(max_shots=1000, max_errors=100)` and
                `sinter.collect` was called with `max_shots=500` and
                `max_errors=200`, then either 500 shots or 100 errors will be
                collected for the task (whichever comes first).
            skip_validation: Defaults to False. Normally the arguments given to
                this method are checked for consistency (e.g. the detector error
                model should have the same number of detectors as the circuit).
                Setting this argument to True will skip doing the consistency
                checks. Note that this can result in confusing errors later, if
                the arguments are not actually consistent.
            _unvalidated_strong_id: Must be set to None unless `skip_validation`
                is set to True. Otherwise, if this is specified then it should
                be equal to the value returned by self.strong_id().
        """
        if not skip_validation:
            if _unvalidated_strong_id is not None:
                raise ValueError("_unvalidated_strong_id is not None and not skip_validation")
            dem = detector_error_model
            if dem is not None:
                if circuit.num_detectors != dem.num_detectors:
                    raise ValueError(f"circuit.num_detectors={circuit.num_detectors!r} != detector_error_model.num_detectors={dem.num_detectors!r}")
                if circuit.num_observables != dem.num_observables:
                    raise ValueError(f"circuit.num_observables={circuit.num_observables!r} != detector_error_model.num_observables={dem.num_observables!r}")
            mask = postselection_mask
            if mask is not None:
                if not isinstance(mask, np.ndarray):
                    raise ValueError(f"not isinstance(postselection_mask={mask!r}, np.ndarray)")
                if mask.dtype != np.uint8:
                    raise ValueError(f"postselection_mask.dtype={mask.dtype!r} != np.uint8")
                shape = (math.ceil(circuit.num_detectors / 8),)
                if mask.shape != shape:
                    raise ValueError(f"postselection_mask.shape={mask.shape!r} != (math.ceil(circuit.num_detectors / 8),)={shape!r}")
        self.circuit = circuit
        self.decoder = decoder
        self.detector_error_model = detector_error_model
        self.postselection_mask = postselection_mask
        self.json_metadata = json_metadata
        self.collection_options = collection_options
        self._unvalidated_strong_id = _unvalidated_strong_id

    def strong_id_value(self) -> Dict[str, Any]:
        """Contains all raw values that affect the strong id.

        This value is converted into the actual strong id by:
            - Serializing it into text using JSON.
            - Serializing the JSON text into bytes using UTF8.
            - Hashing the UTF8 bytes using SHA256.
        """
        if self.decoder is None:
            raise ValueError("Can't compute strong_id until `decoder` is set.")
        if self.detector_error_model is None:
            raise ValueError("Can't compute strong_id until `detector_error_model` is set.")
        return {
            'circuit': str(self.circuit),
            'decoder': self.decoder,
            'decoder_error_model': str(self.detector_error_model),
            'postselection_mask':
                None
                if self.postselection_mask is None
                else [int(e) for e in self.postselection_mask],
            'json_metadata': self.json_metadata,
        }

    def strong_id_text(self) -> str:
        """The text that is serialized and hashed to get the strong id.

        This value is converted into the actual strong id by:
            - Serializing into bytes using UTF8.
            - Hashing the UTF8 bytes using SHA256.
        """
        return json.dumps(self.strong_id_value())

    def strong_id_bytes(self) -> bytes:
        """The bytes that are hashed to get the strong id.

        This value is converted into the actual strong id by:
            - Hashing these bytes using SHA256.
        """
        return self.strong_id_text().encode('utf8')

    def _recomputed_strong_id(self) -> str:
        return hashlib.sha256(self.strong_id_bytes()).hexdigest()

    def strong_id(self) -> str:
        """A cryptographically unique identifier for this task.

        This value is affected by:
            - The exact circuit.
            - The exact detector error model.
            - The decoder.
            - The json metadata.
            - The postselection mask.
        """
        if self._unvalidated_strong_id is None:
            self._unvalidated_strong_id = self._recomputed_strong_id()
        return self._unvalidated_strong_id
