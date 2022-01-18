import cirq
import numpy as np
import pytest
import stim
import stimcirq


def test_stim_conversion():
    a, b, c = cirq.LineQubit.range(3)

    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(
                cirq.Moment(
                    stimcirq.CumulativeObservableAnnotation(
                        parity_keys=["unknown"], observable_index=0
                    )
                )
            )
        )
    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(
                cirq.Moment(
                    stimcirq.CumulativeObservableAnnotation(
                        parity_keys=["later"], observable_index=0
                    ),
                    cirq.measure(b, key="later"),
                )
            )
        )
    with pytest.raises(ValueError, match="earlier"):
        stimcirq.cirq_circuit_to_stim_circuit(
            cirq.Circuit(
                cirq.Moment(
                    stimcirq.CumulativeObservableAnnotation(
                        parity_keys=["later"], observable_index=0
                    )
                ),
                cirq.Moment(cirq.measure(b, key="later")),
            )
        )
    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.Moment(cirq.measure(b, key="earlier")),
            cirq.Moment(
                stimcirq.CumulativeObservableAnnotation(parity_keys=["earlier"], observable_index=0)
            ),
        )
    ) == stim.Circuit(
        """
        QUBIT_COORDS(1) 0
        M 0
        TICK
        OBSERVABLE_INCLUDE(0) rec[-1]
        TICK
    """
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.Moment(
                cirq.measure(b, key="earlier"),
                stimcirq.CumulativeObservableAnnotation(
                    parity_keys=["earlier"], observable_index=0
                ),
            )
        )
    ) == stim.Circuit(
        """
        QUBIT_COORDS(1) 0
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1]
        TICK
    """
    )

    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.Moment(cirq.measure(a, key="a"), cirq.measure(b, key="b")),
            cirq.Moment(
                stimcirq.CumulativeObservableAnnotation(parity_keys=["a", "b"], observable_index=2)
            ),
        )
    ) == stim.Circuit(
        """
        M 0 1
        TICK
        OBSERVABLE_INCLUDE(2) rec[-1] rec[-2]
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
                stimcirq.CumulativeObservableAnnotation(parity_keys=["a", "b"], observable_index=0),
                cirq.measure(c, key="c"),
                stimcirq.CumulativeObservableAnnotation(parity_keys=["a", "c"], observable_index=0),
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
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
        M 2
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-3]
        TICK
    """
    )


def test_simulation():
    a = cirq.LineQubit(0)
    s = cirq.Simulator().sample(
        cirq.Circuit(
            cirq.X(a),
            cirq.measure(a, key="a"),
            stimcirq.CumulativeObservableAnnotation(parity_keys=["a"], observable_index=0),
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
            stimcirq.CumulativeObservableAnnotation(parity_keys=["a", "b"], observable_index=2),
        ),
        """
0: ---M('a')----------

1: ---M('b')----------
      Obs2('a','b')
        """,
        use_unicode_characters=False,
    )


def test_repr():
    val = stimcirq.CumulativeObservableAnnotation(parity_keys=["a", "b"], observable_index=2)
    assert eval(repr(val), {"stimcirq": stimcirq}) == val


def test_equality():
    eq = cirq.testing.EqualsTester()
    eq.add_equality_group(
        stimcirq.CumulativeObservableAnnotation(observable_index=0),
        stimcirq.CumulativeObservableAnnotation(observable_index=0),
    )
    eq.add_equality_group(stimcirq.CumulativeObservableAnnotation(observable_index=1))
    eq.add_equality_group(
        stimcirq.CumulativeObservableAnnotation(parity_keys=["a"], observable_index=0)
    )
    eq.add_equality_group(
        stimcirq.CumulativeObservableAnnotation(parity_keys=["a"], observable_index=1)
    )
    eq.add_equality_group(
        stimcirq.CumulativeObservableAnnotation(parity_keys=["b"], observable_index=0)
    )
    eq.add_equality_group(
        stimcirq.CumulativeObservableAnnotation(parity_keys=["a", "b"], observable_index=0),
        stimcirq.CumulativeObservableAnnotation(parity_keys=["b", "a"], observable_index=0),
    )


def test_json_serialization():
    c = cirq.Circuit(
        stimcirq.CumulativeObservableAnnotation(parity_keys=["a", "b"], observable_index=5),
        stimcirq.CumulativeObservableAnnotation(
            parity_keys=["a", "b"], relative_keys=[-1, -3], observable_index=5
        ),
        stimcirq.CumulativeObservableAnnotation(observable_index=2),
        stimcirq.CumulativeObservableAnnotation(parity_keys=["d", "c"], observable_index=5),
    )
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2
