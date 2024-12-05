from typing import Any, Dict, List

import cirq
import stim


@cirq.value_equality
class CXSwapGate(cirq.Gate):
    """Handles explaining stim's CXSWAP and SWAPCX gates to cirq."""

    def __init__(
        self,
        *,
        inverted: bool,
    ):
        self.inverted = inverted

    def _num_qubits_(self) -> int:
        return 2

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs) -> List[str]:
        return ['ZSWAP', 'XSWAP'][::-1 if self.inverted else +1]

    def _value_equality_values_(self):
        return (
            self.inverted,
        )

    def _decompose_(self, qubits):
        a, b = qubits
        if self.inverted:
            yield cirq.SWAP(a, b)
            yield cirq.CNOT(a, b)
        else:
            yield cirq.CNOT(a, b)
            yield cirq.SWAP(a, b)

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int], tag: str, **kwargs):
        edit_circuit.append('SWAPCX' if self.inverted else 'CXSWAP', targets, tag=tag)

    def __pow__(self, power: int) -> 'CXSwapGate':
        if power == +1:
            return self
        if power == -1:
            return CXSwapGate(inverted=not self.inverted)
        return NotImplemented

    def __str__(self) -> str:
        return 'SWAPCX' if self.inverted else 'CXSWAP'

    def __repr__(self):
        return f'stimcirq.CXSwapGate(inverted={self.inverted!r})'

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {
            'inverted': self.inverted,
        }
