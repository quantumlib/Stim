import itertools
from typing import Dict, List, Sequence, Tuple, Union

import cirq
import numpy as np
import pytest
import stim
import stimcirq
from stimcirq._cirq_to_stim import cirq_circuit_to_stim_data, gate_to_stim_append_func


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
        c.append("H", range(n))
        for i in range(n):
            c.append("CNOT", [i, i + n])
        c.append("H", [2 * n])
        for q, p in pre.items():
            c.append(f"C{p}", [2 * n, q.x])
        qs = cirq.LineQubit.range(n)
        conv_gate, _ = cirq_circuit_to_stim_data(cirq.Circuit(gate(*qs)), q2i={q: q.x for q in qs})
        c += conv_gate
        for q, p in post.items():
            c.append(f"C{p}", [2 * n, q.x])
        if post.coefficient == -1:
            c.append("Z", [2 * n])
        c.append("H", [2 * n])
        c.append("M", [2 * n])
        correct = np.count_nonzero(c.compile_sampler().sample_bit_packed(10)) == 0
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
    c, _ = cirq_circuit_to_stim_data(
        cirq.Circuit(cirq.H(a), cirq.CNOT(a, b), cirq.measure(a, b), cirq.reset(a))
    )
    assert (
        str(c).strip()
        == """
H 0
TICK
CX 0 1
TICK
M 0 1
TICK
R 0
TICK
    """.strip()
    )


ROUND_TRIP_NOISY_GATES = [
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
    cirq.AsymmetricDepolarizingChannel(p_x=0, p_y=0, p_z=0),
    cirq.AsymmetricDepolarizingChannel(p_x=0.2, p_y=0.1, p_z=0.3),
    cirq.AsymmetricDepolarizingChannel(p_x=0.1, p_y=0, p_z=0),
    cirq.AsymmetricDepolarizingChannel(p_x=0, p_y=0.1, p_z=0),
    cirq.AsymmetricDepolarizingChannel(p_x=0, p_y=0, p_z=0.1),
    *[
        cirq.asymmetric_depolarize(error_probabilities={a + b: 0.1})
        for a, b in list(itertools.product('IXYZ', repeat=2))[1:]
    ],
    cirq.asymmetric_depolarize(error_probabilities={'IX': 0.125, 'ZY': 0.375}),
]


@pytest.mark.parametrize("gate", ROUND_TRIP_NOISY_GATES)
def test_frame_simulator_sampling_noisy_gates_agrees_with_cirq_data(gate: cirq.Gate):
    # Create test circuit that uses superdense coding to quantify arbitrary Pauli error mixtures.
    n = cirq.num_qubits(gate)
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
    stim_circuit, _ = cirq_circuit_to_stim_data(
        circuit + cirq.measure(*sorted(circuit.all_qubits())[::-1])
    )
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


@pytest.mark.parametrize("gate", ROUND_TRIP_NOISY_GATES)
def test_tableau_simulator_sampling_noisy_gates_agrees_with_cirq_data(gate: cirq.Gate):
    # Technically this be a test of the `stim` package itself, but it's so convenient to compare to cirq.

    # Create test circuit that uses superdense coding to quantify arbitrary Pauli error mixtures.
    n = cirq.num_qubits(gate)
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
    stim_circuit, _ = cirq_circuit_to_stim_data(
        circuit + cirq.measure(*sorted(circuit.all_qubits())[::-1])
    )
    sample_count = 10000
    samples = []
    for _ in range(sample_count):
        sim = stim.TableauSimulator()
        sim.do(stim_circuit)
        s = 0
        for k, v in enumerate(sim.current_measurement_record()):
            s |= v << k
        samples.append(s)

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


def test_cirq_circuit_to_stim_circuit_custom_stim_method():
    class DetectorGate(cirq.Gate):
        def _num_qubits_(self):
            return 1

        def _measure_keys_(self):
            return ("custom",)

        def _stim_conversion_(
            self,
            edit_circuit: stim.Circuit,
            edit_measurement_key_lengths: List[Tuple[str, int]],
            targets: Sequence[int],
            **kwargs,
        ):
            edit_measurement_key_lengths.append(("custom", 2))
            edit_circuit.append("M", [stim.target_inv(targets[0])])
            edit_circuit.append("M", [targets[0]])
            edit_circuit.append("DETECTOR", [stim.target_rec(-1)])

    class SecondLastMeasurementWasDeterministicOperation(cirq.Operation):
        def _stim_conversion_(self, edit_circuit: stim.Circuit, tag: str, **kwargs):
            edit_circuit.append("DETECTOR", [stim.target_rec(-2)], tag=tag)

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

    stim_circuit = stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit)
    assert (
        str(stim_circuit).strip()
        == """
M 0 1 2
TICK
DETECTOR rec[-2]
TICK
M !1 1
DETECTOR rec[-1]
TICK
    """.strip()
    )

    class BadGate(cirq.Gate):
        def num_qubits(self) -> int:
            return 1

        def _stim_conversion_(self):
            pass

    with pytest.raises(TypeError, match="dont_forget_your_star_star_kwargs"):
        stimcirq.cirq_circuit_to_stim_circuit(cirq.Circuit(BadGate().on(a)))

    sample = stimcirq.StimSampler().sample(cirq_circuit)
    assert len(sample.columns) == 4
    np.testing.assert_array_equal(sample["a"], [0])
    np.testing.assert_array_equal(sample["b"], [0])
    np.testing.assert_array_equal(sample["c"], [0])
    np.testing.assert_array_equal(sample["custom"], [2])


