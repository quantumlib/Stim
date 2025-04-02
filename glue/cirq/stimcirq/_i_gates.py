from collections.abc import Iterable
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


@cirq.value_equality
class IIErrorGate(cirq.Gate):
    """Handles explaining stim's II_ERROR gate to cirq."""

    @staticmethod
    def from_args(args_list) -> 'IIErrorGate | IIGate':
        """given the args list, return the appropriate cirq gate."""
        if len(args_list) == 1:
            return IIGate.with_probability(args_list[0])
        return IIErrorGate(args_list)

    def __init__(self, gate_args: Iterable[float]):
        self.gate_args = tuple(gate_args)

    def _num_qubits_(self) -> int:
        return 2

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs) -> List[str]:
        if self.gate_args:
            return [f'II_ERROR({self.gate_args})', f'II_ERROR({self.gate_args})']
        return ['II_ERROR', 'II_ERROR']

    def _value_equality_values_(self):
        return self.gate_args

    def _decompose_(self, qubits):
        pass

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int], tag: str, **kwargs):
        edit_circuit.append('II_ERROR', targets, arg=self.gate_args, tag=tag)

    def __pow__(self, power: int) -> 'IIGate':
        return self

    def __str__(self) -> str:
        return 'II_ERROR'

    def __repr__(self):
        return f'stimcirq.IIErrorGate()'

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {'gate_args': self.gate_args}


@cirq.value_equality
class IErrorGate(cirq.Gate):
    """Handles explaining stim's I_ERROR gate to cirq."""

    @staticmethod
    def from_args(cls, args_list) -> 'IIErrorGate | IIGate':
        """given the args list, return the appropriate cirq gate."""
        if len(args_list) == 1:
            return cirq.I.with_probability(args_list[0])
        return IErrorGate(args_list)

    def __init__(self, gate_args: Iterable[float]):
        self.gate_args = tuple(gate_args)

    def _num_qubits_(self) -> int:
        return 1

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs) -> List[
        str]:
        if self.gate_args:
            return [f'I_ERROR({self.gate_args})']
        return ['I_ERROR']

    def _value_equality_values_(self):
        return self.gate_args

    def _decompose_(self, qubits):
        pass

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int],
        tag: str, **kwargs):
        edit_circuit.append('I_ERROR', targets, arg=self.gate_args, tag=tag)

    def __pow__(self, power: int) -> 'IIGate':
        return self

    def __str__(self) -> str:
        return 'I_ERROR'

    def __repr__(self):
        return f'stimcirq.IErrorGate()'

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {}
