from typing import Dict, Tuple, Sequence, List

import cirq
import numpy as np
import pytest
import stim

import stim_cirq
from stim_cirq._stim_sampler import (
    cirq_circuit_to_stim_data,
    gate_to_stim_append_func,
)


def solve_tableau(gate: cirq.Gate) -> Dict[cirq.PauliString, cirq.PauliString]:
    """Computes a stabilizer tableau for the given gate."""

    result = {}

    n = gate.num_qubits()
    qs = cirq.LineQubit.range(n)
    for inp in [g(q) for g in [cirq.X, cirq.Z] for q in qs]:
        # Use superdense coding to extract X and Z flips from the generator conjugated by the gate.
        c = cirq.Circuit(
            cirq.H.on_each(qs),
            [cirq.CNOT(q, q + n) for q in qs],
            gate(*qs) ** -1,
            inp,
            gate(*qs),
            [cirq.CNOT(q, q + n) for q in qs],
            cirq.H.on_each(qs),
            [cirq.measure(q, q + n, key=str(q)) for q in qs],
        )

        # Extract X/Y/Z data from sample result (which should be deterministic).
        s = cirq.Simulator().sample(c)
        out: cirq.PauliString = cirq.PauliString({q: "IXZY"[s[str(q)][0]] for q in qs})

        # Use phase kickback to determine the sign of the output stabilizer.
        sign = cirq.NamedQubit('a')
        c = cirq.Circuit(
            cirq.H(sign),
            inp.controlled_by(sign),
            gate(*qs),
            out.controlled_by(sign),
            cirq.H(sign),
            cirq.measure(sign, key='sign'),
        )
        if cirq.Simulator().sample(c)['sign'][0]:
            out *= -1

        result[inp] = out
    return result


def test_solve_tableau():
    a, b = cirq.LineQubit.range(2)
    assert solve_tableau(cirq.I) == {cirq.X(a): cirq.X(a), cirq.Z(a): cirq.Z(a)}
    assert solve_tableau(cirq.S) == {cirq.X(a): cirq.Y(a), cirq.Z(a): cirq.Z(a)}
    assert solve_tableau(cirq.S ** -1) == {cirq.X(a): -cirq.Y(a), cirq.Z(a): cirq.Z(a)}
    assert solve_tableau(cirq.H) == {cirq.X(a): cirq.Z(a), cirq.Z(a): cirq.X(a)}
    assert solve_tableau(
        cirq.SingleQubitCliffordGate.from_xz_map((cirq.Y, False), (cirq.X, False))
    ) == {cirq.X(a): cirq.Y(a), cirq.Z(a): cirq.X(a)}
    assert solve_tableau(cirq.CZ) == {
        cirq.X(a): cirq.X(a) * cirq.Z(b),
        cirq.Z(a): cirq.Z(a),
        cirq.X(b): cirq.Z(a) * cirq.X(b),
        cirq.Z(b): cirq.Z(b),
    }


def assert_unitary_gate_converts_correctly(gate: cirq.Gate):
    n = gate.num_qubits()
    for pre, post in solve_tableau(gate).items():
        # Create a circuit that measures pre before the gate times post after the gate.
        # If the gate is translated correctly, the measurement will always be zero.

        c = stim.Circuit()
        c.append_operation("H", range(n))
        for i in range(n):
            c.append_operation("CNOT", [i, i + n])
        c.append_operation("H", [2 * n])
        for q, p in pre.items():
            c.append_operation(f"C{p}", [2 * n, q.x])
        qs = cirq.LineQubit.range(n)
        conv_gate, _ = cirq_circuit_to_stim_data(cirq.Circuit(gate(*qs)), q2i={q: q.x for q in qs})
        c += conv_gate
        for q, p in post.items():
            c.append_operation(f"C{p}", [2 * n, q.x])
        if post.coefficient == -1:
            c.append_operation("Z", [2 * n])
        c.append_operation("H", [2 * n])
        c.append_operation("M", [2 * n])
        correct = not np.any(c.compile_sampler().sample_bit_packed(10))
        assert correct, f"{gate!r} failed to turn {pre} into {post}.\nConverted to:\n{conv_gate}\n"


@pytest.mark.parametrize("gate", gate_to_stim_append_func().keys())
def test_unitary_gate_conversions(gate: cirq.Gate):
    # Note: filtering in the parametrize annotation causes a false 'gate undefined' lint error.
    if cirq.has_unitary(gate):
        assert_unitary_gate_converts_correctly(gate)


def test_more_unitary_gate_conversions():
    for p in [1, 1j, -1, -1j]:
        assert_unitary_gate_converts_correctly(p * cirq.DensePauliString("IXYZ"))
        assert_unitary_gate_converts_correctly((p * cirq.DensePauliString("IXYZ")).controlled(1))

    a, b = cirq.LineQubit.range(2)
    c, _ = cirq_circuit_to_stim_data(cirq.Circuit(cirq.H(a), cirq.CNOT(a, b), cirq.measure(a, b), cirq.reset(a)))
    assert (
        str(c).strip()
        == """
# Circuit [num_qubits=2, num_measurements=2]
H 0
CX 0 1
M 0 1
R 0
    """.strip()
    )


