from typing import Tuple, Union, Callable

import cirq
import pytest
import stim

import stimcirq
from ._circuit_conversion import STIM_TO_CIRQ_GATE_TABLE


def test_stim_circuit_to_cirq_circuit():
    circuit = stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
        X 0
        CNOT 0 1
        X_ERROR(0.125) 0 1
        CORRELATED_ERROR(0.25) X0 Y1 Z2
        M 1
        M !1
        MR 0 !1
    """))
    a, b, c = cirq.LineQubit.range(3)
    assert circuit == cirq.Circuit(
        cirq.X(a),
        cirq.CNOT(a, b),
        cirq.X.with_probability(0.125).on(a),
        cirq.X.with_probability(0.125).on(b),
        cirq.PauliString({a: cirq.X, b: cirq.Y, c: cirq.Z}).with_probability(0.25),
        cirq.measure(b, key="0"),
        cirq.measure(b, key="1", invert_mask=(True,)),
        cirq.measure(a, key="2"),
        cirq.ResetChannel().on(a),
        cirq.measure(b, key="3", invert_mask=(True,)),
        cirq.ResetChannel().on(b),
    )


@pytest.mark.parametrize("handler", STIM_TO_CIRQ_GATE_TABLE.values())
def test_exact_gate_round_trips(handler: Union[cirq.Gate, Callable[[float], cirq.Gate], Tuple]):
    if handler == ():
        return
    if isinstance(handler, cirq.Gate):
        gate = handler
    else:
        gate = handler(0.125)

    n = gate.num_qubits()
    qs = cirq.LineQubit.range(n)
    original = cirq.Circuit(gate.on(*qs))
    converted = stimcirq.cirq_circuit_to_stim_circuit(original)
    restored = stimcirq.stim_circuit_to_cirq_circuit(converted)
    assert original == restored


def test_round_trip_preserves_moment_structure():
    a, b = cirq.LineQubit.range(2)
    circuit = cirq.Circuit(
        cirq.Moment(),
        cirq.Moment(cirq.H(a)),
        cirq.Moment(cirq.H(b)),
        cirq.Moment(),
        cirq.Moment(),
        cirq.Moment(cirq.CNOT(a, b)),
        cirq.Moment(),
    )
    converted = stimcirq.cirq_circuit_to_stim_circuit(circuit)
    restored = stimcirq.stim_circuit_to_cirq_circuit(converted)
    assert restored == circuit
