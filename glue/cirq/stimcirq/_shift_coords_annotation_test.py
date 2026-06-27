import cirq
import numpy as np
import stim
import stimcirq


def test_conversion():
    stim_circuit = stim.Circuit("""
        M 0
        DETECTOR(4) rec[-1]
        SHIFT_COORDS(1, 2, 3)
        DETECTOR(5) rec[-1]
        TICK
    """)
    cirq_circuit = cirq.Circuit(
        cirq.measure(cirq.LineQubit(0), key="0"),
        stimcirq.DetAnnotation(parity_keys=["0"], coordinate_metadata=[4]),
        stimcirq.ShiftCoordsAnnotation((1, 2, 3)),
        stimcirq.DetAnnotation(parity_keys=["0"], coordinate_metadata=[5]),
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit) == stim_circuit
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit) == cirq_circuit
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit, flatten=False) == cirq_circuit
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit, flatten=True) == cirq.Circuit(
        cirq.measure(cirq.LineQubit(0), key="0"),
        stimcirq.DetAnnotation(parity_keys=["0"], coordinate_metadata=[4]),
        stimcirq.DetAnnotation(parity_keys=["0"], coordinate_metadata=[6]),
    )


def test_simulation():
    a = cirq.LineQubit(0)
    s = cirq.Simulator().sample(
        cirq.Circuit(
            cirq.X(a), cirq.measure(a, key="a"), stimcirq.ShiftCoordsAnnotation((1, 2, 3))
        ),
        repetitions=3,
    )
    np.testing.assert_array_equal(s["a"], [1, 1, 1])


def test_diagram():
    a, b = cirq.LineQubit.range(2)
    cirq.testing.assert_has_diagram(
        cirq.Circuit(
            cirq.measure(a, key="a"),
            cirq.measure(b, key="b"),
            stimcirq.ShiftCoordsAnnotation([1, 2, 3]),
        ),
        """
0: ---M('a')---------------

1: ---M('b')---------------
      ShiftCoords(1,2,3)
        """,
        use_unicode_characters=False,
    )


def test_repr():
    val = stimcirq.ShiftCoordsAnnotation([1, 2, 3])
    assert eval(repr(val), {"stimcirq": stimcirq}) == val


def test_equality():
    eq = cirq.testing.EqualsTester()
    eq.add_equality_group(stimcirq.ShiftCoordsAnnotation([]))
    eq.add_equality_group(
        stimcirq.ShiftCoordsAnnotation([1, 2]), stimcirq.ShiftCoordsAnnotation((1, 2))
    )


def test_json_serialization():
    c = cirq.Circuit(stimcirq.ShiftCoordsAnnotation([1, 2, 3]))
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2


def test_json_backwards_compat_exact():
    raw = stimcirq.ShiftCoordsAnnotation([2, 3, 5])
    packed = '{\n  "cirq_type": "ShiftCoordsAnnotation",\n  "shift": [\n    2,\n    3,\n    5\n  ]\n}'
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
    assert cirq.to_json(raw) == packed
