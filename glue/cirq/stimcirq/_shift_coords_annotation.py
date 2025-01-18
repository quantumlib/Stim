from typing import Any, Dict, Iterable, List, Tuple

import cirq
import stim


@cirq.value_equality
class ShiftCoordsAnnotation(cirq.Operation):
    """Annotates that the qubit/detector coordinate origin is being moved.

    Creates a SHIFT_COORDS operation when converting to a stim circuit.
    """

    def __init__(self, shift: Iterable[float]):
        """

        Args:
            shift: How much to shift each coordinate..
        """
        self.shift = tuple(shift)

    @property
    def qubits(self) -> Tuple[cirq.Qid, ...]:
        return ()

    def with_qubits(self, *new_qubits) -> 'ShiftCoordsAnnotation':
        return self

    def _value_equality_values_(self) -> Any:
        return self.shift

    def _circuit_diagram_info_(self, args: Any) -> str:
        k = ",".join(repr(e) for e in self.shift)
        return f"ShiftCoords({k})"

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {'shift': self.shift}

    def __repr__(self) -> str:
        return f'stimcirq.ShiftCoordsAnnotation({self.shift!r})'

    def _decompose_(self):
        return []

    def _is_comment_(self) -> bool:
        return True

    def _stim_conversion_(self, *, edit_circuit: stim.Circuit, tag: str, **kwargs):
        edit_circuit.append_operation("SHIFT_COORDS", [], self.shift, tag=tag)
