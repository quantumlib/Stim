import cirq
import stimcirq


def test_repr():
    r = stimcirq.MeasureAndOrResetGate(
        measure=True,
        reset=True,
        basis='Y',
        key='result',
        invert_measure=True,
        measure_flip_probability=0.125,
    )
    assert eval(repr(r), {'stimcirq': stimcirq}) == r


def test_json_serialization():
    r = stimcirq.MeasureAndOrResetGate(
        measure=True,
        reset=True,
        basis='Y',
        key='result',
        invert_measure=True,
        measure_flip_probability=0.125,
    )
    c = cirq.Circuit(r(cirq.LineQubit(1)))
    json = cirq.to_json(c)
    c2 = cirq.read_json(json_text=json, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER])
    assert c == c2


def test_json_backwards_compat_exact():
    raw = stimcirq.MeasureAndOrResetGate(
        measure=True,
        reset=True,
        basis='X',
        key='res',
        invert_measure=True,
        measure_flip_probability=0.25,
    )
    packed = '{\n  "cirq_type": "MeasureAndOrResetGate",\n  "measure": true,\n  "reset": true,\n  "basis": "X",\n  "invert_measure": true,\n  "key": "res",\n  "measure_flip_probability": 0.25\n}'
    assert cirq.read_json(json_text=packed, resolvers=[*cirq.DEFAULT_RESOLVERS, stimcirq.JSON_RESOLVER]) == raw
    assert cirq.to_json(raw) == packed