def test_custom_qubit_indexing():
    a = cirq.NamedQubit("a")
    b = cirq.NamedQubit("b")
    actual = stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(cirq.CNOT(a, b)), qubit_to_index_dict={a: 10, b: 15}
    )
    assert actual == stim.Circuit('CX 10 15\nTICK')
    actual = stimcirq.cirq_circuit_to_stim_circuit(
        cirq.FrozenCircuit(cirq.CNOT(a, b)), qubit_to_index_dict={a: 10, b: 15}
    )
    assert actual == stim.Circuit('CX 10 15\nTICK')


def test_on_loop():
    a, b = cirq.LineQubit.range(2)
    c = cirq.Circuit(
        cirq.CircuitOperation(
            cirq.FrozenCircuit(
                cirq.X(a),
                cirq.X(b),
                cirq.measure(a, key="a"),
                cirq.measure(b, key="b"),
            ),
            repetitions=3,
        )
    )
    result = stimcirq.StimSampler().run(c)
    assert result.measurements.keys() == {'0:a', '0:b', '1:a', '1:b', '2:a', '2:b'}


def test_multi_moment_circuit_operation():
    q0 = cirq.LineQubit(0)
    cc = cirq.Circuit(
        cirq.CircuitOperation(
            cirq.FrozenCircuit(
                cirq.Moment(cirq.H(q0)),
                cirq.Moment(cirq.H(q0)),
                cirq.Moment(cirq.H(q0)),
                cirq.Moment(cirq.H(q0)),
            )
        )
    )
    assert stimcirq.cirq_circuit_to_stim_circuit(cc) == stim.Circuit("""
        H 0
        TICK
        H 0
        TICK
        H 0
        TICK
        H 0
        TICK
    """)


def test_on_tagged_loop():
    a, b = cirq.LineQubit.range(2)
    c = cirq.Circuit(
        cirq.CircuitOperation(
            cirq.FrozenCircuit(
                cirq.X(a),
                cirq.X(b),
                cirq.measure(a, key="a"),
                cirq.measure(b, key="b"),
            ),
            repetitions=3,
        ).with_tags('my_tag')
    )

    stim_circuit = stimcirq.cirq_circuit_to_stim_circuit(c)
    assert stim.CircuitRepeatBlock in {type(instr) for instr in stim_circuit}


def test_random_gate_channel():
    q0, q1 = cirq.LineQubit.range(2)

    circuit = cirq.Circuit(cirq.RandomGateChannel(
        sub_gate=cirq.DensePauliString((0, 1)),
        probability=0.25).on(q0, q1))
    assert stimcirq.cirq_circuit_to_stim_circuit(circuit) == stim.Circuit("""
        E(0.25) X1
        TICK
    """)


def test_custom_tagging():
    assert stimcirq.cirq_circuit_to_stim_circuit(
        cirq.Circuit(
            cirq.X(cirq.LineQubit(0)).with_tags('test'),
            cirq.X(cirq.LineQubit(0)).with_tags((2, 3, 4)),
            cirq.H(cirq.LineQubit(0)).with_tags('a', 'b'),
        ),
        tag_func=lambda op: "PAIR" if len(op.tags) == 2 else repr(op.tags),
    ) == stim.Circuit("""
        X[('test',)] 0
        TICK
        X[((2, 3, 4),)] 0
        TICK
        H[PAIR] 0
        TICK
    """)


def test_round_trip_example_circuit():
    stim_circuit = stim.Circuit.generated(
        "surface_code:rotated_memory_x",
        distance=3,
        rounds=1,
        after_clifford_depolarization=0.01,
    )
    cirq_circuit = stimcirq.stim_circuit_to_cirq_circuit(stim_circuit.flattened())
    circuit_back = stimcirq.cirq_circuit_to_stim_circuit(cirq_circuit)
    assert len(circuit_back.shortest_graphlike_error()) == 3
