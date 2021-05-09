import dataclasses
import functools
from typing import Callable, Dict, List, Tuple, Union, Iterator, cast

import cirq
import stim


@dataclasses.dataclass(frozen=True)
class MeasureAndOrReset(cirq.SingleQubitGate):
    measure: bool
    reset: bool
    basis: str
    invert_measure: bool
    key: str

    def _decompose_(self, qubits):
        q, = qubits
        if self.measure:
            if self.basis == 'X':
                yield cirq.H(q)
            elif self.basis == 'Y':
                yield cirq.X(q)**0.5
            yield cirq.measure(q, key=self.key, invert_mask=(True,) if self.invert_measure else ())
        if self.reset:
            yield cirq.ResetChannel().on(q)
            if self.basis == 'X':
                yield cirq.H(q)
            elif self.basis == 'Y':
                yield cirq.X(q)**-0.5

    def with_key(self, key: str) -> 'MeasureAndOrReset':
        return MeasureAndOrReset(
            measure=self.measure,
            reset=self.reset,
            basis=self.basis,
            invert_measure=self.invert_measure,
            key=key,
        )

    def with_bits_flipped(self, *bit_positions):
        assert bit_positions == (0,)
        return MeasureAndOrReset(
            measure=self.measure,
            reset=self.reset,
            basis=self.basis,
            invert_measure=not self.invert_measure,
            key=self.key,
        )

    def _stim_op_name(self) -> str:
        result = ''
        if self.measure:
            result += "M"
        if self.reset:
            result += "R"
        if self.basis != 'Z':
            result += self.basis
        return result

    def _stim_conversion_(
            self,
            edit_circuit: stim.Circuit,
            targets: List[int],
            **kwargs):
        if self.invert_measure:
            targets[0] = stim.target_inv(targets[0])
        edit_circuit.append_operation(self._stim_op_name(), targets)

    def __str__(self) -> str:
        result = self._stim_op_name()
        if self.invert_measure:
            result = "!" + result
        if self.measure:
            result += f"('{self.key}')"
        return result

    def __repr__(self):
        return (f'stimcirq.MeasureAndOrReset('
                f'measure={self.measure!r}, '
                f'reset={self.reset!r}, '
                f'basis={self.basis!r}, '
                f'invert_measure={self.invert_measure!r}, '
                f'key={self.key!r})')


@functools.lru_cache(maxsize=1)
def stim_to_cirq_gate_table() -> Dict[str, Union[Tuple, cirq.Gate, Callable[[float], cirq.Gate]]]:
    return {
        "R": cirq.ResetChannel(),
        "RX": MeasureAndOrReset(measure=False, reset=True, basis='X', invert_measure=False, key=''),
        "RY": MeasureAndOrReset(measure=False, reset=True, basis='Y', invert_measure=False, key=''),
        "M": cirq.MeasurementGate(num_qubits=1, key='0'),
        "MX": MeasureAndOrReset(measure=True, reset=False, basis='X', invert_measure=False, key='0'),
        "MY": MeasureAndOrReset(measure=True, reset=False, basis='Y', invert_measure=False, key='0'),
        "MR": MeasureAndOrReset(measure=True, reset=True, basis='Z', invert_measure=False, key='0'),
        "MRX": MeasureAndOrReset(measure=True, reset=True, basis='X', invert_measure=False, key='0'),
        "MRY": MeasureAndOrReset(measure=True, reset=True, basis='Y', invert_measure=False, key='0'),
        "I": cirq.I,
        "X": cirq.X,
        "Y": cirq.Y,
        "Z": cirq.Z,
        "H_XY": cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.Y, False), z_to=(cirq.Z, True)),
        "H": cirq.H,
        "H_YZ": cirq.SingleQubitCliffordGate.from_xz_map(x_to=(cirq.X, True), z_to=(cirq.Y, False)),
        "SQRT_X": cirq.X**0.5,
        "SQRT_X_DAG": cirq.X**-0.5,
        "SQRT_Y": cirq.Y**0.5,
        "SQRT_Y_DAG": cirq.Y**-0.5,
        "S": cirq.S,
        "S_DAG": cirq.S**-1,
        "SWAP": cirq.SWAP,
        "ISWAP": cirq.ISWAP,
        "ISWAP_DAG": cirq.ISWAP**-1,
        "XCX": cirq.PauliInteractionGate(cirq.X, False, cirq.X, False),
        "XCY": cirq.PauliInteractionGate(cirq.X, False, cirq.Y, False),
        "XCZ": cirq.PauliInteractionGate(cirq.X, False, cirq.Z, False),
        "YCX": cirq.PauliInteractionGate(cirq.Y, False, cirq.X, False),
        "YCY": cirq.PauliInteractionGate(cirq.Y, False, cirq.Y, False),
        "YCZ": cirq.PauliInteractionGate(cirq.Y, False, cirq.Z, False),
        "CX": cirq.CNOT,
        "CY": cirq.Y.controlled(1),
        "CZ": cirq.CZ,
        "DEPOLARIZE1": lambda arg: cirq.DepolarizingChannel(arg, 1),
        "DEPOLARIZE2": lambda arg: cirq.DepolarizingChannel(arg, 2),
        "X_ERROR": lambda arg: cirq.X.with_probability(arg),
        "Y_ERROR": lambda arg: cirq.Y.with_probability(arg),
        "Z_ERROR": lambda arg: cirq.Z.with_probability(arg),
        "DETECTOR": (),
        "OBSERVABLE_INCLUDE": (),
        "TICK": (),
    }


