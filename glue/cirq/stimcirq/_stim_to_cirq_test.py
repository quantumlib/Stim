import inspect
import itertools
from typing import Any, Callable, cast, Tuple, Union

import cirq
import pytest
import stim
import stimcirq

from ._stim_to_cirq import CircuitTranslationTracker


def test_stim_circuit_to_cirq_circuit():
    circuit = stimcirq.stim_circuit_to_cirq_circuit(
        stim.Circuit(
            """
        X 0
        CNOT 0 1
        X_ERROR(0.125) 0 1
        CORRELATED_ERROR(0.25) X0 Y1 Z2
        M 1
        M !1
        TICK
        MR 0 !1
    """
        )
    )
    a, b, c = cirq.LineQubit.range(3)
    assert circuit == cirq.Circuit(
        cirq.Moment(cirq.X(a)),
        cirq.Moment(cirq.CNOT(a, b)),
        cirq.Moment(cirq.X.with_probability(0.125).on(a), cirq.X.with_probability(0.125).on(b)),
        cirq.Moment(cirq.PauliString({a: cirq.X, b: cirq.Y, c: cirq.Z}).with_probability(0.25)),
        cirq.Moment(cirq.measure(b, key="0")),
        cirq.Moment(cirq.measure(b, key="1", invert_mask=(True,))),
        cirq.Moment(
            stimcirq.MeasureAndOrResetGate(
                measure=True, reset=True, basis='Z', invert_measure=False, key='2'
            ).on(a),
            stimcirq.MeasureAndOrResetGate(
                measure=True, reset=True, basis='Z', invert_measure=True, key='3'
            ).on(b),
        ),
    )


def assert_circuits_are_equivalent_and_convert(
    cirq_circuit: cirq.Circuit, stim_circuit: stim.Circuit
):
    assert cirq_circuit == stimcirq.stim_circuit_to_cirq_circuit(stim_circuit)
    assert stim_circuit == stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit)


@pytest.mark.parametrize(
    "name",
    [
        k
        for k, v in CircuitTranslationTracker.get_handler_table().items()
        if isinstance(v, CircuitTranslationTracker.OneToOneGateHandler)
    ],
)
def test_gates_converted_using_OneToOneGateHandler(name: str):
    handler = cast(
        CircuitTranslationTracker.OneToOneGateHandler,
        CircuitTranslationTracker.get_handler_table()[name],
    )
    gate = handler.gate
    n = cirq.num_qubits(gate)
    qs = cirq.LineQubit.range(n)

    cirq_original = cirq.Circuit(gate.on(*qs))
    stim_targets = " ".join(str(e) for e in range(n))
    stim_original = stim.Circuit(f"{name} {stim_targets}\nTICK")
    assert_circuits_are_equivalent_and_convert(cirq_original, stim_original)


@pytest.mark.parametrize(
    "name",
    [
        k
        for k, v in CircuitTranslationTracker.get_handler_table().items()
        if isinstance(v, CircuitTranslationTracker.SweepableGateHandler)
    ],
)
def test_gates_converted_using_SweepableGateHandler(name: str):
    handler = cast(
        CircuitTranslationTracker.SweepableGateHandler,
        CircuitTranslationTracker.get_handler_table()[name],
    )
    gate = handler.gate
    n = cirq.num_qubits(gate)
    qs = cirq.LineQubit.range(n)

    # Without sweeping.
    cirq_original = cirq.Circuit(gate.on(*qs))
    stim_targets = " ".join(str(e) for e in range(n))
    stim_original = stim.Circuit(f"{name} {stim_targets}\nTICK")
    assert_circuits_are_equivalent_and_convert(cirq_original, stim_original)

    # With sweeping.
    q = qs[0]
    cirq_original = cirq.Circuit(
        stimcirq.SweepPauli(
            stim_sweep_bit_index=3, cirq_sweep_symbol="sweep[3]", pauli=handler.pauli_gate
        ).on(q)
    )
    if name.startswith("C") or name.startswith("Z"):
        stim_original = stim.Circuit(f"{name} sweep[3] 0\nTICK")
        assert_circuits_are_equivalent_and_convert(cirq_original, stim_original)
    else:
        stim_original = stim.Circuit(f"{name} 0 sweep[3]\nTICK")
        # Round trip loses distinction between ZCX and XCZ.
        assert stimcirq.stim_circuit_to_cirq_circuit(stim_original) == cirq_original


