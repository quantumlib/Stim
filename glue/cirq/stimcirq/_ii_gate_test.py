import cirq
import stimcirq


def test_repr():
    r = stimcirq.IIGate()
    assert eval(repr(r), {'stimcirq': stimcirq}) == r


def test_json_serialization():
    r = stimcirq.IIGate()
    c = cirq.Circuit(r(cirq.LineQubit(1), cirq.LineQubit(3)))
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2


def test_json_backwards_compat_exact():
    raw = stimcirq.IIGate()
    packed = '{\n  "cirq_type": "IIGate"\n}'
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
    assert cirq.to_json(raw) == packed
