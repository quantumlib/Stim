import collections

import pytest
import stim
import numpy as np


def test_get_measurement_flips():
    s = stim.FlipSimulator(batch_size=11)
    assert s.num_measurements == 0
    assert s.num_qubits == 0
    assert s.batch_size == 11

    s.do(
        stim.Circuit(
            """
        X_ERROR(1) 100
        M 100
    """
        )
    )
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

    s.do(
        stim.Circuit(
            """
        X_ERROR(1) 25
        M 24 25
        DETECTOR rec[-1]
        DETECTOR rec[-1]
        DETECTOR rec[-1]
    """
        )
    )
    assert s.num_measurements == 2
    assert s.num_detectors == 3
    assert s.num_qubits == 26
    assert s.batch_size == 11
    np.testing.assert_array_equal(
        s.get_detector_flips(), np.ones(shape=(3, 11), dtype=np.bool_)
    )
    np.testing.assert_array_equal(
        s.get_detector_flips(detector_index=1), np.ones(shape=(11,), dtype=np.bool_)
    )
    np.testing.assert_array_equal(
        s.get_detector_flips(instance_index=1), np.ones(shape=(3,), dtype=np.bool_)
    )
    assert s.get_detector_flips(detector_index=1, instance_index=1)


def test_get_observable_flips():
    s = stim.FlipSimulator(batch_size=11)
    assert s.num_measurements == 0
    assert s.num_qubits == 0
    assert s.batch_size == 11
    assert s.num_observables == 0

    s.do(
        stim.Circuit(
            """
        X_ERROR(1) 25
        M 24 25
        OBSERVABLE_INCLUDE(2) rec[-1]
    """
        )
    )
    assert s.num_measurements == 2
    assert s.num_detectors == 0
    assert s.num_observables == 3
    assert s.num_qubits == 26
    assert s.batch_size == 11
    np.testing.assert_array_equal(
        s.get_observable_flips(observable_index=1),
        np.zeros(shape=(11,), dtype=np.bool_),
    )
    np.testing.assert_array_equal(
        s.get_observable_flips(observable_index=2), np.ones(shape=(11,), dtype=np.bool_)
    )
    np.testing.assert_array_equal(
        s.get_observable_flips(instance_index=1), [False, False, True]
    )
    assert not s.get_observable_flips(observable_index=1, instance_index=1)
    assert s.get_observable_flips(observable_index=2, instance_index=1)


def test_peek_pauli_flips():
    sim = stim.FlipSimulator(batch_size=500, disable_stabilizer_randomization=True)
    sim.do(
        stim.Circuit(
            """
        X_ERROR(0.3) 1
        Y_ERROR(0.3) 2
        Z_ERROR(0.3) 3
        DEPOLARIZE1(0.3) 4
    """
        )
    )
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


def test_set_pauli_flip():
    sim = stim.FlipSimulator(
        batch_size=2,
        disable_stabilizer_randomization=True,
        num_qubits=3,
    )
    assert sim.peek_pauli_flips() == [
        stim.PauliString("___"),
        stim.PauliString("___"),
    ]

    sim.set_pauli_flip("X", qubit_index=2, instance_index=0)
    assert sim.peek_pauli_flips() == [
        stim.PauliString("__X"),
        stim.PauliString("___"),
    ]

    sim.set_pauli_flip(3, qubit_index=1, instance_index=1)
    assert sim.peek_pauli_flips() == [
        stim.PauliString("__X"),
        stim.PauliString("_Z_"),
    ]

    sim.set_pauli_flip(2, qubit_index=0, instance_index=1)
    assert sim.peek_pauli_flips() == [
        stim.PauliString("__X"),
        stim.PauliString("YZ_"),
    ]

    sim.set_pauli_flip(1, qubit_index=0, instance_index=-1)
    assert sim.peek_pauli_flips() == [
        stim.PauliString("__X"),
        stim.PauliString("XZ_"),
    ]

    sim.set_pauli_flip(0, qubit_index=2, instance_index=-2)
    assert sim.peek_pauli_flips() == [
        stim.PauliString("___"),
        stim.PauliString("XZ_"),
    ]

    with pytest.raises(ValueError, match="pauli"):
        sim.set_pauli_flip(-1, qubit_index=0, instance_index=0)
    with pytest.raises(ValueError, match="pauli"):
        sim.set_pauli_flip(4, qubit_index=0, instance_index=0)
    with pytest.raises(ValueError, match="pauli"):
        sim.set_pauli_flip("R", qubit_index=0, instance_index=0)
    with pytest.raises(ValueError, match="pauli"):
        sim.set_pauli_flip("XY", qubit_index=0, instance_index=0)
    with pytest.raises(ValueError, match="pauli"):
        sim.set_pauli_flip(object(), qubit_index=0, instance_index=0)

    with pytest.raises(IndexError, match="instance_index"):
        sim.set_pauli_flip("X", qubit_index=0, instance_index=-3)
    with pytest.raises(IndexError, match="instance_index"):
        sim.set_pauli_flip("X", qubit_index=0, instance_index=3)

    with pytest.raises(IndexError, match="qubit_index"):
        sim.set_pauli_flip("X", qubit_index=-1, instance_index=0)

    sim.set_pauli_flip("X", qubit_index=4, instance_index=0)
    assert sim.peek_pauli_flips() == [
        stim.PauliString("____X"),
        stim.PauliString("XZ___"),
    ]