@pytest.mark.parametrize(
    "name,probability",
    itertools.product(
        [
            k
            for k, v in CircuitTranslationTracker.get_handler_table().items()
            if isinstance(v, CircuitTranslationTracker.OneToOneNoisyGateHandler)
        ],
        [0, 0.125],
    ),
)
def test_gates_converted_using_OneToOneNoisyGateHandler(name: str, probability: float):
    handler = cast(
        CircuitTranslationTracker.OneToOneNoisyGateHandler,
        CircuitTranslationTracker.get_handler_table()[name],
    )
    gate = handler.prob_to_gate(probability)
    n = cirq.num_qubits(gate)
    qs = cirq.LineQubit.range(n)

    cirq_original = cirq.Circuit(gate.on(*qs))
    stim_targets = " ".join(str(e) for e in range(n))
    stim_original = stim.Circuit(f"{name}({probability}) {stim_targets}\nTICK")
    assert_circuits_are_equivalent_and_convert(cirq_original, stim_original)


@pytest.mark.parametrize(
    "name,invert,probability",
    itertools.product(
        [
            k
            for k, v in CircuitTranslationTracker.get_handler_table().items()
            if isinstance(v, CircuitTranslationTracker.OneToOneMeasurementHandler)
        ],
        [False, True],
        [0, 0.125],
    ),
)
def test_gates_converted_using_OneToOneMeasurementHandler(
    name: str, invert: bool, probability: float
):
    handler = cast(
        CircuitTranslationTracker.OneToOneMeasurementHandler,
        CircuitTranslationTracker.get_handler_table()[name],
    )
    if handler.basis == 'Z' and not handler.reset and not probability:
        cirq_original = cirq.Circuit(
            cirq.measure(cirq.LineQubit(5), key="0", invert_mask=(1,) * invert)
        )
    else:
        cirq_original = cirq.Circuit(
            stimcirq.MeasureAndOrResetGate(
                basis=handler.basis,
                key="0",
                invert_measure=invert,
                reset=handler.reset,
                measure_flip_probability=probability,
                measure=handler.measure,
            ).on(cirq.LineQubit(5))
        )
    inverted = "!" if invert else ""
    p_str = f"({probability})" if probability else ""
    stim_original = stim.Circuit(f"QUBIT_COORDS(5) 0\n{name}{p_str} {inverted}0\nTICK")
    assert_circuits_are_equivalent_and_convert(cirq_original, stim_original)


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
        stimcirq.stim_circuit_to_cirq_circuit(
            stim.Circuit(
                """
            M 9
            MX 9
            MY 9
            MZ 9
            R 9
            RX 9
            RY 9
            RZ 9
            MR 9
            MRX 9
            MRY 9
            MRZ 9
        """
            )
        ),
        """
9: ---M('0')---MX('1')---MY('2')---M('3')---R---RX---RY---R---MR('4')---MRX('5')---MRY('6')---MR('7')---
        """,
        use_unicode_characters=False,
    )


def test_all_known_gates_explicitly_handled():
    gates = stim.gate_data()
    handled = CircuitTranslationTracker.get_handler_table().keys()
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
    assert s == stim.Circuit(
        """
        QUBIT_COORDS(-2, 5) 0
        QUBIT_COORDS(2, 3.5) 1
        QUBIT_COORDS(101) 2
        QUBIT_COORDS(200.5) 3
        H 2
        Z 3
        X 1
        Y 0
        TICK
    """
    )
    c2 = stimcirq.stim_circuit_to_cirq_circuit(s)
    for q in c2.all_qubits():
        if q == cirq.LineQubit(101):
            assert isinstance(cast(cirq.LineQubit, q).x, int)
        if q == cirq.GridQubit(2, 3.5):
            assert isinstance(cast(cirq.GridQubit, q).row, int)
    assert c2 == c

    assert (
        stimcirq.stim_circuit_to_cirq_circuit(
            stim.Circuit(
                """
        QUBIT_COORDS(10) 0
        SHIFT_COORDS(1, 2, 3)
        QUBIT_COORDS(20, 30) 1
        H 0 1
    """
            )
        )
        == cirq.Circuit(
            stimcirq.ShiftCoordsAnnotation([1, 2, 3]),
            cirq.H(cirq.LineQubit(10)),
            cirq.H(cirq.GridQubit(21, 32)),
        )
    )


