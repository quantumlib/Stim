import inspect
from typing import Tuple, Union, Callable, Any, cast

import cirq
import pytest
import stim

import stimcirq
from ._stim_to_cirq import stim_to_cirq_gate_table, not_handled_or_handled_specially_set


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


@pytest.mark.parametrize("key,handler", list(stim_to_cirq_gate_table().items()) + [
    ("m-case", stimcirq.MeasureAndOrReset(
        measure=True,
        reset=reset,
        basis=basis,
        invert_measure=invert,
        key='0'
    ))
    for reset in [False, True]
    for basis in ['X', 'Y', 'Z']
    for invert in [False, True]
    if reset or basis != 'Z'
] + [
    ("+M", cirq.MeasurementGate(num_qubits=1, key='0', invert_mask=())),
    ("-M", cirq.MeasurementGate(num_qubits=1, key='0', invert_mask=(True,))),
])
def test_exact_gate_round_trips(key: str, handler: Union[cirq.Gate, Callable[[Any], cirq.Gate], Tuple]):
    if handler == ():
        return
    if isinstance(handler, cirq.Gate):
        gate = handler
    else:
        try:
            gate = handler(0.125)
        except NotImplementedError:
            assert key in not_handled_or_handled_specially_set()
            return
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


def test_all_known_gates_explicitly_handled():
    output_from_stim_help_gates = """
CNOT
CORRELATED_ERROR
CX
CY
CZ
C_XYZ
C_ZYX
DEPOLARIZE1
DEPOLARIZE2
DETECTOR
E
ELSE_CORRELATED_ERROR
H
H_XY
H_XZ
H_YZ
I
ISWAP
ISWAP_DAG
M
MR
MRX
MRY
MRZ
MX
MY
MZ
OBSERVABLE_INCLUDE
PAULI_CHANNEL_1
PAULI_CHANNEL_2
QUBIT_COORDS
R
REPEAT
RX
RY
RZ
S
SHIFT_COORDS
SQRT_X
SQRT_XX
SQRT_XX_DAG
SQRT_X_DAG
SQRT_Y
SQRT_YY
SQRT_YY_DAG
SQRT_Y_DAG
SQRT_Z
SQRT_ZZ
SQRT_ZZ_DAG
SQRT_Z_DAG
SWAP
S_DAG
TICK
X
XCX
XCY
XCZ
X_ERROR
Y
YCX
YCY
YCZ
Y_ERROR
Z
ZCX
ZCY
ZCZ
Z_ERROR
    """
    gates = output_from_stim_help_gates.strip().split()
    handled = stim_to_cirq_gate_table().keys() | not_handled_or_handled_specially_set()
    for gate_name in gates:
        assert gate_name in handled, gate_name


def test_line_grid_qubit_round_trip():
    c = cirq.Circuit(
        cirq.H(cirq.LineQubit(101)),
        cirq.Z(cirq.LineQubit(200.5)),
        cirq.X(cirq.GridQubit(2, 3.5)),
        cirq.Y(cirq.GridQubit(-2, 5)),
    )
    s = stimcirq.cirq_circuit_to_stim_circuit(c)
    assert s == stim.Circuit("""
        QUBIT_COORDS(-2, 5) 0
        QUBIT_COORDS(2, 3.5) 1
        QUBIT_COORDS(101) 2
        QUBIT_COORDS(200.5) 3
        H 2
        Z 3
        X 1
        Y 0
        TICK
    """)
    c2 = stimcirq.stim_circuit_to_cirq_circuit(s)
    for q in c2.all_qubits():
        if q == cirq.LineQubit(101):
            assert isinstance(cast(cirq.LineQubit, q).x, int)
        if q == cirq.GridQubit(2, 3.5):
            assert isinstance(cast(cirq.GridQubit, q).row, int)
    assert c2 == c

    assert stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
        QUBIT_COORDS(10) 0
        SHIFT_COORDS(1, 2, 3)
        QUBIT_COORDS(20, 30) 1
        H 0 1
    """)) == cirq.Circuit(cirq.H(cirq.LineQubit(10)), cirq.H(cirq.GridQubit(21, 32)))


def test_noisy_measurements():
    s = stim.Circuit("""
        MX(0.125) 0
        MY(0.125) 1
        MZ(0.125) 2
        MRX(0.125) 3
        MRY(0.125) 4
        MRZ(0.25) 5
        TICK
    """)
    c = stimcirq.stim_circuit_to_cirq_circuit(s)
    assert c == cirq.Circuit(
        stimcirq.MeasureAndOrReset(measure=True, reset=False, basis='X', invert_measure=False, key='0', measure_flip_probability=0.125).on(cirq.LineQubit(0)),
        stimcirq.MeasureAndOrReset(measure=True, reset=False, basis='Y', invert_measure=False, key='1', measure_flip_probability=0.125).on(cirq.LineQubit(1)),
        stimcirq.MeasureAndOrReset(measure=True, reset=False, basis='Z', invert_measure=False, key='2', measure_flip_probability=0.125).on(cirq.LineQubit(2)),
        stimcirq.MeasureAndOrReset(measure=True, reset=True, basis='X', invert_measure=False, key='3', measure_flip_probability=0.125).on(cirq.LineQubit(3)),
        stimcirq.MeasureAndOrReset(measure=True, reset=True, basis='Y', invert_measure=False, key='4', measure_flip_probability=0.125).on(cirq.LineQubit(4)),
        stimcirq.MeasureAndOrReset(measure=True, reset=True, basis='Z', invert_measure=False, key='5', measure_flip_probability=0.25).on(cirq.LineQubit(5)),
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(c) == s
    cirq.testing.assert_equivalent_repr(c, global_vals={'stimcirq': stimcirq})
