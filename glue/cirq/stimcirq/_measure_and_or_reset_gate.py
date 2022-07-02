from typing import Any, Dict, List

import cirq
import stim


@cirq.value_equality
class MeasureAndOrResetGate(cirq.Gate):
    """Handles explaining stim's MX/MY/MZ/MRX/MRY/MRZ/RX/RY/RZ gates to cirq."""

    def __init__(
        self,
        measure: bool,
        reset: bool,
        basis: str,
        invert_measure: bool,
        key: str,
        measure_flip_probability: float = 0,
    ):
        self.measure = measure
        self.reset = reset
        self.basis = basis
        self.invert_measure = invert_measure
        self.key = key
        self.measure_flip_probability = measure_flip_probability

    def _num_qubits_(self) -> int:
        return 1

    def _circuit_diagram_info_(self, args: cirq.CircuitDiagramInfoArgs) -> str:
        return str(self)

    def resolve(self, target: cirq.Qid) -> cirq.Operation:
        if (
            self.basis == 'Z'
            and self.measure
            and self.measure_flip_probability == 0
            and not self.reset
        ):
            return cirq.MeasurementGate(
                num_qubits=1, key=self.key, invert_mask=(1,) if self.invert_measure else ()
            ).on(target)

        return MeasureAndOrResetGate(
            measure=self.measure,
            reset=self.reset,
            basis=self.basis,
            invert_measure=self.invert_measure,
            key=self.key,
            measure_flip_probability=self.measure_flip_probability,
        ).on(target)

    def _value_equality_values_(self):
        return (
            self.measure,
            self.reset,
            self.basis,
            self.invert_measure,
            self.key,
            self.measure_flip_probability,
        )

    def _decompose_(self, qubits):
        (q,) = qubits
        if self.measure:
            if self.basis == 'X':
                yield cirq.H(q)
            elif self.basis == 'Y':
                yield cirq.X(q) ** 0.5
            if self.measure_flip_probability:
                raise NotImplementedError("Noisy measurement as a cirq operation.")
            else:
                yield cirq.measure(
                    q, key=self.key, invert_mask=(True,) if self.invert_measure else ()
                )
        if self.reset:
            yield cirq.ResetChannel().on(q)
        if self.measure or self.reset:
            if self.basis == 'X':
                yield cirq.H(q)
            elif self.basis == 'Y':
                yield cirq.X(q) ** -0.5

    def _stim_op_name(self) -> str:
        result = ''
        if self.measure:
            result += "M"
        if self.reset:
            result += "R"
        if self.basis != 'Z':
            result += self.basis
        return result

    def _stim_conversion_(self, edit_circuit: stim.Circuit, targets: List[int], **kwargs):
        if self.invert_measure:
            targets[0] = stim.target_inv(targets[0])
        if self.measure_flip_probability:
            edit_circuit.append_operation(
                self._stim_op_name(), targets, self.measure_flip_probability
            )
        else:
            edit_circuit.append_operation(self._stim_op_name(), targets)

    def __str__(self) -> str:
        result = self._stim_op_name()
        if self.invert_measure:
            result = "!" + result
        if self.measure:
            result += f"('{self.key}')"
        if self.measure_flip_probability:
            result += f"^{self.measure_flip_probability:%}"
        return result

    def __repr__(self):
        return (
            f'stimcirq.MeasureAndOrResetGate('
            f'measure={self.measure!r}, '
            f'reset={self.reset!r}, '
            f'basis={self.basis!r}, '
            f'invert_measure={self.invert_measure!r}, '
            f'key={self.key!r}, '
            f'measure_flip_probability={self.measure_flip_probability!r})'
        )

    @staticmethod
    def _json_namespace_() -> str:
        return ''

    def _json_dict_(self) -> Dict[str, Any]:
        return {
            'measure': self.measure,
            'reset': self.reset,
            'basis': self.basis,
            'invert_measure': self.invert_measure,
            'key': self.key,
            'measure_flip_probability': self.measure_flip_probability,
        }