def test_noisy_measurements():
    s = stim.Circuit(
        """
        MX(0.125) 0
        MY(0.125) 1
        MZ(0.125) 2
        MRX(0.125) 3
        MRY(0.125) 4
        MRZ(0.25) 5
        TICK
    """
    )
    c = stimcirq.stim_circuit_to_cirq_circuit(s)
    assert c == cirq.Circuit(
        stimcirq.MeasureAndOrResetGate(
            measure=True,
            reset=False,
            basis='X',
            invert_measure=False,
            key='0',
            measure_flip_probability=0.125,
        ).on(cirq.LineQubit(0)),
        stimcirq.MeasureAndOrResetGate(
            measure=True,
            reset=False,
            basis='Y',
            invert_measure=False,
            key='1',
            measure_flip_probability=0.125,
        ).on(cirq.LineQubit(1)),
        stimcirq.MeasureAndOrResetGate(
            measure=True,
            reset=False,
            basis='Z',
            invert_measure=False,
            key='2',
            measure_flip_probability=0.125,
        ).on(cirq.LineQubit(2)),
        stimcirq.MeasureAndOrResetGate(
            measure=True,
            reset=True,
            basis='X',
            invert_measure=False,
            key='3',
            measure_flip_probability=0.125,
        ).on(cirq.LineQubit(3)),
        stimcirq.MeasureAndOrResetGate(
            measure=True,
            reset=True,
            basis='Y',
            invert_measure=False,
            key='4',
            measure_flip_probability=0.125,
        ).on(cirq.LineQubit(4)),
        stimcirq.MeasureAndOrResetGate(
            measure=True,
            reset=True,
            basis='Z',
            invert_measure=False,
            key='5',
            measure_flip_probability=0.25,
        ).on(cirq.LineQubit(5)),
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(c) == s
    cirq.testing.assert_equivalent_repr(c, global_vals={'stimcirq': stimcirq})


def test_convert_mpp():
    s = stim.Circuit(
        """
        MPP X2*Y5*Z3
        TICK
        MPP Y2 X1 Z3*Z4
        X 0
        TICK
    """
    )
    c = cirq.Circuit(
        cirq.Moment(
            cirq.PauliMeasurementGate(cirq.DensePauliString("XYZ", coefficient=+1), key='0').on(
                cirq.LineQubit(2), cirq.LineQubit(5), cirq.LineQubit(3)
            )
        ),
        cirq.Moment(
            cirq.PauliMeasurementGate(cirq.DensePauliString("Y"), key='1').on(cirq.LineQubit(2)),
            cirq.PauliMeasurementGate(cirq.DensePauliString("X"), key='2').on(cirq.LineQubit(1)),
            cirq.PauliMeasurementGate(cirq.DensePauliString("ZZ"), key='3').on(
                cirq.LineQubit(3), cirq.LineQubit(4)
            ),
            cirq.X(cirq.LineQubit(0)),
        ),
    )
    assert_circuits_are_equivalent_and_convert(c, s)
    cirq.testing.assert_has_diagram(
        c,
        """
0: ---------------X-----------

1: ---------------M(X)('2')---

2: ---M(X)('0')---M(Y)('1')---
      |
3: ---M(Z)--------M(Z)('3')---
      |           |
4: ---|-----------M(Z)--------
      |
5: ---M(Y)--------------------
        """,
        use_unicode_characters=False,
    )


def test_convert_detector():
    s = stim.Circuit(
        """
        H 0
        TICK
        CNOT 0 1
        TICK
        M 1 0
        DETECTOR(2, 3, 5) rec[-1] rec[-2]
        TICK
    """
    )
    a, b = cirq.LineQubit.range(2)
    c = cirq.Circuit(
        cirq.H(a),
        cirq.CNOT(a, b),
        cirq.Moment(
            cirq.measure(b, key='0'),
            cirq.measure(a, key='1'),
            stimcirq.DetAnnotation(parity_keys=['0', '1'], coordinate_metadata=(2, 3, 5)),
        ),
    )
    cirq.testing.assert_has_diagram(
        c,
        """
0: ---H---@---M('1')---------
          |
1: -------X---M('0')---------
              Det('0','1')
        """,
        use_unicode_characters=False,
    )
    assert_circuits_are_equivalent_and_convert(c, s)


def test_convert_observable():
    s = stim.Circuit(
        """
        H 0
        TICK
        CNOT 0 1
        TICK
        M 1 0
        OBSERVABLE_INCLUDE(5) rec[-1] rec[-2]
        TICK
    """
    )
    a, b = cirq.LineQubit.range(2)
    c = cirq.Circuit(
        cirq.H(a),
        cirq.CNOT(a, b),
        cirq.Moment(
            cirq.measure(b, key='0'),
            cirq.measure(a, key='1'),
            stimcirq.CumulativeObservableAnnotation(parity_keys=['0', '1'], observable_index=5),
        ),
    )
    cirq.testing.assert_has_diagram(
        c,
        """
0: ---H---@---M('1')----------
          |
1: -------X---M('0')----------
              Obs5('0','1')
        """,
        use_unicode_characters=False,
    )
    assert_circuits_are_equivalent_and_convert(c, s)


def test_sweep_target():
    stim_circuit = stim.Circuit(
        """
        CX sweep[2] 0
        CY sweep[3] 1
        CZ sweep[5] 2 sweep[7] 3
        TICK
    """
    )
    a, b, c, d = cirq.LineQubit.range(4)
    cirq_circuit = cirq.Circuit(
        stimcirq.SweepPauli(stim_sweep_bit_index=2, cirq_sweep_symbol="sweep[2]", pauli=cirq.X).on(
            a
        ),
        stimcirq.SweepPauli(stim_sweep_bit_index=3, cirq_sweep_symbol="sweep[3]", pauli=cirq.Y).on(
            b
        ),
        stimcirq.SweepPauli(stim_sweep_bit_index=5, cirq_sweep_symbol="sweep[5]", pauli=cirq.Z).on(
            c
        ),
        stimcirq.SweepPauli(stim_sweep_bit_index=7, cirq_sweep_symbol="sweep[7]", pauli=cirq.Z).on(
            d
        ),
    )
    cirq.testing.assert_has_diagram(
        cirq_circuit,
        """
0: ---X^sweep[2]='sweep[2]'---

1: ---Y^sweep[3]='sweep[3]'---

2: ---Z^sweep[5]='sweep[5]'---

3: ---Z^sweep[7]='sweep[7]'---
        """,
        use_unicode_characters=False,
    )
    assert_circuits_are_equivalent_and_convert(cirq_circuit, stim_circuit)


def test_convert_repeat_simple():
    stim_circuit = stim.Circuit(
        """
        REPEAT 1000000 {
            H 0
            TICK
            CNOT 0 1
            TICK
        }
        M 0
        TICK
    """
    )
    a, b = cirq.LineQubit.range(2)
    cirq_circuit = cirq.Circuit(
        cirq.CircuitOperation(
            cirq.FrozenCircuit(cirq.H(a), cirq.CNOT(a, b)),
            repetitions=1000000,
            use_repetition_ids=False,
        ),
        cirq.measure(a, key="0"),
    )
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit) == cirq_circuit
    assert stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit) == stim_circuit


