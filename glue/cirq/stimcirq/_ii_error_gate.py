from typing import Any, Dict, List, Iterable

import cirq
import stim



@cirq.value_equality
class IIErrorGate(cirq.Gate):
    """Handles explaining stim's II_ERROR gate to cirq."""

    def __init__(self, gate_args: Iterable[float]):
        self.gate_args = tuple(gate_args)

    def _num_qubits_(self) -> int:
        return 2

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs) -> List[str]:
        return [f'II_ERROR({self.gate_args})', f'II_ERROR({self.gate_args})']

    def _value_equality_values_(self):
        return self.gate_args

    def _decompose_(self, qubits):
        pass

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int], tag: str, **kwargs):
        edit_circuit.append('II_ERROR', targets, arg=self.gate_args, tag=tag)

    def __str__(self) -> str:
        return 'II_ERROR(' + ",".join(str(e) for e in self.gate_args) + ')'

    def __repr__(self):
        return f'stimcirq.IIErrorGate({self.gate_args!r})'

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {'gate_args': self.gate_args}
