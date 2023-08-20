import collections

import pytest
import stim
import numpy as np


def test_get_measurement_flips():
    s = stim.FlipSimulator(batch_size=11)
    assert s.num_measurements == 0
    assert s.num_qubits == 0
    assert s.batch_size == 11

    s.do(stim.Circuit("""
        X_ERROR(1) 100
        M 100
    """))
    assert s.num_measurements == 1
    assert s.num_qubits == 101
    assert s.batch_size == 11

    m = s.get_measurement_flips(record_index=0)
    np.testing.assert_array_equal(m, [True] * 11)
    assert s.num_measurements == 1
    assert s.num_qubits == 101
    assert s.batch_size == 11


def test_stabilizer_randomization():
    s = stim.FlipSimulator(
        batch_size=256,
        num_qubits=10,
        disable_stabilizer_randomization=True,
    )
    assert s.peek_pauli_flips() == [stim.PauliString(10)] * 256
    s.do(stim.Circuit("R 19"))
    assert s.peek_pauli_flips() == [stim.PauliString(20)] * 256

    s = stim.FlipSimulator(
        batch_size=256,
        num_qubits=10,
    )
    v = np.array([list(p) for p in s.peek_pauli_flips()], dtype=np.uint8)
    assert v.shape == (256, 10)
    assert np.all((v == 0) | (v == 3))
    assert 0.2 < np.count_nonzero(v == 3) / (256 * 10) < 0.8

    s = stim.FlipSimulator(
        batch_size=256,
        num_qubits=10,
        disable_stabilizer_randomization=False,
    )
    v = np.array([list(p) for p in s.peek_pauli_flips()], dtype=np.uint8)
    assert v.shape == (256, 10)
    assert np.all((v == 0) | (v == 3))
    assert 0.2 < np.count_nonzero(v == 3) / (256 * 10) < 0.8
    s.do(stim.Circuit("R 19"))
    v = np.array([list(p) for p in s.peek_pauli_flips()], dtype=np.uint8)
    assert v.shape == (256, 20)
    assert np.all((v == 0) | (v == 3))
    assert 0.2 < np.count_nonzero(v == 3) / (256 * 20) < 0.8


def test_get_detector_flips():
    s = stim.FlipSimulator(batch_size=11)
    assert s.num_measurements == 0
    assert s.num_qubits == 0
    assert s.batch_size == 11

    s.do(stim.Circuit("""
        X_ERROR(1) 25
        M 24 25
        DETECTOR rec[-1]
        DETECTOR rec[-1]
        DETECTOR rec[-1]
    """))
    assert s.num_measurements == 2
    assert s.num_detectors == 3
    assert s.num_qubits == 26
    assert s.batch_size == 11
    np.testing.assert_array_equal(
        s.get_detector_flips(),
        np.ones(shape=(3, 11), dtype=np.bool_))
    np.testing.assert_array_equal(
        s.get_detector_flips(detector_index=1),
        np.ones(shape=(11,), dtype=np.bool_))
    np.testing.assert_array_equal(
        s.get_detector_flips(instance_index=1),
        np.ones(shape=(3,), dtype=np.bool_))
    assert s.get_detector_flips(detector_index=1, instance_index=1)


def test_get_observable_flips():
    s = stim.FlipSimulator(batch_size=11)
    assert s.num_measurements == 0
    assert s.num_qubits == 0
    assert s.batch_size == 11
    assert s.num_observables == 0

    s.do(stim.Circuit("""
        X_ERROR(1) 25
        M 24 25
        OBSERVABLE_INCLUDE(2) rec[-1]
    """))
    assert s.num_measurements == 2
    assert s.num_detectors == 0
    assert s.num_observables == 3
    assert s.num_qubits == 26
    assert s.batch_size == 11
    np.testing.assert_array_equal(
        s.get_observable_flips(observable_index=1),
        np.zeros(shape=(11,), dtype=np.bool_))
    np.testing.assert_array_equal(
        s.get_observable_flips(observable_index=2),
        np.ones(shape=(11,), dtype=np.bool_))
    np.testing.assert_array_equal(
        s.get_observable_flips(instance_index=1),
        [False, False, True])
    assert not s.get_observable_flips(observable_index=1, instance_index=1)
    assert s.get_observable_flips(observable_index=2, instance_index=1)


