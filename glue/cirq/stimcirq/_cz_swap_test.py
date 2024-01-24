import cirq
import stim
import stimcirq


def test_stim_conversion():
    a, b, c = cirq.LineQubit.range(3)

    cirq_circuit = cirq.Circuit(
        stimcirq.CZSwapGate().on(a, b),
        stimcirq.CZSwapGate().on(b, c),
    )
    stim_circuit = stim.Circuit(
        """
        CZSWAP 0 1
        TICK
        CZSWAP 1 2
        TICK
        """
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit) == stim_circuit
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit) == cirq_circuit


def test_diagram():
    a, b = cirq.LineQubit.range(2)
    cirq.testing.assert_has_diagram(
        cirq.Circuit(
            stimcirq.CZSwapGate()(a, b),
            stimcirq.CZSwapGate()(a, b),
        ),
        """
0: ---ZSWAP---ZSWAP---
      |       |
1: ---ZSWAP---ZSWAP---
        """,
        use_unicode_characters=False,
    )


def test_inverse():
    a = stimcirq.CZSwapGate()
    assert a**+1 == a
    assert a**-1 == a


def test_repr():
    val = stimcirq.CZSwapGate()
    assert eval(repr(val), {"stimcirq": stimcirq}) == val


def test_equality():
    eq = cirq.testing.EqualsTester()
    eq.add_equality_group(stimcirq.CZSwapGate(), stimcirq.CZSwapGate())


def test_json_serialization():
    a, b, d = cirq.LineQubit.range(3)
    c = cirq.Circuit(
        stimcirq.CZSwapGate()(a, b),
        stimcirq.CZSwapGate()(b, d),
    )
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2


def test_json_backwards_compat_exact():
    raw = stimcirq.CZSwapGate()
    packed = '{\n  "cirq_type": "CZSwapGate"\n}'
    assert cirq.to_json(raw) == packed
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
