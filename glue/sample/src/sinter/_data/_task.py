import pathlib
from typing import Any, Dict, Optional, TYPE_CHECKING

import hashlib
import json
import math
from typing import Union

import numpy as np

from sinter._data._collection_options import CollectionOptions

if TYPE_CHECKING:
    import sinter
    import stim


class Task:
    """A decoding problem that sinter can sample from.

    Attributes:
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
        postselected_observables_mask: Defaults to None (unused). A bit
            packed bitmask identifying observable indices to postselect on.
            Anytime the decoder's predicted flip for one of these
            observables doesn't agree with the actual measured flip value of
            the observable, the shot is discarded instead of counting as an
            error.
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

    Examples:
        >>> import sinter
        >>> import stim
        >>> task = sinter.Task(
        ...     circuit=stim.Circuit.generated(
        ...         'repetition_code:memory',
        ...         rounds=10,
        ...         distance=10,
        ...         before_round_data_depolarization=1e-3,
        ...     ),
        ... )
    """

    def __init__(
        self,
        *,
        circuit: Optional['stim.Circuit'] = None,
        decoder: Optional[str] = None,
        detector_error_model: Optional['stim.DetectorErrorModel'] = None,
        postselection_mask: Optional[np.ndarray] = None,
        postselected_observables_mask: Optional[np.ndarray] = None,
        json_metadata: Any = None,
        collection_options: 'sinter.CollectionOptions' = CollectionOptions(),
        skip_validation: bool = False,
        circuit_path: Optional[Union[str, pathlib.Path]] = None,
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
            postselected_observables_mask: Defaults to None (unused). A bit
                packed bitmask identifying observable indices to postselect on.
                Anytime the decoder's predicted flip for one of these
                observables doesn't agree with the actual measured flip value of
                the observable, the shot is discarded instead of counting as an
                error.
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
            circuit_path: Typically set to None. If the circuit isn't specified,
                this is the filepath to read it from. Not included in the strong
                id.
            _unvalidated_strong_id: Must be set to None unless `skip_validation`
                is set to True. Otherwise, if this is specified then it should
                be equal to the value returned by self.strong_id().
        """
        if not skip_validation:
            if circuit_path is None and circuit is None:
                raise ValueError('circuit_path is None and circuit is None')
            if _unvalidated_strong_id is not None:
                raise ValueError("_unvalidated_strong_id is not None and not skip_validation")
            dem = detector_error_model
            if circuit is not None:
                num_dets = circuit.num_detectors
                num_obs = circuit.num_observables
                if dem is not None:
                    if circuit.num_detectors != dem.num_detectors:
                        raise ValueError(f"circuit.num_detectors={num_dets!r} != detector_error_model.num_detectors={dem.num_detectors!r}")
                    if num_obs != dem.num_observables:
                        raise ValueError(f"circuit.num_observables={num_obs!r} != detector_error_model.num_observables={dem.num_observables!r}")
                if postselection_mask is not None:
                    shape = (math.ceil(num_dets / 8),)
                    if postselection_mask.shape != shape:
                        raise ValueError(f"postselection_mask.shape={postselection_mask.shape!r} != (math.ceil(circuit.num_detectors / 8),)={shape!r}")
                if postselected_observables_mask is not None:
                    shape = (math.ceil(num_obs / 8),)
                    if postselected_observables_mask.shape != shape:
                        raise ValueError(f"postselected_observables_mask.shape={postselected_observables_mask.shape!r} != (math.ceil(circuit.num_observables / 8),)={shape!r}")
            if postselection_mask is not None:
                if not isinstance(postselection_mask, np.ndarray):
                    raise ValueError(f"not isinstance(postselection_mask={postselection_mask!r}, np.ndarray)")
                if postselection_mask.dtype != np.uint8:
                    raise ValueError(f"postselection_mask.dtype={postselection_mask.dtype!r} != np.uint8")
            if postselected_observables_mask is not None:
                if not isinstance(postselected_observables_mask, np.ndarray):
                    raise ValueError(f"not isinstance(postselected_observables_mask={postselected_observables_mask!r}, np.ndarray)")
                if postselected_observables_mask.dtype != np.uint8:
                    raise ValueError(f"postselected_observables_mask.dtype={postselected_observables_mask.dtype!r} != np.uint8")
        self.circuit_path = None if circuit_path is None else pathlib.Path(circuit_path)
        self.circuit = circuit
        self.decoder = decoder
        self.detector_error_model = detector_error_model
        self.postselection_mask = postselection_mask
        self.postselected_observables_mask = postselected_observables_mask
        self.json_metadata = json_metadata
        self.collection_options = collection_options
        self._unvalidated_strong_id = _unvalidated_strong_id

    def strong_id_value(self) -> Dict[str, Any]:
        """Contains all raw values that affect the strong id.

        This value is converted into the actual strong id by:
            - Serializing it into text using JSON.
            - Serializing the JSON text into bytes using UTF8.
            - Hashing the UTF8 bytes using SHA256.

        Examples:
            >>> import sinter
            >>> import stim
            >>> task = sinter.Task(
            ...     circuit=stim.Circuit('H 0'),
            ...     detector_error_model=stim.DetectorErrorModel(),
            ...     decoder='pymatching',
            ... )
            >>> task.strong_id_value()
            {'circuit': 'H 0', 'decoder': 'pymatching', 'decoder_error_model': '', 'postselection_mask': None, 'json_metadata': None}
        """
        if self.circuit is None:
            raise ValueError("Can't compute strong_id until `circuit` is set.")
        if self.decoder is None:
            raise ValueError("Can't compute strong_id until `decoder` is set.")
        if self.detector_error_model is None:
            raise ValueError("Can't compute strong_id until `detector_error_model` is set.")
        result = {
            'circuit': str(self.circuit),
            'decoder': self.decoder,
            'decoder_error_model': str(self.detector_error_model),
            'postselection_mask':
                None
                if self.postselection_mask is None
                else [int(e) for e in self.postselection_mask],
            'json_metadata': self.json_metadata,
        }
        if self.postselected_observables_mask is not None:
            result['postselected_observables_mask'] = [int(e) for e in self.postselected_observables_mask]
        return result

    def strong_id_text(self) -> str:
        """The text that is serialized and hashed to get the strong id.

        This value is converted into the actual strong id by:
            - Serializing into bytes using UTF8.
            - Hashing the UTF8 bytes using SHA256.

        Examples:
            >>> import sinter
            >>> import stim
            >>> task = sinter.Task(
            ...     circuit=stim.Circuit('H 0'),
            ...     detector_error_model=stim.DetectorErrorModel(),
            ...     decoder='pymatching',
            ... )
            >>> task.strong_id_text()
            '{"circuit": "H 0", "decoder": "pymatching", "decoder_error_model": "", "postselection_mask": null, "json_metadata": null}'
        """
        return json.dumps(self.strong_id_value())

    def strong_id_bytes(self) -> bytes:
        """The bytes that are hashed to get the strong id.

        This value is converted into the actual strong id by:
            - Hashing these bytes using SHA256.

        Examples:
            >>> import sinter
            >>> import stim
            >>> task = sinter.Task(
            ...     circuit=stim.Circuit('H 0'),
            ...     detector_error_model=stim.DetectorErrorModel(),
            ...     decoder='pymatching',
            ... )
            >>> task.strong_id_bytes()
            b'{"circuit": "H 0", "decoder": "pymatching", "decoder_error_model": "", "postselection_mask": null, "json_metadata": null}'
        """
        return self.strong_id_text().encode('utf8')

    def _recomputed_strong_id(self) -> str:
        return hashlib.sha256(self.strong_id_bytes()).hexdigest()

    def strong_id(self) -> str:
        """Computes a cryptographically unique identifier for this task.

        This value is affected by:
            - The exact circuit.
            - The exact detector error model.
            - The decoder.
            - The json metadata.
            - The postselection mask.

        Examples:
            >>> import sinter
            >>> import stim
            >>> task = sinter.Task(
            ...     circuit=stim.Circuit(),
            ...     detector_error_model=stim.DetectorErrorModel(),
            ...     decoder='pymatching',
            ... )
            >>> task.strong_id()
            '7424ea021693d4abc1c31c12e655a48779f61a7c2969e457ae4fe400c852bee5'
        """
        if self._unvalidated_strong_id is None:
            self._unvalidated_strong_id = self._recomputed_strong_id()
        return self._unvalidated_strong_id

    def __repr__(self) -> str:
        terms = []
        if self.circuit is not None:
            terms.append(f'circuit={self.circuit!r}')
        if self.decoder is not None:
            terms.append(f'decoder={self.decoder!r}')
        if self.detector_error_model is not None:
            terms.append(f'detector_error_model={self.detector_error_model!r}')
        if self.postselection_mask is not None:
            nd = self.circuit.num_detectors
            bits = list(np.unpackbits(self.postselection_mask, count=nd, bitorder='little'))
            terms.append(f'''postselection_mask=np.packbits({bits!r}, bitorder='little')''')
        if self.postselected_observables_mask is not None:
            no = self.circuit.num_observables
            bits = list(np.unpackbits(self.postselected_observables_mask, count=no, bitorder='little'))
            terms.append(f'''postselected_observables_mask=np.packbits({bits!r}, bitorder='little')''')
        if self.json_metadata is not None:
            terms.append(f'json_metadata={self.json_metadata!r}')
        if self.collection_options != CollectionOptions():
            terms.append(f'collection_options={self.collection_options!r}')
        if self.circuit_path is not None:
            terms.append(f'circuit_path={self.circuit_path!r}')
        return f'sinter.Task({", ".join(terms)})'

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, Task):
            return NotImplemented
        if self._unvalidated_strong_id is not None and other._unvalidated_strong_id is not None:
            return self._unvalidated_strong_id == other._unvalidated_strong_id
        return (
            self.circuit_path == other.circuit_path and
            self.circuit == other.circuit and
            self.decoder == other.decoder and
            self.detector_error_model == other.detector_error_model and
            np.array_equal(self.postselection_mask, other.postselection_mask) and
            np.array_equal(self.postselected_observables_mask, other.postselected_observables_mask) and
            self.json_metadata == other.json_metadata and
            self.collection_options == other.collection_options
        )