@pytest.mark.parametrize(
    "gate",
    [
        cirq.BitFlipChannel(0.1),
        cirq.BitFlipChannel(0.2),
        cirq.PhaseFlipChannel(0.1),
        cirq.PhaseFlipChannel(0.2),
        cirq.PhaseDampingChannel(0.1),
        cirq.PhaseDampingChannel(0.2),
        cirq.X.with_probability(0.1),
        cirq.X.with_probability(0.2),
        cirq.Y.with_probability(0.1),
        cirq.Y.with_probability(0.2),
        cirq.Z.with_probability(0.1),
        cirq.Z.with_probability(0.2),
        cirq.DepolarizingChannel(0.1),
        cirq.DepolarizingChannel(0.2),
        cirq.DepolarizingChannel(0.1, n_qubits=2),
        cirq.DepolarizingChannel(0.2, n_qubits=2),
    ],
)
def test_noisy_gate_conversions(gate: cirq.Gate):
    # Create test circuit that uses superdense coding to quantify arbitrary Pauli error mixtures.
    n = gate.num_qubits()
    qs = cirq.LineQubit.range(n)
    circuit = cirq.Circuit(
        cirq.H.on_each(qs),
        [cirq.CNOT(q, q + n) for q in qs],
        gate(*qs),
        [cirq.CNOT(q, q + n) for q in qs],
        cirq.H.on_each(qs),
    )
    expected_rates = cirq.final_density_matrix(circuit).diagonal().real

    # Convert test circuit to Stim and sample from it.
    stim_circuit, _ = cirq_circuit_to_stim_data(circuit + cirq.measure(*sorted(circuit.all_qubits())[::-1]))
    sample_count = 10000
    samples = stim_circuit.compile_sampler().sample_bit_packed(sample_count).flat
    unique, counts = np.unique(samples, return_counts=True)

    # Compare sample rates to expected rates.
    for value, count in zip(unique, counts):
        expected_rate = expected_rates[value]
        actual_rate = count / sample_count
        allowed_variation = 5 * (expected_rate * (1 - expected_rate) / sample_count) ** 0.5
        if not 0 <= expected_rate - allowed_variation <= 1:
            raise ValueError("Not enough samples to bound results away from extremes.")
        assert abs(expected_rate - actual_rate) < allowed_variation, (
            f"Sample rate {actual_rate} is over 5 standard deviations away from {expected_rate}.\n"
            f"Gate: {gate}\n"
            f"Test circuit:\n{circuit}\n"
            f"Converted circuit:\n{stim_circuit}\n"
        )


def test_end_to_end():
    sampler = stim_cirq.StimSampler()
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
    s2 = stim_cirq.StimSampler().sample(circuit)
    assert s1['out'][0] == s2['out'][0]


def test_custom_gates():
    s = stim_cirq.StimSampler()
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

    s = stim_cirq.StimSampler()
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
    s = stim_cirq.StimSampler()
    a, b = cirq.LineQubit.range(2)
    s.run(cirq.Circuit((cirq.X(a) * cirq.Y(b)).with_probability(0.1)))


def test_cirq_circuit_to_stim_circuit_custom_stim_method():
    class DetectorGate(cirq.Gate):
        def _num_qubits_(self):
            return 1

        def _measure_keys_(self):
            return "custom",

        def _stim_conversion_(self,
                              edit_circuit: stim.Circuit,
                              edit_measurement_key_lengths: List[Tuple[str, int]],
                              targets: Sequence[int],
                              **kwargs):
            edit_measurement_key_lengths.append(("custom", 2))
            edit_circuit.append_operation("M", [stim.target_inv(targets[0])])
            edit_circuit.append_operation("M", [targets[0]])
            edit_circuit.append_operation("DETECTOR", [stim.target_rec(-1)])

    class SecondLastMeasurementWasDeterministicOperation(cirq.Operation):
        def _stim_conversion_(self, edit_circuit: stim.Circuit, **kwargs):
            edit_circuit.append_operation("DETECTOR", [stim.target_rec(-2)])

        def with_qubits(self, *new_qubits):
            raise NotImplementedError()

        @property
        def qubits(self) -> Tuple['cirq.Qid', ...]:
            return ()

    a, b, c = cirq.LineQubit.range(3)
    cirq_circuit = cirq.Circuit(
        cirq.measure(a, key="a"),
        cirq.measure(b, key="b"),
        cirq.measure(c, key="c"),
        cirq.Moment(SecondLastMeasurementWasDeterministicOperation()),
        cirq.Moment(DetectorGate().on(b)),
    )

    stim_circuit = stim_cirq.cirq_circuit_to_stim_circuit(cirq_circuit)
    assert str(stim_circuit).strip() == """
# Circuit [num_qubits=3, num_measurements=5]
M 0 1 2
DETECTOR rec[-2]
M !1 1
DETECTOR rec[-1]
    """.strip()

    class BadGate(cirq.Gate):
        def num_qubits(self) -> int:
            return 1

        def _stim_conversion_(self):
            pass

    with pytest.raises(TypeError, match="dont_forget_your_star_star_kwargs"):
        stim_cirq.cirq_circuit_to_stim_circuit(cirq.Circuit(BadGate().on(a)))

    sample = stim_cirq.StimSampler().sample(cirq_circuit)
    assert len(sample.columns) == 4
    np.testing.assert_array_equal(sample["a"], [0])
    np.testing.assert_array_equal(sample["b"], [0])
    np.testing.assert_array_equal(sample["c"], [0])
    np.testing.assert_array_equal(sample["custom"], [2])
