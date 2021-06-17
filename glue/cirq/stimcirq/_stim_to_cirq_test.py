import inspect
from typing import Tuple, Union, Callable, Any

import cirq
import pytest
import stim

import stimcirq
from ._stim_to_cirq import stim_to_cirq_gate_table


def test_two_qubit_asymmetric_depolarizing_channel():
    r = stimcirq.TwoQubitAsymmetricDepolarizingChannel([0.125, 0, 0, 0, 0, 0, 0.375, 0, 0, 0, 0, 0, 0, 0.25, 0])
    assert r._dense_mixture_() == [
        (0.25, cirq.DensePauliString("II")),
        (0.125, cirq.DensePauliString("IX")),
        (0.375, cirq.DensePauliString("XZ")),
        (0.25, cirq.DensePauliString("ZY")),
    ]
    cirq.testing.assert_has_diagram(cirq.Circuit(r.on(*cirq.LineQubit.range(2))), """
0: ───PauliMix(II:0.25,IX:0.125,XZ:0.375,ZY:0.25)───
      │
1: ───#2────────────────────────────────────────────
    """)
    assert eval(repr(r), {'stimcirq': stimcirq}) == r


def test_stim_circuit_to_cirq_circuit():
    circuit = stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
        X 0
        CNOT 0 1
        X_ERROR(0.125) 0 1
        CORRELATED_ERROR(0.25) X0 Y1 Z2
        M 1
        M !1
        TICK
        MR 0 !1
    """))
    a, b, c = cirq.LineQubit.range(3)
    assert circuit == cirq.Circuit(
        cirq.Moment(cirq.X(a)),
        cirq.Moment(cirq.CNOT(a, b)),
        cirq.Moment(
            cirq.X.with_probability(0.125).on(a),
            cirq.X.with_probability(0.125).on(b),
        ),
        cirq.Moment(
            cirq.PauliString({a: cirq.X, b: cirq.Y, c: cirq.Z}).with_probability(0.25),
        ),
        cirq.Moment(
            cirq.measure(b, key="0"),
        ),
        cirq.Moment(
            cirq.measure(b, key="1", invert_mask=(True,)),
        ),
        cirq.Moment(
            stimcirq.MeasureAndOrReset(measure=True, reset=True, basis='Z', invert_measure=False, key='2').on(a),
            stimcirq.MeasureAndOrReset(measure=True, reset=True, basis='Z', invert_measure=True, key='3').on(b),
        ),
    )


@pytest.mark.parametrize("handler", list(stim_to_cirq_gate_table().values()) + [
    stimcirq.MeasureAndOrReset(
        measure=True,
        reset=reset,
        basis=basis,
        invert_measure=invert,
        key='0'
    )
    for reset in [False, True]
    for basis in ['X', 'Y', 'Z']
    for invert in [False, True]
    if reset or basis != 'Z'
] + [
    cirq.MeasurementGate(num_qubits=1, key='0', invert_mask=()),
    cirq.MeasurementGate(num_qubits=1, key='0', invert_mask=(True,)),
])
def test_exact_gate_round_trips(handler: Union[cirq.Gate, Callable[[Any], cirq.Gate], Tuple]):
    if handler == ():
        return
    if isinstance(handler, cirq.Gate):
        gate = handler
    else:
        try:
            gate = handler(0.125)
        except:
            try:
                gate = handler([k/128 for k in range(1, 4)])
            except:
                gate = handler([k/128 for k in range(1, 16)])
    n = cirq.num_qubits(gate)
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


def test_circuit_diagram():
    cirq.testing.assert_has_diagram(
        stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
            M 0
            MX 0
            MY 0
            MZ 0
            R 0
            RX 0
            RY 0
            RZ 0
            MR 0
            MRX 0
            MRY 0
            MRZ 0
        """)), """
0: ───M───MX('1')───MY('2')───M('3')───R───RX───RY───R───MR('4')───MRX('5')───MRY('6')───MR('7')───
        """)
