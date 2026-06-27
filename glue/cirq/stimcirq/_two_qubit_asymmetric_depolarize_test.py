import cirq
import stim
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


def test_json_backwards_compat_exact():
    raw = stimcirq.TwoQubitAsymmetricDepolarizingChannel([0.0125, 0.1, 0, 0.23, 0, 0, 0.0375, 0, 0.01, 0, 0, 0, 0, 0.25, 0])
    packed = '{\n  "cirq_type": "TwoQubitAsymmetricDepolarizingChannel",\n  "probabilities": [\n    0.0125,\n    0.1,\n    0,\n    0.23,\n    0,\n    0,\n    0.0375,\n    0,\n    0.01,\n    0,\n    0,\n    0,\n    0,\n    0.25,\n    0\n  ]\n}'
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
    assert cirq.to_json(raw) == packed


def test_native_cirq_gate_converts():
    c = cirq.Circuit(cirq.asymmetric_depolarize(
        error_probabilities={
            'IX': 0.125,
            'ZY': 0.25
        }).on(cirq.LineQubit(0), cirq.LineQubit(1)))
    s = stim.Circuit("""
        PAULI_CHANNEL_2(0.125, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.25, 0) 0 1
        TICK
    """)
    assert stimcirq.cirq_circuit_to_stim_circuit(c) == s
    assert stimcirq.stim_circuit_to_cirq_circuit(s) == c