def test_broadcast_pauli_errors():
    sim = stim.FlipSimulator(
        batch_size=2,
        num_qubits=3,
        disable_stabilizer_randomization=True,
    )
    sim.broadcast_pauli_errors(
        pauli="X",
        mask=np.asarray([[True, False], [False, False], [True, True]]),
    )
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+X_X"), stim.PauliString("+__X")]
    sim.broadcast_pauli_errors(
        pauli="Z",
        mask=np.asarray([[True, True], [True, False], [False, False]]),
    )
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+YZX"), stim.PauliString("+Z_X")]
    sim.broadcast_pauli_errors(
        pauli="Y",
        mask=np.asarray([[True, False], [False, True], [False, True]]),
    )
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+_ZX"), stim.PauliString("+ZYZ")]
    sim.broadcast_pauli_errors(
        pauli="I",
        mask=np.asarray([[True, True], [False, True], [True, True]]),
    )
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+_ZX"), stim.PauliString("+ZYZ")]

    # do it again with ints
    sim = stim.FlipSimulator(
        batch_size=2,
        num_qubits=3,
        disable_stabilizer_randomization=True,
    )
    sim.broadcast_pauli_errors(
        pauli=1,
        mask=np.asarray([[True, False], [False, False], [True, True]]),
    )
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+X_X"), stim.PauliString("+__X")]
    sim.broadcast_pauli_errors(
        pauli=3,
        mask=np.asarray([[True, True], [True, False], [False, False]]),
    )
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+YZX"), stim.PauliString("+Z_X")]
    sim.broadcast_pauli_errors(
        pauli=2,
        mask=np.asarray([[True, False], [False, True], [False, True]]),
    )
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+_ZX"), stim.PauliString("+ZYZ")]
    sim.broadcast_pauli_errors(
        pauli=0,
        mask=np.asarray([[True, True], [False, True], [True, True]]),
    )
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+_ZX"), stim.PauliString("+ZYZ")]

    with pytest.raises(ValueError, match="pauli"):
        sim.broadcast_pauli_errors(
            pauli="whoops",
            mask=np.asarray([[True, True], [False, True], [True, True]]),
        )
    with pytest.raises(ValueError, match="pauli"):
        sim.broadcast_pauli_errors(
            pauli=4,
            mask=np.asarray([[True, True], [False, True], [True, True]]),
        )
    with pytest.raises(ValueError, match="batch_size"):
        sim.broadcast_pauli_errors(
            pauli="X",
            mask=np.asarray(
                [[True, True, True], [False, True, True], [True, True, True]]
            ),
        )
    with pytest.raises(ValueError, match="batch_size"):
        sim.broadcast_pauli_errors(
            pauli="X",
            mask=np.asarray([[True], [False], [True]]),
        )
    sim = stim.FlipSimulator(
        batch_size=2,
        num_qubits=3,
        disable_stabilizer_randomization=True,
    )
    sim.broadcast_pauli_errors(
        pauli="X",
        mask=np.asarray([[True, False], [False, False], [True, True], [True, True]]),
    )  # automatically expands the qubit basis
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+X_XX"), stim.PauliString("+__XX")]
    sim.broadcast_pauli_errors(
        pauli="X",
        mask=np.asarray(
            [
                [True, False],
                [False, False],
            ]
        ),
    )  # tolerates fewer qubits in mask than in simulator
    peek = sim.peek_pauli_flips()
    assert peek == [stim.PauliString("+__XX"), stim.PauliString("+__XX")]


def test_repro_heralded_pauli_channel_1_bug():
    circuit = stim.Circuit(
        """
        R 0 1
        HERALDED_PAULI_CHANNEL_1(0.2, 0.2, 0, 0) 1
        M 0
    """
    )
    result = circuit.compile_sampler().sample(1024)
    assert np.sum(result[:, 0]) > 0
    assert np.sum(result[:, 1]) == 0


