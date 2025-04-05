import cirq
import stimcirq


def test_repr():
    r = stimcirq.IErrorGate([0.25, 0.125])
    assert eval(repr(r), {'stimcirq': stimcirq}) == r
    r = stimcirq.IErrorGate([])
    assert eval(repr(r), {'stimcirq': stimcirq}) == r


def test_json_serialization():
    r = stimcirq.IErrorGate([0.25, 0.125])
    c = cirq.Circuit(r(cirq.LineQubit(1)))
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2


def test_json_backwards_compat_exact():
    raw = stimcirq.IErrorGate([0.25, 0.125])
    packed = '{\n  "cirq_type": "IErrorGate",\n  "gate_args": [\n    0.25,\n    0.125\n  ]\n}'
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
    assert cirq.to_json(raw) == packed
