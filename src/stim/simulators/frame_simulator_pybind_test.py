import stim
import numpy as np


def test_get_measurement_flips():
    s = stim.FlipSimulator(batch_size=11)
    assert s.num_measurements == 0
    assert s.num_qubits == 0
    assert s.batch_size == 11

    s.do_circuit(stim.Circuit("""
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
    np.testing.assert_array_equal(
        s.peek_current_pauli_errors(),
        np.zeros(shape=(10, 256), dtype=np.uint8),
    )
    s.do_circuit(stim.Circuit("R 19"))
    np.testing.assert_array_equal(
        s.peek_current_pauli_errors(),
        np.zeros(shape=(20, 256), dtype=np.uint8),
    )

    s = stim.FlipSimulator(
        batch_size=256,
        num_qubits=10,
        disable_stabilizer_randomization=False,
    )
    v = s.peek_current_pauli_errors()
    assert v.shape == (10, 256)
    assert np.all((v == 0) | (v == 3))
    assert 0.2 < np.count_nonzero(v == 3) / (256 * 10) < 0.8
    s.do_circuit(stim.Circuit("R 19"))
    v = s.peek_current_pauli_errors()
    assert v.shape == (20, 256)
    assert np.all((v == 0) | (v == 3))
    assert 0.2 < np.count_nonzero(v == 3) / (256 * 20) < 0.8


def test_get_detector_flips():
    s = stim.FlipSimulator(batch_size=11)
    assert s.num_measurements == 0
    assert s.num_qubits == 0
    assert s.batch_size == 11

    s.do_circuit(stim.Circuit("""
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

    s.do_circuit(stim.Circuit("""
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
