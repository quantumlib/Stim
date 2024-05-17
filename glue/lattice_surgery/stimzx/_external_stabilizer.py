from typing import List, Any

import stim


class ExternalStabilizer:
    """An input-to-output relationship enforced by a stabilizer circuit."""

    def __init__(self, *, input: stim.PauliString, output: stim.PauliString):
        self.input = input
        self.output = output

    @staticmethod
    def from_dual(dual: stim.PauliString, num_inputs: int) -> 'ExternalStabilizer':
        sign = dual.sign

        # Transpose input. Ys get negated.
        for k in range(num_inputs):
            if dual[k] == 2:
                sign *= -1

        return ExternalStabilizer(
            input=dual[:num_inputs],
            output=dual[num_inputs:],
        )

    @staticmethod
    def canonicals_from_duals(duals: List[stim.PauliString], num_inputs: int) -> List['ExternalStabilizer']:
        if not duals:
            return []
        duals = [e.copy() for e in duals]
        num_qubits = len(duals[0])
        num_outputs = num_qubits - num_inputs
        id_out = stim.PauliString(num_outputs)

        # Pivot on output qubits, to potentially isolate input-only stabilizers.
        _eliminate_stabilizers(duals, range(num_inputs, num_qubits))

        # Separate input-only stabilizers from the rest.
        input_only_stabilizers = []
        output_using_stabilizers = []
        for dual in duals:
            if dual[num_inputs:] == id_out:
                input_only_stabilizers.append(dual)
            else:
                output_using_stabilizers.append(dual)

        # Separately canonicalize the output-using and input-only stabilizers.
        _eliminate_stabilizers(output_using_stabilizers, range(num_qubits))
        _eliminate_stabilizers(input_only_stabilizers, range(num_inputs))

        duals = input_only_stabilizers + output_using_stabilizers
        return [ExternalStabilizer.from_dual(e, num_inputs) for e in duals]

    def __mul__(self, other: 'ExternalStabilizer') -> 'ExternalStabilizer':
        return ExternalStabilizer(input=other.input * self.input, output=self.output * other.output)

    def __str__(self) -> str:
        return str(self.input) + ' -> ' + str(self.output)

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, ExternalStabilizer):
            return NotImplemented
        return self.output == other.output and self.input == other.input

    def __ne__(self, other: Any) -> bool:
        return not self == other

    def __repr__(self):
        return f'stimzx.ExternalStabilizer(input={self.input!r}, output={self.output!r})'


def _eliminate_stabilizers(stabilizers: List[stim.PauliString], elimination_indices: range):
    """Performs partial Gaussian elimination on the list of stabilizers."""
    min_pivot = 0
    for q in elimination_indices:
        for b in [1, 3]:
            for pivot in range(min_pivot, len(stabilizers)):
                p = stabilizers[pivot][q]
                if p == 2 or p == b:
                    break
            else:
                continue
            for k, stabilizer in enumerate(stabilizers):
                p = stabilizer[q]
                if k != pivot and (p == 2 or p == b):
                    stabilizer *= stabilizers[pivot]
            if min_pivot != pivot:
                stabilizers[min_pivot], stabilizers[pivot] = stabilizers[pivot], stabilizers[min_pivot]
            min_pivot += 1
