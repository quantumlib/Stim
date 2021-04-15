import functools
from typing import Callable, Dict, List, Tuple, Union, Iterator

import cirq
import stim


@functools.lru_cache(maxsize=1)
def stim_to_cirq_gate_table() -> Dict[str, Union[Tuple, cirq.Gate, Callable[[float], cirq.Gate]]]:
    return {
        "R": cirq.ResetChannel(),
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
        m = gate.num_qubits()
        for k in range(0, len(targets), m):
            yield gate(*[cirq.LineQubit(q) for q in targets[k:k+m]])
        return

    if name == "M" or name == "MR":
        for t in targets:
            if isinstance(t, int):
                q = t
                inv = False
            elif t[0] == "inv":
                q = t[1]
                inv = True
            else:
                raise NotImplementedError("Unrecognized measurement target.")
            q = cirq.LineQubit(q)
            yield cirq.measure(q, key=str(get_next_measure_id()), invert_mask=(True,) if inv else ())
            if name == "MR":
                yield cirq.ResetChannel().on(q)
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
