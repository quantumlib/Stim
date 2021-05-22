import cirq
import numpy as np
import pytest

import stimcirq


def test_end_to_end():
    sampler = stimcirq.StimSampler()
    a, b = cirq.LineQubit.range(2)
    result = sampler.run(
        cirq.Circuit(
            cirq.H(a),
            cirq.CNOT(a, b),
            cirq.X(a) ** 0.5,
            cirq.X(b) ** 0.5,
            cirq.measure(a, key='a'),
            cirq.measure(b, key='b'),
        ),
        repetitions=1000,
    )
    np.testing.assert_array_equal(result.measurements['a'], result.measurements['b'] ^ 1)


def test_endian():
    a, b = cirq.LineQubit.range(2)
    circuit = cirq.Circuit(cirq.X(a), cirq.measure(a, b, key='out'))
    s1 = cirq.Simulator().sample(circuit)
    s2 = stimcirq.StimSampler().sample(circuit)
    assert s1['out'][0] == s2['out'][0]


def test_custom_gates():
    s = stimcirq.StimSampler()
    a, b, c, d = cirq.LineQubit.range(4)

    class GoodGate(cirq.SingleQubitGate):
        def _num_qubits_(self) -> int:
            return 1

        def _decompose_(self, qubits):
            return [cirq.X.on_each(*qubits)]

    class IndirectlyGoodGate(cirq.SingleQubitGate):
        def _decompose_(self, qubits):
            return [GoodGate().on_each(*qubits)]

    class BadGate(cirq.SingleQubitGate):
        pass

    class IndirectlyBadGate(cirq.SingleQubitGate):
        def _decompose_(self):
            return [BadGate()]

    np.testing.assert_array_equal(
        s.sample(cirq.Circuit(GoodGate().on(a), cirq.measure(a, key='out')))['out'], [True]
    )

    np.testing.assert_array_equal(
        s.sample(cirq.Circuit(IndirectlyGoodGate().on(a), cirq.measure(a, key='out')))['out'],
        [True],
    )

    with pytest.raises(TypeError, match="BadGate.+into stim"):
        s.sample(cirq.Circuit(BadGate().on(a)))

    with pytest.raises(TypeError, match="IndirectlyBadGate.+into stim"):
        s.sample(cirq.Circuit(IndirectlyBadGate().on(a)))

    with pytest.raises(TypeError, match="into stim"):
        s.sample(cirq.Circuit(cirq.TOFFOLI(a, b, c)))


def test_custom_measurement():
    class XBasisMeasurement(cirq.Gate):
        def __init__(self, key: str):
            self.key = key

        def _num_qubits_(self) -> int:
            return 1

        def _decompose_(self, qubits):
            q, = qubits
            return [cirq.H(q), cirq.measure(q, key=self.key), cirq.H(q)]

    s = stimcirq.StimSampler()
    a, b = cirq.LineQubit.range(2)
    out = s.sample(cirq.Circuit(
        cirq.H(a),
        cirq.X(b),
        cirq.H(b),
        XBasisMeasurement("a").on(a),
        XBasisMeasurement("b").on(b),
    ))
    np.testing.assert_array_equal(out["a"], [0])
    np.testing.assert_array_equal(out["b"], [1])


def test_correlated_error():
    s = stimcirq.StimSampler()
    a, b = cirq.LineQubit.range(2)
    s.run(cirq.Circuit((cirq.X(a) * cirq.Y(b)).with_probability(0.1)))
