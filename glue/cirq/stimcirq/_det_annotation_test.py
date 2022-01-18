import cirq
import numpy as np
import pytest
import stim
import stimcirq


def test_stim_conversion():
    a, b, c = cirq.LineQubit.range(3)

    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(cirq.Moment(stimcirq.DetAnnotation(parity_keys=["unknown"])))
        )
    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(
                cirq.Moment(
                    stimcirq.DetAnnotation(parity_keys=["later"]), cirq.measure(b, key="later")
                )
            )
        )
    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(
                cirq.Moment(stimcirq.DetAnnotation(parity_keys=["later"])),
                cirq.Moment(cirq.measure(b, key="later")),
            )
        )
    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.Moment(cirq.measure(b, key="earlier")),
            cirq.Moment(stimcirq.DetAnnotation(parity_keys=["earlier"])),
        )
    ) == stim.Circuit(
        """
        QUBIT_COORDS(1) 0
        M 0
        TICK
        DETECTOR rec[-1]
        TICK
    """
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.Moment(
                cirq.measure(b, key="earlier"), stimcirq.DetAnnotation(parity_keys=["earlier"])
            )
        )
    ) == stim.Circuit(
        """
        QUBIT_COORDS(1) 0
        M 0
        DETECTOR rec[-1]
        TICK
    """
    )

    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.Moment(cirq.measure(a, key="a"), cirq.measure(b, key="b")),
            cirq.Moment(
                stimcirq.DetAnnotation(parity_keys=["a", "b"], coordinate_metadata=(1, 2, 3.5))
            ),
        )
    ) == stim.Circuit(
        """
        M 0 1
        TICK
        DETECTOR(1, 2, 3.5) rec[-1] rec[-2]
        TICK
    """
    )

    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.H(a),
            cirq.CNOT(a, b),
            cirq.CNOT(a, c),
            cirq.Moment(
                cirq.measure(a, key="a"),
                cirq.measure(b, key="b"),
                stimcirq.DetAnnotation(parity_keys=["a", "b"]),
                cirq.measure(c, key="c"),
                stimcirq.DetAnnotation(parity_keys=["a", "c"]),
            ),
        )
    ) == stim.Circuit(
        """
        H 0
        TICK
        CNOT 0 1
        TICK
        CNOT 0 2
        TICK
        M 0 1
        DETECTOR rec[-1] rec[-2]
        M 2
        DETECTOR rec[-1] rec[-3]
        TICK
    """
    )


def test_simulation():
    a = cirq.LineQubit(0)
    s = cirq.Simulator().sample(
        cirq.Circuit(
            cirq.X(a), cirq.measure(a, key="a"), stimcirq.DetAnnotation(parity_keys=["a"])
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
            stimcirq.DetAnnotation(parity_keys=["a", "b"]),
        ),
        """
0: ---M('a')---------

1: ---M('b')---------
      Det('a','b')
        """,
        use_unicode_characters=False,
    )


def test_repr():
    val = stimcirq.DetAnnotation(parity_keys=["a", "b"], coordinate_metadata=(1, 2))
    assert eval(repr(val), {"stimcirq": stimcirq}) == val


def test_equality():
    eq = cirq.testing.EqualsTester()
    eq.add_equality_group(stimcirq.DetAnnotation(), stimcirq.DetAnnotation())
    eq.add_equality_group(stimcirq.DetAnnotation(parity_keys=["a"]))
    eq.add_equality_group(
        stimcirq.DetAnnotation(parity_keys=["a"], coordinate_metadata=[1, 2]),
        stimcirq.DetAnnotation(parity_keys=["a"], coordinate_metadata=(1, 2)),
    )
    eq.add_equality_group(stimcirq.DetAnnotation(parity_keys=["a"], coordinate_metadata=(2, 1)))
    eq.add_equality_group(stimcirq.DetAnnotation(parity_keys=["b"]))
    eq.add_equality_group(
        stimcirq.DetAnnotation(parity_keys=["a", "b"]),
        stimcirq.DetAnnotation(parity_keys=["b", "a"]),
    )


def test_json_serialization():
    c = cirq.Circuit(
        stimcirq.DetAnnotation(parity_keys=["a", "b"]),
        stimcirq.DetAnnotation(parity_keys=["a", "b"], relative_keys=[-3, -5]),
        stimcirq.DetAnnotation(coordinate_metadata=(1, 2)),
        stimcirq.DetAnnotation(parity_keys=["d", "c"], coordinate_metadata=(1, 2, 3)),
    )
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2