def test_convert_repeat_measurements():
    stim_circuit = stim.Circuit(
        """
        QUBIT_COORDS(2, 3) 0
        QUBIT_COORDS(4, 5) 1
        REPEAT 1000000 {
            H 0
            TICK
            CNOT 0 1
            TICK
            M 0 1
            DETECTOR(5) rec[-1] rec[-2]
            SHIFT_COORDS(2)
            TICK
        }
        M 0
        DETECTOR(7) rec[-1] rec[-3]
        TICK
    """
    )
    a, b = cirq.GridQubit(2, 3), cirq.GridQubit(4, 5)
    cirq_circuit = cirq.Circuit(
        cirq.Moment(
            cirq.CircuitOperation(
                cirq.FrozenCircuit(
                    cirq.Moment(cirq.H(a)),
                    cirq.Moment(cirq.CNOT(a, b)),
                    cirq.Moment(
                        cirq.measure(a, key=cirq.MeasurementKey("0")),
                        cirq.measure(b, key=cirq.MeasurementKey("1")),
                        stimcirq.DetAnnotation(relative_keys=[-1, -2], coordinate_metadata=[5]),
                        stimcirq.ShiftCoordsAnnotation([2]),
                    ),
                ),
                repetitions=1000000,
                use_repetition_ids=False,
            )
        ),
        cirq.Moment(
            cirq.measure(a, key="2000000"),
            stimcirq.DetAnnotation(relative_keys=[-1, -3], coordinate_metadata=[7]),
        ),
    )
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit) == cirq_circuit
    assert stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit) == stim_circuit


