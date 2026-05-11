import cirq
import pytest
import stim
import stimcirq


def test_cirq_to_stim_to_cirq_classical_control():
    q = cirq.LineQubit(0)
    cirq_circuit = cirq.Circuit(
        cirq.measure(q, key="test"),
        cirq.X(q).with_classical_controls("test").with_tags("test2")
    )
    stim_circuit = stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit)
    assert stim_circuit == stim.Circuit("""
        M 0
        TICK
        CX[test2] rec[-1] 0
        TICK
    """)
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit) == cirq.Circuit(
        cirq.measure(q, key="0"),
        stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.X).on(q).with_tags("test2")
    )


def test_cirq_to_stim_to_cirq_feedback_pauli():
    q = cirq.LineQubit(0)
    cirq_circuit = cirq.Circuit(
        cirq.measure(q, key="test"),
        stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.X).on(q).with_tags('test3')
    )
    stim_circuit = stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit)
    assert stim_circuit == stim.Circuit("""
        M 0
        TICK
        CX[test3] rec[-1] 0
        TICK
    """)
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit) == cirq.Circuit(
        cirq.measure(q, key="0"),
        stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.X).on(q).with_tags('test3')
    )


def test_stim_to_cirq_conversion():
    with pytest.raises(NotImplementedError, match="wrong target"):
        stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
            M 0
            TICK
            XCZ rec[-1] 3
        """))
    with pytest.raises(NotImplementedError, match="wrong target"):
        stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
            M 0
            TICK
            YCZ rec[-1] 3
        """))
    with pytest.raises(NotImplementedError, match="wrong target"):
        stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
            M 0
            TICK
            CY 3 rec[-1]
        """))
    with pytest.raises(NotImplementedError, match="wrong target"):
        stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
            M 0
            TICK
            CX 3 rec[-1]
        """))
    with pytest.raises(NotImplementedError, match="Two classical"):
        stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
            M 0 1
            TICK
            CZ rec[-1] rec[-2]
        """))

    assert stimcirq.stim_circuit_to_cirq_circuit(stim.Circuit("""
        M 0
        TICK
        ZCX rec[-1] 0
        ZCY rec[-1] 1
        ZCZ rec[-1] 2
        XCZ 3 rec[-1]
        YCZ 4 rec[-1]
        ZCZ 5 rec[-1]
    """)) == cirq.Circuit(
        cirq.Moment(
            cirq.measure(cirq.LineQubit(0), key=cirq.MeasurementKey(name='0')),
        ),
        cirq.Moment(
            stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.X).on(cirq.LineQubit(0)),
            stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.Y).on(cirq.LineQubit(1)),
            stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.Z).on(cirq.LineQubit(2)),
            stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.X).on(cirq.LineQubit(3)),
            stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.Y).on(cirq.LineQubit(4)),
            stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.Z).on(cirq.LineQubit(5)),
        ),
    )


def test_stim_conversion():
    a, b, c = cirq.LineQubit.range(3)

    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(cirq.Moment(cirq.X(a).with_classical_controls("unknown")))
        )
    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(
                cirq.Moment(
                    cirq.X(a).with_classical_controls("unknown"), cirq.measure(b, key="later")
                )
            )
        )
    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(
                cirq.Moment(cirq.X(a).with_classical_controls("unknown")),
                cirq.Moment(cirq.measure(b, key="later")),
            )
        )
    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.Moment(cirq.measure(b, key="earlier")),
            cirq.Moment(cirq.X(b).with_classical_controls("earlier")),
        )
    ) == stim.Circuit(
        """
        QUBIT_COORDS(1) 0
        M 0
        TICK
        CX rec[-1] 0
        TICK
        """
    )

    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.Moment(cirq.measure(a, key="a"), cirq.measure(b, key="b")),
            cirq.Moment(
                cirq.X(b).with_classical_controls("a"),
            ),
            cirq.Moment(
                cirq.Z(b).with_classical_controls("b"),
            ),
        )
    ) == stim.Circuit(
        """
        M 0 1
        TICK
        CX rec[-2] 1
        TICK
        CZ rec[-1] 1
        TICK
    """
    )


def test_diagram():
    a, b = cirq.LineQubit.range(2)
    cirq.testing.assert_has_diagram(
        cirq.Circuit(
            cirq.measure(a, key="a"),
            cirq.measure(b, key="b"),
            stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli='Y').on(a),
        ),
        """
0: ---M('a')---Y^rec[-1]---

1: ---M('b')---------------
        """,
        use_unicode_characters=False,
    )


def test_repr():
    val = stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.Y)
    assert eval(repr(val), {"cirq": cirq, "stimcirq": stimcirq}) == val


def test_equality():
    eq = cirq.testing.EqualsTester()
    eq.add_equality_group(
        stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.X),
        stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.X))
    eq.add_equality_group(stimcirq.FeedbackPauli(relative_measurement_index=-1, pauli=cirq.Y))
    eq.add_equality_group(
        stimcirq.FeedbackPauli(relative_measurement_index=-4, pauli=cirq.X),
    )
    eq.add_equality_group(stimcirq.FeedbackPauli(relative_measurement_index=-10, pauli=cirq.Z))


def test_json_serialization():
    c = cirq.Circuit(
        stimcirq.FeedbackPauli(relative_measurement_index=-3, pauli=cirq.X).on(cirq.LineQubit(0)),
        stimcirq.FeedbackPauli(relative_measurement_index=-5, pauli=cirq.Y).on(cirq.LineQubit(1)),
        stimcirq.FeedbackPauli(relative_measurement_index=-7, pauli=cirq.Z).on(cirq.LineQubit(2)),
    )
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2


def test_json_backwards_compat_exact():
    raw = stimcirq.FeedbackPauli(relative_measurement_index=-3, pauli=cirq.X)
    packed = '{\n  "cirq_type": "FeedbackPauli",\n  "pauli": {\n    "cirq_type": "_PauliX",\n    "exponent": 1.0,\n    "global_shift": 0.0\n  },\n  "relative_measurement_index": -3\n}'
    assert cirq.to_json(raw) == packed
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
