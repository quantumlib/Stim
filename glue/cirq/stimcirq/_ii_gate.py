from typing import Any, Dict, List

import cirq
import stim


@cirq.value_equality
class IIGate(cirq.Gate):
    """Handles explaining stim's II gate to cirq."""

    def _num_qubits_(self) -> int:
        return 2

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs) -> List[str]:
        return ['II', 'II']

    def _value_equality_values_(self):
        return ()

    def _decompose_(self, qubits):
        pass

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int], tag: str, **kwargs):
        edit_circuit.append('II', targets, tag=tag)

    def __pow__(self, power: int) -> 'IIGate':
        return self

    def __str__(self) -> str:
        return 'II'

    def __repr__(self):
        return f'stimcirq.IIGate()'

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {}
