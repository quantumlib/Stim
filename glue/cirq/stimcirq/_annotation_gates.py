from typing import Any, Dict, List, Sequence

import cirq


@cirq.value_equality
class _AnnotationGate(cirq.Gate):
    """Base class for the gate form of a stimcirq annotation operation.

    Stimcirq annotations (e.g. `stimcirq.DetAnnotation`) are `cirq.Operation`s
    that don't correspond to any physical action. Each annotation's `gate`
    property returns an instance of a subclass of this class, wrapping the
    annotation. This allows annotations to be matched by `cirq.GateFamily` and
    `cirq.Gateset` (e.g. for device validation), similar to how
    `cirq.GlobalPhaseGate` relates to global phase operations.

    Applying the gate to qubits (via `on`) returns the wrapped annotation
    targeting those qubits.
    """

    def __init__(self, annotation: cirq.Operation):
        self._annotation = annotation

    @property
    def annotation(self) -> cirq.Operation:
        """The annotation operation this gate is the gate form of."""
        return self._annotation

    def _num_qubits_(self) -> int:
        return len(self._annotation.qubits)

    def on(self, *qubits: cirq.Qid) -> cirq.Operation:
        if len(qubits) != len(self._annotation.qubits):
            raise ValueError(
                f"{self!r} acts on {len(self._annotation.qubits)} qubits, but got {qubits!r}."
            )
        return self._annotation.with_qubits(*qubits)

    def _value_equality_values_(self) -> Any:
        return self._annotation

    def _decompose_(self, qubits: Sequence[cirq.Qid]) -> List[cirq.Operation]:
        return []

    def _is_comment_(self) -> bool:
        return True

    def _circuit_diagram_info_(self, args: Any) -> Any:
        return cirq.circuit_diagram_info(self._annotation, args, default=NotImplemented)

    def __repr__(self) -> str:
        return f'stimcirq.{type(self).__name__}({self._annotation!r})'

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {'annotation': self._annotation}

    @classmethod
    def _from_json_dict_(cls, annotation: cirq.Operation, **kwargs) -> '_AnnotationGate':
        return cls(annotation)


class DetAnnotationGate(_AnnotationGate):
    """The gate form of a `stimcirq.DetAnnotation`.

    Returned by `stimcirq.DetAnnotation(...).gate`.
    """


class CumulativeObservableAnnotationGate(_AnnotationGate):
    """The gate form of a `stimcirq.CumulativeObservableAnnotation`.

    Returned by `stimcirq.CumulativeObservableAnnotation(...).gate`.
    """


class ShiftCoordsAnnotationGate(_AnnotationGate):
    """The gate form of a `stimcirq.ShiftCoordsAnnotation`.

    Returned by `stimcirq.ShiftCoordsAnnotation(...).gate`.
    """
