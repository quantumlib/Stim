import cirq
import stim
import stimcirq


def test_stim_conversion():
    a, b, c = cirq.LineQubit.range(3)

    cirq_circuit = cirq.Circuit(
        stimcirq.CXSwapGate(inverted=False).on(a, b),
        stimcirq.CXSwapGate(inverted=True).on(b, c),
    )
    stim_circuit = stim.Circuit(
        """
        CXSWAP 0 1
        TICK
        SWAPCX 1 2
        TICK
        """
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit) == stim_circuit
    assert stimcirq.stim_circuit_to_cirq_circuit(stim_circuit) == cirq_circuit


def test_diagram():
    a, b = cirq.LineQubit.range(2)
    cirq.testing.assert_has_diagram(
        cirq.Circuit(
            stimcirq.CXSwapGate(inverted=False)(a, b),
            stimcirq.CXSwapGate(inverted=True)(a, b),
        ),
        """
0: ---ZSWAP---XSWAP---
      |       |
1: ---XSWAP---ZSWAP---
        """,
        use_unicode_characters=False,
    )


def test_inverse():
    a = stimcirq.CXSwapGate(inverted=True)
    b = stimcirq.CXSwapGate(inverted=False)
    assert a**+1 == a
    assert a**-1 == b
    assert b**+1 == b
    assert b**-1 == a


def test_repr():
    val = stimcirq.CXSwapGate(inverted=True)
    assert eval(repr(val), {"stimcirq": stimcirq}) == val
    val = stimcirq.CXSwapGate(inverted=False)
    assert eval(repr(val), {"stimcirq": stimcirq}) == val


def test_equality():
    eq = cirq.testing.EqualsTester()
    eq.add_equality_group(stimcirq.CXSwapGate(inverted=False), stimcirq.CXSwapGate(inverted=False))
    eq.add_equality_group(stimcirq.CXSwapGate(inverted=True))


def test_json_serialization():
    a, b, d = cirq.LineQubit.range(3)
    c = cirq.Circuit(
        stimcirq.CXSwapGate(inverted=False)(a, b),
        stimcirq.CXSwapGate(inverted=False)(b, d),
    )
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2


def test_json_backwards_compat_exact():
    raw = stimcirq.CXSwapGate(inverted=True)
    packed = '{\n  "cirq_type": "CXSwapGate",\n  "inverted": true\n}'
    assert cirq.to_json(raw) == packed
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