def test_to_numpy():
    sim = stim.FlipSimulator(batch_size=50)
    sim.do(
        stim.Circuit.generated(
            "surface_code:rotated_memory_x",
            distance=5,
            rounds=3,
            after_clifford_depolarization=0.1,
        )
    )

    xs0, zs0, ms0, ds0, os0 = sim.to_numpy(
        output_xs=True,
        output_zs=True,
        output_measure_flips=True,
        output_detector_flips=True,
        output_observable_flips=True,
    )
    for k in range(50):
        np.testing.assert_array_equal(
            xs0[:, k], sim.peek_pauli_flips()[k].to_numpy()[0]
        )
        np.testing.assert_array_equal(
            zs0[:, k], sim.peek_pauli_flips()[k].to_numpy()[1]
        )
        np.testing.assert_array_equal(
            ms0[:, k], sim.get_measurement_flips(instance_index=k)
        )
        np.testing.assert_array_equal(
            ds0[:, k], sim.get_detector_flips(instance_index=k)
        )
        np.testing.assert_array_equal(
            os0[:, k], sim.get_observable_flips(instance_index=k)
        )

    xs, zs, ms, ds, os = sim.to_numpy(output_xs=True)
    np.testing.assert_array_equal(xs, xs0)
    assert zs is None
    assert ms is None
    assert ds is None
    assert os is None

    xs, zs, ms, ds, os = sim.to_numpy(output_zs=True)
    assert xs is None
    np.testing.assert_array_equal(zs, zs0)
    assert ms is None
    assert ds is None
    assert os is None

    xs, zs, ms, ds, os = sim.to_numpy(output_measure_flips=True)
    assert xs is None
    assert zs is None
    np.testing.assert_array_equal(ms, ms0)
    assert ds is None
    assert os is None

    xs, zs, ms, ds, os = sim.to_numpy(output_detector_flips=True)
    assert xs is None
    assert zs is None
    assert ms is None
    np.testing.assert_array_equal(ds, ds0)
    assert os is None

    xs, zs, ms, ds, os = sim.to_numpy(output_observable_flips=True)
    assert xs is None
    assert zs is None
    assert ms is None
    assert ds is None
    np.testing.assert_array_equal(os, os0)

    xs1 = np.empty_like(xs0)
    zs1 = np.empty_like(zs0)
    ms1 = np.empty_like(ms0)
    ds1 = np.empty_like(ds0)
    os1 = np.empty_like(os0)
    xs2, zs2, ms2, ds2, os2 = sim.to_numpy(
        output_xs=xs1,
        output_zs=zs1,
        output_measure_flips=ms1,
        output_detector_flips=ds1,
        output_observable_flips=os1,
    )
    assert xs1 is xs2
    assert zs1 is zs2
    assert ms1 is ms2
    assert ds1 is ds2
    assert os1 is os2
    np.testing.assert_array_equal(xs1, xs0)
    np.testing.assert_array_equal(zs1, zs0)
    np.testing.assert_array_equal(ms1, ms0)
    np.testing.assert_array_equal(ds1, ds0)
    np.testing.assert_array_equal(os1, os0)

    xs2, zs2, ms2, ds2, os2 = sim.to_numpy(
        transpose=True,
        output_xs=True,
        output_zs=True,
        output_measure_flips=True,
        output_detector_flips=True,
        output_observable_flips=True,
    )
    np.testing.assert_array_equal(xs2, np.transpose(xs0))
    np.testing.assert_array_equal(zs2, np.transpose(zs0))
    np.testing.assert_array_equal(ms2, np.transpose(ms0))
    np.testing.assert_array_equal(ds2, np.transpose(ds0))
    np.testing.assert_array_equal(os2, np.transpose(os0))

    xs2, zs2, ms2, ds2, os2 = sim.to_numpy(
        bit_packed=True,
        output_xs=True,
        output_zs=True,
        output_measure_flips=True,
        output_detector_flips=True,
        output_observable_flips=True,
    )
    np.testing.assert_array_equal(xs2, np.packbits(xs0, axis=1, bitorder="little"))
    np.testing.assert_array_equal(zs2, np.packbits(zs0, axis=1, bitorder="little"))
    np.testing.assert_array_equal(ms2, np.packbits(ms0, axis=1, bitorder="little"))
    np.testing.assert_array_equal(ds2, np.packbits(ds0, axis=1, bitorder="little"))
    np.testing.assert_array_equal(os2, np.packbits(os0, axis=1, bitorder="little"))

    xs2, zs2, ms2, ds2, os2 = sim.to_numpy(
        transpose=True,
        bit_packed=True,
        output_xs=True,
        output_zs=True,
        output_measure_flips=True,
        output_detector_flips=True,
        output_observable_flips=True,
    )
    np.testing.assert_array_equal(
        xs2, np.packbits(np.transpose(xs0), axis=1, bitorder="little")
    )
    np.testing.assert_array_equal(
        zs2, np.packbits(np.transpose(zs0), axis=1, bitorder="little")
    )
    np.testing.assert_array_equal(
        ms2, np.packbits(np.transpose(ms0), axis=1, bitorder="little")
    )
    np.testing.assert_array_equal(
        ds2, np.packbits(np.transpose(ds0), axis=1, bitorder="little")
    )
    np.testing.assert_array_equal(
        os2, np.packbits(np.transpose(os0), axis=1, bitorder="little")
    )

    with pytest.raises(ValueError, match="at least one output"):
        sim.to_numpy()
    with pytest.raises(ValueError, match="shape="):
        sim.to_numpy(output_xs=np.empty(shape=(0, 0), dtype=np.uint64))
    with pytest.raises(ValueError, match="shape="):
        sim.to_numpy(output_zs=np.empty(shape=(0, 0), dtype=np.uint64))
    with pytest.raises(ValueError, match="shape="):
        sim.to_numpy(output_measure_flips=np.empty(shape=(0, 0), dtype=np.uint64))
    with pytest.raises(ValueError, match="shape="):
        sim.to_numpy(output_detector_flips=np.empty(shape=(0, 0), dtype=np.uint64))
    with pytest.raises(ValueError, match="shape="):
        sim.to_numpy(output_observable_flips=np.empty(shape=(0, 0), dtype=np.uint64))