def test_single_repeat_loops_always_flattened():
    assert stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit(
        """
        REPEAT 1 {
            H 0
        }
        """)) == cirq.Circuit(cirq.H(cirq.LineQubit(0)))
    assert stimcirq.cirq_circuit_to_stim_circuit(cirq.Circuit(
        cirq.CircuitOperation(
            cirq.FrozenCircuit(
                cirq.H(cirq.LineQubit(0)),
            ),
            repetitions=1,
        ),
    )) == stim.Circuit("H 0\nTICK")


def test_circuit_operations_always_in_isolated_moment():
    stim_circuit = stim.Circuit(
        """
        REPEAT 2 {
            H 0 1
        }
        H 2 3
        """
    )
    cirq_circuit = stimcirq.stim_circuit_to_cirq_circuit(stim_circuit)
    qs = cirq.LineQubit.range(4)
    expected_cirq_circuit = cirq.Circuit(
        cirq.CircuitOperation(
            circuit=cirq.FrozenCircuit([cirq.H(qs[0]),cirq.H(qs[1])]),
            repetitions=2,
            use_repetition_ids=False
        ),
        cirq.Moment(cirq.H(qs[2]),cirq.H(qs[3])),
    )
    assert cirq_circuit == expected_cirq_circuit


@pytest.mark.skip(reason="blocked by https://github.com/quantumlib/Cirq/issues/6136")
def test_stim_circuit_to_cirq_circuit_mpad():
    stim_circuit = stim.Circuit("""
        MPAD 0 1
    """)
    cirq_circuit = stimcirq.stim_circuit_to_cirq_circuit(stim_circuit)
    assert cirq_circuit == cirq.Circuit(
        cirq.PauliMeasurementGate(cirq.DensePauliString(""), key="0").on(),
        cirq.PauliMeasurementGate(-cirq.DensePauliString(""), key="1").on(),
    )


def test_stim_circuit_to_cirq_circuit_mxx_myy_mzz():
    stim_circuit = stim.Circuit("""
        MXX 0 1
        MYY 0 !1
        MZZ !0 1
    """)
    cirq_circuit = stimcirq.stim_circuit_to_cirq_circuit(stim_circuit)
    a, b = cirq.LineQubit.range(2)
    assert cirq_circuit == cirq.Circuit(
        cirq.PauliMeasurementGate(cirq.DensePauliString("XX"), key='0').on(a, b),
        cirq.PauliMeasurementGate(-cirq.DensePauliString("YY"), key='1').on(a, b),
        cirq.PauliMeasurementGate(-cirq.DensePauliString("ZZ"), key='2').on(a, b),
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit) == stim.Circuit("""
        MPP X0*X1
        TICK
        MPP !Y0*Y1
        TICK
        MPP !Z0*Z1
        TICK
    """)


