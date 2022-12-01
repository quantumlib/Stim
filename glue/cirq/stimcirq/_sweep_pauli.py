from typing import AbstractSet, Any, Dict, List, Optional, Union

import cirq
import stim
import sympy


@cirq.value_equality
class SweepPauli(cirq.Gate):
    """A Pauli gate included depending on sweep data, with compatibility across stim and cirq.

    Includes a sweep bit index for stim and a sweep symbol for cirq.
    Creates sweep-controlled operations like `CX sweep[5] 0` when converting to a stim circuit.
    """

    def __init__(
        self,
        *,
        stim_sweep_bit_index: int,
        cirq_sweep_symbol: Optional[Union[str, sympy.Symbol]] = None,
        pauli: cirq.Pauli,
    ):
        r"""

        Args:
            stim_sweep_bit_index: The bit position, in some unspecified array, controlling the Pauli.
            cirq_sweep_symbol: The symbol used by cirq. Defaults to f"sweep_{sweep_bit_index}".
            pauli: The cirq Pauli operation to apply when the bit is True.
        """
        if cirq_sweep_symbol is None:
            cirq_sweep_symbol = f"sweep[{stim_sweep_bit_index}]"
        self.stim_sweep_bit_index = stim_sweep_bit_index
        self.cirq_sweep_symbol = cirq_sweep_symbol
        self.pauli = pauli

    def _decomposed_into_pauli(self) -> cirq.Gate:
        return self.pauli ** self.cirq_sweep_symbol

    def _resolve_parameters_(self, resolver: cirq.ParamResolver, recursive: bool) -> cirq.Gate:
        new_value = resolver.value_of(self.cirq_sweep_symbol, recursive=recursive)
        if str(new_value) == str(self.cirq_sweep_symbol):
            return self
        return self.pauli ** new_value

    def _is_parameterized_(self) -> bool:
        return True

    def _parameter_names_(self) -> AbstractSet[str]:
        return {str(self.cirq_sweep_symbol)}

    def _decompose_(self, qubits) -> cirq.OP_TREE:
        return self.pauli.on(*qubits) ** self.cirq_sweep_symbol

    def _num_qubits_(self) -> int:
        return 1

    def _value_equality_values_(self) -> Any:
        return self.pauli, self.stim_sweep_bit_index, self.cirq_sweep_symbol

    def _circuit_diagram_info_(self, args: Any) -> str:
        return f"{self.pauli}^sweep[{self.stim_sweep_bit_index}]='{self.cirq_sweep_symbol}'"

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {
            'pauli': self.pauli,
            'stim_sweep_bit_index': self.stim_sweep_bit_index,
            'cirq_sweep_symbol': self.cirq_sweep_symbol,
        }

    def __repr__(self) -> str:
        return (
            f'stimcirq.SweepPauli('
            f'pauli={self.pauli!r}, '
            f'stim_sweep_bit_index={self.stim_sweep_bit_index!r}, '
            f'cirq_sweep_symbol={self.cirq_sweep_symbol!r})'
        )

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int], **kwargs):
        edit_circuit.append_operation(
            f"C{self.pauli}", [stim.target_sweep_bit(self.stim_sweep_bit_index)] + targets
        )
