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
