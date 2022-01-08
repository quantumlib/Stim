from typing import Any, Dict, Iterable, List, Tuple

import cirq
import stim


@cirq.value_equality
class DetAnnotation(cirq.Operation):
    """Annotates that a particular combination of measurements is deterministic.

    Creates a DETECTOR operation when converting to a stim circuit.
    """

    def __init__(self, *parity_keys: str, coordinate_metadata: Iterable[float] = ()):
        """

        Args:
            parity_keys: The keys of some measurements with the property that their parity is always
                the same under noiseless execution of the circuit.
            coordinate_metadata: An optional location for the detector. This has no effect on the
                function of the circuit, but can be used by plotting tools.
        """
        self.parity_keys = frozenset(parity_keys)
        self.coordinate_metadata = tuple(coordinate_metadata)

    @property
    def qubits(self) -> Tuple[cirq.Qid, ...]:
        return ()

    def with_qubits(self, *new_qubits) -> 'DetAnnotation':
        return self

    def _value_equality_values_(self) -> Any:
        return self.parity_keys, self.coordinate_metadata

    def _circuit_diagram_info_(self, args: Any) -> str:
        k = ",".join(repr(e) for e in sorted(self.parity_keys))
        return f"Det({k})"

    @classmethod
    def _from_json_helper(
        cls, parity_keys: List[str], coordinate_metadata: List[float]
    ) -> 'DetAnnotation':
        return DetAnnotation(*parity_keys, coordinate_metadata=coordinate_metadata)

    def _json_dict_(self) -> Dict[str, Any]:
        return {
            'cirq_type': self.__class__.__name__,
            'parity_keys': sorted(self.parity_keys),
            'coordinate_metadata': self.coordinate_metadata,
        }

    def __repr__(self) -> str:
        k = ", ".join(repr(e) for e in sorted(self.parity_keys))
        return f'stimcirq.DetAnnotation({k}, coordinate_metadata={self.coordinate_metadata!r})'

    def _decompose_(self):
        return []

    def _is_comment_(self) -> bool:
        return True

    def _stim_conversion_(
        self,
        edit_circuit: stim.Circuit,
        edit_measurement_key_lengths: List[Tuple[str, int]],
        **kwargs,
    ):
        # Ideally these references would all be resolved ahead of time, to avoid the redundant
        # linear search overhead and also to avoid the detectors and measurements being interleaved
        # instead of grouped (grouping measurements is helpful for stabilizer simulation). But that
        # didn't happen and this is the context we're called in and we're going to make it work.

        # Find indices of measurement record targets.
        remaining = set(self.parity_keys)
        rec_targets = []
        for offset in range(len(edit_measurement_key_lengths)):
            m_key, m_len = edit_measurement_key_lengths[-1 - offset]
            if m_len != 1:
                raise NotImplementedError(f"multi-qubit measurement {m_key!r}")
            if m_key in remaining:
                remaining.discard(m_key)
                rec_targets.append(stim.target_rec(-1 - offset))
                if not remaining:
                    break
        if remaining:
            raise ValueError(
                f"{self!r} was processed before measurements it referenced ({sorted(remaining)!r})."
                f" Make sure the referenced measurements keys are actually in the circuit, and come"
                f" in an earlier moment (or earlier in the same moment's operation order)."
            )

        edit_circuit.append_operation("DETECTOR", rec_targets, self.coordinate_metadata)