def test_stim_circuit_to_cirq_circuit_spp():
    stim_circuit = stim.Circuit("""
        SPP X1*Y2*Z3
        SPP !Y4
        SPP_DAG X5
        SPP_DAG !Z0
    """)
    cirq_circuit = stimcirq.stim_circuit_to_cirq_circuit(stim_circuit)
    assert cirq_circuit.to_text_diagram(use_unicode_characters=False).strip() == """
0: ---[Z]^0.5----

1: ---[X]--------
      |
2: ---[Y]--------
      |
3: ---[Z]^0.5----

4: ---[Y]^-0.5---

5: ---[X]^-0.5---
    """.strip()
    q0, q1, q2, q3, q4, q5 = cirq.LineQubit.range(6)
    assert cirq_circuit == cirq.Circuit([
        cirq.Moment(
            cirq.PauliStringPhasor(cirq.X(q1)*cirq.Y(q2)*cirq.Z(q3), exponent_neg=0.5),
            cirq.PauliStringPhasor(cirq.Y(q4), exponent_neg=0, exponent_pos=0.5),
            cirq.PauliStringPhasor(cirq.X(q5), exponent_neg=-0.5),
            cirq.PauliStringPhasor(cirq.Z(q0), exponent_neg=0, exponent_pos=-0.5),
        ),
    ])
    assert stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit) == stim.Circuit("""
        SPP X1*Y2*Z3
        SPP_DAG Y4 X5
        SPP Z0
        TICK
    """)


def test_tags_convert():
    assert stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
        H[my_tag] 0
    """)) == cirq.Circuit(
        cirq.H(cirq.LineQubit(0)).with_tags('my_tag'),
    )


@pytest.mark.parametrize('gate', sorted(stim.gate_data().keys()))
def test_every_operation_converts_tags(gate: str):
    if gate in [
        "ELSE_CORRELATED_ERROR",
        "HERALDED_ERASE",
        "HERALDED_PAULI_CHANNEL_1",
        "TICK",
        "REPEAT",
        "MPAD",
        "QUBIT_COORDS",
    ]:
        pytest.skip()

    data = stim.gate_data(gate)
    stim_circuit = stim.Circuit()
    arg = None
    targets = [0, 1]
    if data.num_parens_arguments_range.start:
        arg = [2**-6] * data.num_parens_arguments_range.start
    if data.takes_pauli_targets:
        targets = [stim.target_x(0), stim.target_y(1)]
    if data.takes_measurement_record_targets and not data.is_unitary:
        stim_circuit.append("M", [0], tag='custom_tag')
        targets = [stim.target_rec(-1)]
    if gate == 'SHIFT_COORDS':
        targets = []
    if gate == 'OBSERVABLE_INCLUDE':
        arg = [1]
    stim_circuit.append(gate, targets, arg, tag='custom_tag')
    cirq_circuit = stimcirq.stim_circuit_to_cirq_circuit(stim_circuit)
    assert any(cirq_circuit.all_operations())
    for op in cirq_circuit.all_operations():
        assert op.tags == ('custom_tag',)
    restored_circuit = stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit)
    assert restored_circuit.pop() == stim.CircuitInstruction("TICK")
    assert all(instruction.tag == 'custom_tag' for instruction in restored_circuit)
    if gate not in ['MXX', 'MYY', 'MZZ']:
        assert restored_circuit == stim_circuit


def test_loop_tagging():
    stim_circuit = stim.Circuit("""
        REPEAT[custom-tag] 5 {
            H[tag2] 0
            TICK
        }
    """)
    cirq_circuit = stimcirq.stim_circuit_to_cirq_circuit(stim_circuit)
    assert cirq_circuit == cirq.Circuit(
        cirq.CircuitOperation(
            cirq.FrozenCircuit(
                cirq.H(cirq.LineQubit(0)).with_tags('tag2'),
            ),
            repetitions=5,
            use_repetition_ids=False,
        ).with_tags('custom-tag')
    )
    restored_circuit = stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit)
    assert restored_circuit == stim_circuit