def test_peek_pauli_flips():
    sim = stim.FlipSimulator(batch_size=500, disable_stabilizer_randomization=True)
    sim.do(stim.Circuit("""
        X_ERROR(0.3) 1
        Y_ERROR(0.3) 2
        Z_ERROR(0.3) 3
        DEPOLARIZE1(0.3) 4
    """))
    assert sim.num_qubits == 5
    assert sim.batch_size == 500
    flips = sim.peek_pauli_flips()
    assert len(flips) == 500
    assert len(flips[0]) == 5
    v0 = collections.Counter([p[0] for p in flips])
    v1 = collections.Counter([p[1] for p in flips])
    v2 = collections.Counter([p[2] for p in flips])
    v3 = collections.Counter([p[3] for p in flips])
    v4 = collections.Counter([p[4] for p in flips])
    assert v0.keys() == {0}
    assert v1.keys() == {0, 1}
    assert v2.keys() == {0, 2}
    assert v3.keys() == {0, 3}
    assert v4.keys() == {0, 1, 2, 3}
    assert v0[0] == 500
    assert 250 < v1[0] < 450
    assert 250 < v2[0] < 450
    assert 250 < v3[0] < 450
    assert 250 < v4[0] < 450


def test_surface_code():
    circuit = stim.Circuit.generated(
        "surface_code:rotated_memory_x",
        distance=3,
        rounds=5,
        before_round_data_depolarization=1e-2,
        before_measure_flip_probability=1e-2,
        after_reset_flip_probability=1e-2,
        after_clifford_depolarization=1e-2,
    )

    # Find the index of the final MR layer.
    mr_layer = len(circuit) - 1
    while circuit[mr_layer].name != 'MR':
        mr_layer -= 1
    circuit_before_mr = circuit[:mr_layer]

    sim = stim.FlipSimulator(batch_size=256, disable_stabilizer_randomization=True)
    sim.do(circuit_before_mr)
    for b in sim.peek_pauli_flips():
        print(list(b))


def test_set_pauli_flip():
    sim = stim.FlipSimulator(
        batch_size=2,
        disable_stabilizer_randomization=True,
        num_qubits=3,
    )
    assert sim.peek_pauli_flips() == [
        stim.PauliString('___'),
        stim.PauliString('___'),
    ]

    sim.set_pauli_flip('X', qubit_index=2, instance_index=0)
    assert sim.peek_pauli_flips() == [
        stim.PauliString('__X'),
        stim.PauliString('___'),
    ]

    sim.set_pauli_flip(3, qubit_index=1, instance_index=1)
    assert sim.peek_pauli_flips() == [
        stim.PauliString('__X'),
        stim.PauliString('_Z_'),
    ]

    sim.set_pauli_flip(2, qubit_index=0, instance_index=1)
    assert sim.peek_pauli_flips() == [
        stim.PauliString('__X'),
        stim.PauliString('YZ_'),
    ]

    sim.set_pauli_flip(1, qubit_index=0, instance_index=-1)
    assert sim.peek_pauli_flips() == [
        stim.PauliString('__X'),
        stim.PauliString('XZ_'),
    ]

    sim.set_pauli_flip(0, qubit_index=2, instance_index=-2)
    assert sim.peek_pauli_flips() == [
        stim.PauliString('___'),
        stim.PauliString('XZ_'),
    ]

    with pytest.raises(ValueError, match='Expected pauli'):
        sim.set_pauli_flip(-1, qubit_index=0, instance_index=0)
    with pytest.raises(ValueError, match='Expected pauli'):
        sim.set_pauli_flip(4, qubit_index=0, instance_index=0)
    with pytest.raises(ValueError, match='Expected pauli'):
        sim.set_pauli_flip('R', qubit_index=0, instance_index=0)
    with pytest.raises(ValueError, match='Expected pauli'):
        sim.set_pauli_flip('XY', qubit_index=0, instance_index=0)
    with pytest.raises(ValueError, match='Expected pauli'):
        sim.set_pauli_flip(object(), qubit_index=0, instance_index=0)

    with pytest.raises(IndexError, match='instance_index'):
        sim.set_pauli_flip('X', qubit_index=0, instance_index=-3)
    with pytest.raises(IndexError, match='instance_index'):
        sim.set_pauli_flip('X', qubit_index=0, instance_index=3)

    with pytest.raises(IndexError, match='qubit_index'):
        sim.set_pauli_flip('X', qubit_index=-1, instance_index=0)

    sim.set_pauli_flip('X', qubit_index=4, instance_index=0)
    assert sim.peek_pauli_flips() == [
        stim.PauliString('____X'),
        stim.PauliString('XZ___'),
    ]
