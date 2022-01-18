import cirq
import stimcirq


def test_mixture():
    r = stimcirq.TwoQubitAsymmetricDepolarizingChannel(
        [0.125, 0, 0, 0, 0, 0, 0.375, 0, 0, 0, 0, 0, 0, 0.25, 0]
    )
    assert r._dense_mixture_() == [
        (0.25, cirq.DensePauliString("II")),
        (0.125, cirq.DensePauliString("IX")),
        (0.375, cirq.DensePauliString("XZ")),
        (0.25, cirq.DensePauliString("ZY")),
    ]


def test_diagram():
    r = stimcirq.TwoQubitAsymmetricDepolarizingChannel(
        [0.125, 0, 0, 0, 0, 0, 0.375, 0, 0, 0, 0, 0, 0, 0.25, 0]
    )
    cirq.testing.assert_has_diagram(
        cirq.Circuit(r.on(*cirq.LineQubit.range(2))),
        """
0: ---PauliMix(II:0.25,IX:0.125,XZ:0.375,ZY:0.25)---
      |
1: ---#2--------------------------------------------
        """,
        use_unicode_characters=False,
    )


def test_repr():
    r = stimcirq.TwoQubitAsymmetricDepolarizingChannel(
        [0.125, 0, 0, 0, 0, 0, 0.375, 0, 0, 0, 0, 0, 0, 0.25, 0]
    )
    assert eval(repr(r), {'stimcirq': stimcirq}) == r


def test_json_serialization():
    r = stimcirq.TwoQubitAsymmetricDepolarizingChannel(
        [0.0125, 0.1, 0, 0.23, 0, 0, 0.0375, 0, 0, 0, 0, 0, 0, 0.25, 0]
    )
    c = cirq.Circuit(r(cirq.LineQubit(0), cirq.LineQubit(1)))
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2
