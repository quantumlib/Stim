from typing import Any, Dict, List, Tuple, Optional

import cirq
import stim


@cirq.value_equality
class FeedbackPauli(cirq.Gate):
    """A Pauli gate conditioned on a prior measurement."""

    def __init__(
        self,
        *,
        relative_measurement_index: Optional[int] = None,
        pauli: cirq.Pauli,
    ):
        r"""

        Args:
            relative_measurement_index: A negative integer identifying how many measurements ago is the measurement that
                controls the Pauli operation.
            pauli: The cirq Pauli operation to apply when the bit is True.
        """
        if relative_measurement_index is not None and (relative_measurement_index >= 0 or not isinstance(relative_measurement_index, int)):
            raise ValueError(f"{relative_measurement_index=} isn't a negative int (note {type(relative_measurement_index)=})")
        self.relative_measurement_index = relative_measurement_index
        self.pauli = pauli

    def _is_parameterized_(self) -> bool:
        return False

    def _num_qubits_(self) -> int:
        return 1

    def _value_equality_values_(self) -> Any:
        return self.pauli, self.relative_measurement_index

    def _circuit_diagram_info_(self, args: Any) -> str:
        return f"{self.pauli}^rec[{self.relative_measurement_index}]"

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {
            'pauli': self.pauli,
            'relative_measurement_index': self.relative_measurement_index,
        }

    def __repr__(self) -> str:
        return (
            f'stimcirq.FeedbackPauli('
            f'relative_measurement_index={self.relative_measurement_index!r}, '
            f'pauli={self.pauli!r})'
        )

    def _stim_conversion_(
            self,
            *,
            edit_circuit: stim.Circuit,
            tag: str,
            targets: List[int],
            **kwargs,
    ):
        rec_target = stim.target_rec(self.relative_measurement_index)
        edit_circuit.append(f"C{self.pauli}", [rec_target, targets[0]], tag=tag)
