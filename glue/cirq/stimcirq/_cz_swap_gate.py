from typing import Any, Dict, List

import cirq
import stim


@cirq.value_equality
class CZSwapGate(cirq.Gate):
    """Handles explaining stim's CZSWAP gates to cirq."""

    def _num_qubits_(self) -> int:
        return 2

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs) -> List[str]:
        return ['ZSWAP', 'ZSWAP']

    def _value_equality_values_(self):
        return ()

    def _decompose_(self, qubits):
        a, b = qubits
        yield cirq.SWAP(a, b)
        yield cirq.CZ(a, b)

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int], tag: str, **kwargs):
        edit_circuit.append('CZSWAP', targets, tag=tag)

    def __pow__(self, power: int) -> 'CZSwapGate':
        if power == +1:
            return self
        if power == -1:
            return self
        return NotImplemented

    def __str__(self) -> str:
        return 'CZSWAP'

    def __repr__(self):
        return f'stimcirq.CZSwapGate()'

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {}