def test_generate_bernoulli_samples():
    sim = stim.FlipSimulator(batch_size=10)

    v = sim.generate_bernoulli_samples(1001, p=0, bit_packed=False)
    assert v.shape == (1001,)
    assert v.dtype == np.bool_
    assert np.sum(v) == 0

    v2 = sim.generate_bernoulli_samples(1001, p=1, bit_packed=False, out=v)
    assert v is v2
    assert v.shape == (1001,)
    assert v.dtype == np.bool_
    assert np.sum(v) == 1001

    v = sim.generate_bernoulli_samples(2**16, p=0.25, bit_packed=False)
    assert abs(np.sum(v) - 2**16 * 0.25) < 2**12

    v = sim.generate_bernoulli_samples(1001, p=0, bit_packed=True)
    assert v.shape == (126,)
    assert v.dtype == np.uint8
    assert np.sum(np.unpackbits(v, count=1001, bitorder="little")) == 0
    assert np.sum(np.unpackbits(v, count=1008, bitorder="little")) == 0

    v2 = sim.generate_bernoulli_samples(1001, p=1, bit_packed=True, out=v)
    assert v is v2
    assert v.shape == (126,)
    assert v.dtype == np.uint8
    assert np.sum(np.unpackbits(v, count=1001, bitorder="little")) == 1001
    assert np.sum(np.unpackbits(v, count=1008, bitorder="little")) == 1001

    v = sim.generate_bernoulli_samples(256, p=0, bit_packed=True)
    assert np.all(v == 0)

    sim.generate_bernoulli_samples(256 - 101, p=1, bit_packed=True, out=v[1:-11])
    for k in v:
        print(k)
    assert np.all(v[1:-12] == 0xFF)
    assert v[-12] == 7
    assert np.all(v[-11:] == 0)
    assert np.all(v[:1] == 0)

    v = sim.generate_bernoulli_samples(2**16, p=0.25, bit_packed=True)
    assert abs(np.sum(np.unpackbits(v, count=2**16)) - 2**16 * 0.25) < 2**12

    v[:] = 0
    sim.generate_bernoulli_samples(2**16 - 1, p=1, bit_packed=True, out=v)
    assert np.all(v[:-1] == 0xFF)
    assert v[-1] == 0x7F

    v[:] = 0
    sim.generate_bernoulli_samples(2**15, p=1, bit_packed=True, out=v[::2])
    assert np.all(v[0::2] == 0xFF)
    assert np.all(v[1::2] == 0)

    v[:] = 0
    sim.generate_bernoulli_samples(2**15 - 1, p=1, bit_packed=True, out=v[::2])
    assert np.all(v[0::2][:-1] == 0xFF)
    assert v[0::2][-1] == 0x7F
    assert np.all(v[1::2] == 0)


def test_get_measurement_flips_negative_index():
    sim = stim.FlipSimulator(batch_size=8, disable_stabilizer_randomization=True)
    sim.do(
        stim.Circuit(
            """
        X_ERROR(1) 1
        M 0 1
    """
        )
    )
    np.testing.assert_array_equal(
        sim.get_measurement_flips(record_index=-2), [False] * 8
    )
    np.testing.assert_array_equal(
        sim.get_measurement_flips(record_index=-1), [True] * 8
    )
    np.testing.assert_array_equal(
        sim.get_measurement_flips(record_index=0), [False] * 8
    )
    np.testing.assert_array_equal(sim.get_measurement_flips(record_index=1), [True] * 8)
