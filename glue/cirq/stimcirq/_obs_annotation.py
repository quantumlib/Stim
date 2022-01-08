from typing import Any, Dict, List, Tuple

import cirq
import stim


@cirq.value_equality
class CumulativeObservableAnnotation(cirq.Operation):
    """Annotates that a particular combination of measurements is part of a logical observable.

    Creates an OBSERVABLE_INCLUDE operation when converting to a stim circuit.
    """

    def __init__(self, *parity_keys: str, observable_index: int):
        """

        Args:
            parity_keys: The keys of some measurements to include in the logical observable.
            observable_index: A unique index for the logical observable.
        """
        self.parity_keys = frozenset(parity_keys)
        self.observable_index = observable_index

    @property
    def qubits(self) -> Tuple[cirq.Qid, ...]:
        return ()

    def with_qubits(self, *new_qubits) -> 'CumulativeObservableAnnotation':
        return self

    def _value_equality_values_(self) -> Any:
        return self.parity_keys, self.observable_index

    def _circuit_diagram_info_(self, args: Any) -> str:
        k = ",".join(repr(e) for e in sorted(self.parity_keys))
        return f"Obs{self.observable_index}({k})"

    def __repr__(self) -> str:
        k = ", ".join(repr(e) for e in sorted(self.parity_keys))
        return f'stimcirq.CumulativeObservableAnnotation({k}, observable_index={self.observable_index!r})'

    @classmethod
    def _from_json_helper(
        cls, parity_keys: List[str], observable_index: int
    ) -> 'CumulativeObservableAnnotation':
        return CumulativeObservableAnnotation(*parity_keys, observable_index=observable_index)

    def _json_dict_(self) -> Dict[str, Any]:
        return {
            'cirq_type': self.__class__.__name__,
            'parity_keys': sorted(self.parity_keys),
            'observable_index': self.observable_index,
        }

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

        edit_circuit.append_operation("OBSERVABLE_INCLUDE", rec_targets, self.observable_index)