def _translate_flattened_operation(
        op: Tuple[str, List, float],
        get_next_measure_id: Callable[[], int]) -> Iterator[cirq.Operation]:
    name, targets, arg = op

    handler = stim_to_cirq_gate_table().get(name)

    if handler is not None:
        if isinstance(handler, cirq.Gate):
            gate = handler
        elif handler == ():
            return
        else:
            gate = handler(arg)
        for q in targets:
            if isinstance(q, tuple) and q[0] == "rec":
                raise NotImplementedError("Measurement record.")
        if name[0] == 'M':  # Measurement.
            for t in targets:
                if isinstance(t, int):
                    q = t
                    g = gate
                elif t[0] == "inv":
                    q = t[1]
                    g = cast(Union[MeasureAndOrReset, cirq.MeasurementGate], gate).with_bits_flipped(0)
                else:
                    raise NotImplementedError("Unrecognized measurement target.")
                key = str(get_next_measure_id())
                yield g.with_key(key).on(cirq.LineQubit(q))
        else:
            m = gate.num_qubits()
            for k in range(0, len(targets), m):
                yield gate(*[cirq.LineQubit(q) for q in targets[k:k+m]])
        return

    if name == "E":
        yield cirq.PauliString({cirq.LineQubit(q): k for k, q in targets}).with_probability(arg)
        return

    raise NotImplementedError(f"Unsupported gate: {name}")


def stim_circuit_to_cirq_circuit(circuit: stim.Circuit) -> cirq.Circuit:
    """Converts a stim circuit into an equivalent cirq circuit.

    Qubit indices are turned into cirq.LineQubit instances. Measurements are
    keyed by their ordering (e.g. the first measurement is keyed "0", the second
    is keyed "1", etc).

    Not all circuits can be converted:
        - ELSE_CORRELATED_ERROR instructions are not supported.

    Not all circuits can be converted with perfect 1:1 fidelity:
        - DETECTOR annotations are discarded.
        - OBSERVABLE_INCLUDE annotations are discarded.
        - MR ops decompose into separate measurement and reset ops.

    Args:
        circuit: The stim circuit to convert into a cirq circuit.

    Returns:
        The converted circuit.

    Examples:

        >>> import stimcirq
        >>> import stim
        >>> print(stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit('''
        ...     H 0
        ...     CNOT 0 1
        ...     X_ERROR(0.25) 0
        ...     M !1 0
        ... ''')))
        0: ───H───@───X[prob=0.25]───M('1')───
                  │
        1: ───────X───!M('0')─────────────────
    """
    _next_measure_id = 0

    def get_next_measure_id() -> int:
        nonlocal _next_measure_id
        _next_measure_id += 1
        return _next_measure_id - 1

    full_circuit = cirq.Circuit()
    current_tick = cirq.Circuit()
    for op in circuit.flattened_operations():
        if op[0] == 'TICK':
            if len(current_tick):
                full_circuit += current_tick
                current_tick = cirq.Circuit()
            else:
                full_circuit += cirq.Moment()
        else:
            current_tick += _translate_flattened_operation(op, get_next_measure_id)
    full_circuit += current_tick
    return full_circuit
