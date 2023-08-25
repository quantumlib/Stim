from typing import Any, Dict, List, Sequence, Tuple

import cirq
import stim


@cirq.value_equality
class TwoQubitAsymmetricDepolarizingChannel(cirq.Gate):
    """A two qubit error channel that applies Pauli pairs according to the given probability
    distribution.

    The 15 probabilities are in IX, IY, ... ZY, ZZ order. II is skipped; it's the leftover.

    This class is no longer used by stimcirq, but is kept around for backwards
    compatibility of json-serialized circuits.
    """

    def __init__(self, probabilities: Sequence[float]):
        if len(probabilities) != 15:
            raise ValueError(f"len(probabilities={probabilities}) != 15")
        if sum(probabilities) > 1.000001:
            raise ValueError(f"sum(probabilities={probabilities}) > 1")
        self.probabilities = tuple(probabilities)

    def _num_qubits_(self):
        return 2

    def _value_equality_values_(self):
        return self.probabilities

    def _has_mixture_(self):
        return True

    def _dense_mixture_(self):
        result = [(1 - sum(self.probabilities), cirq.DensePauliString([0, 0]))]
        result.extend(
            [
                (p, cirq.DensePauliString([((k + 1) >> 2) & 3, (k + 1) & 3]))
                for k, p in enumerate(self.probabilities)
            ]
        )
        return [(p, g) for p, g in result if p]

    def _mixture_(self):
        return [(p, cirq.unitary(g)) for p, g in self._dense_mixture_()]

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs):
        result = []
        for p, d in self._dense_mixture_():
            result.append(str(d)[1:] + ":" + args.format_real(p))
        return "PauliMix(" + ",".join(result) + ")", "#2"

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int], **kwargs):
        edit_circuit.append_operation("PAULI_CHANNEL_2", targets, self.probabilities)

    def __repr__(self):
        return f"stimcirq.TwoQubitAsymmetricDepolarizingChannel({self.probabilities!r})"

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {'probabilities': list(self.probabilities)}
