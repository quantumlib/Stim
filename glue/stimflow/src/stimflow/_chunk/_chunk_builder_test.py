import numpy as np
import stim

import stimflow


def test_builder_init():
    builder = stimflow.ChunkBuilder([0, 1j, 3 + 2j])
    assert builder.q2i == {0: 0, 1j: 1, 3 + 2j: 2}
    assert builder.circuit == stim.Circuit(
        """
        """
    )


def test_append_tick():
    builder = stimflow.ChunkBuilder([0])
    builder.append("TICK")
    builder.append("TICK")
    assert builder.circuit == stim.Circuit(
        """
        TICK
        TICK
        """
    )


def test_append_shift_coords():
    builder = stimflow.ChunkBuilder([0])
    builder.append("SHIFT_COORDS", arg=[0, 0, 1])
    assert builder.circuit == stim.Circuit(
        """
        SHIFT_COORDS(0, 0, 1)
        """
    )


def test_append_measurements():
    builder = stimflow.ChunkBuilder(range(6))

    builder.append("MXX", [(2, 3)])
    assert builder.lookup_mids([(2, 3)]) == [0]
    assert builder.lookup_mids([(3, 2)]) == [0]

    builder.append("MYY", [(5, 4)])
    assert builder.lookup_mids([(4, 5)]) == [1]
    assert builder.lookup_mids([(5, 4)]) == [1]

    builder.append("M", [3])
    assert builder.lookup_mids([3]) == [2]


def test_append_measurements_canonical_order():
    builder = stimflow.ChunkBuilder(range(6))

    builder.append("MX", [5, 2, 3])
    assert builder.lookup_mids([2]) == [0]
    assert builder.lookup_mids([3]) == [1]
    assert builder.lookup_mids([5]) == [2]

    builder.append("MZZ", [(5, 2), (3, 4)])
    assert builder.lookup_mids([(2, 5)]) == [3]
    assert builder.lookup_mids([(3, 4)]) == [4]

    assert builder.circuit == stim.Circuit(
        """
        MX 2 3 5
        MZZ 2 5 3 4
        """
    )


def test_append_mpp():
    builder = stimflow.ChunkBuilder([2 + 3j, 5 + 7j, 11 + 13j])

    xxx = stimflow.PauliMap.from_xs([2 + 3j, 5 + 7j, 11 + 13j])
    z_z = stimflow.PauliMap.from_zs([11 + 13j, 2 + 3j])
    builder.append("MPP", [xxx, z_z])
    assert builder.lookup_mids([xxx]) == [0]
    assert builder.lookup_mids([z_z]) == [1]

    assert builder.circuit == stim.Circuit(
        """
        MPP X0*X1*X2 Z0*Z2
        """
    )


def test_append_observable_include():
    builder = stimflow.ChunkBuilder([2 + 3j, 5 + 7j, 11 + 13j])

    builder.append("R", [5 + 7j])
    builder.append("M", [2 + 3j, 5 + 7j, 11 + 13j], measure_key_func=lambda e: (e, "X"))
    builder.append("OBSERVABLE_INCLUDE", [(5 + 7j, "X")], arg=2)

    assert builder.circuit == stim.Circuit(
        """
        R 1
        M 0 1 2
        OBSERVABLE_INCLUDE(2) rec[-2]
    """
    )


def test_append_detector():
    builder = stimflow.ChunkBuilder([2 + 3j, 5 + 7j, 11 + 13j])

    builder.append("R", [5 + 7j])
    builder.append("M", [2 + 3j, 5 + 7j, 11 + 13j], measure_key_func=lambda e: (e, "X"))
    builder.append("DETECTOR", [(5 + 7j, "X")], arg=[2, 3, 5])

    assert builder.circuit == stim.Circuit(
        """
        R 1
        M 0 1 2
        DETECTOR(2, 3, 5) rec[-2]
        """
    )


def test_make_surface_code_first_round():
    diameter = 3
    tiles = []

    for x in range(-1, diameter):
        for y in range(-1, diameter):
            m = x + 1j * y + 0.5 + 0.5j
            potential_data = [m + 1j**k * (0.5 + 0.5j) for k in range(4)]
            data = [d for d in potential_data if 0 <= d.real < diameter if 0 <= d.imag < diameter]
            if len(data) not in [2, 4]:
                continue

            basis = "XZ"[(x.real + y.real) % 2 == 0]
            if not (0 <= m.real < diameter - 1) and basis != "Z":
                continue
            if not (0 <= m.imag < diameter - 1) and basis != "X":
                continue
            tiles.append(stimflow.Tile(measure_qubit=m, data_qubits=data, bases=basis))

    patch = stimflow.Patch(tiles)
    obs_x = stimflow.PauliMap({q: "X" for q in patch.data_set if q.real == 0}).with_obs_name("LX")
    obs_z = stimflow.PauliMap({q: "Z" for q in patch.data_set if q.imag == 0}).with_obs_name("LZ")
    code = stimflow.StabilizerCode(patch, logicals=[(obs_x, obs_z)]).with_transformed_coords(
        lambda e: e * (1 - 1j)
    )

    builder = stimflow.ChunkBuilder(code.used_set)

    mxs = {tile.measure_qubit for tile in code.patch if tile.basis == "X"}
    mzs = {tile.measure_qubit for tile in code.patch if tile.basis == "Z"}
    builder.append("RX", mxs)
    builder.append("RZ", mzs | code.data_set)
    builder.append("TICK")

    for layer in range(4):
        offset = [1j, 1, -1, -1j][layer]
        cxs = []
        for tile in code.tiles:
            m = tile.measure_qubit
            s = -1 if tile.basis == "Z" else +1
            d = m + offset * (s if 1 <= layer <= 2 else 1)
            if d in code.data_set:
                cxs.append((m, d)[::s])
        builder.append("CX", cxs)
        builder.append("TICK")
    builder.append("MX", mxs)
    builder.append("MZ", mzs)
    for z in stimflow.sorted_complex(mzs):
        builder.append("DETECTOR", [z], arg=[z.real, z.imag, 0])

    assert builder.circuit == stim.Circuit(
        """
        RX 0 7 9 16
        R 1 2 3 4 5 6 8 10 11 12 13 14 15
        TICK
        CX 0 1 4 3 7 8 9 10 12 11 14 13
        TICK
        CX 0 2 1 3 6 11 7 12 8 13 9 14
        TICK
        CX 7 2 8 3 9 4 10 5 15 13 16 14
        TICK
        CX 2 3 4 5 7 6 9 8 12 13 16 15
        TICK
        MX 0 7 9 16
        M 3 5 11 13
        DETECTOR(1, 0, 0) rec[-4]
        DETECTOR(1, 2, 0) rec[-3]
        DETECTOR(3, -2, 0) rec[-2]
        DETECTOR(3, 0, 0) rec[-1]
        """
    )


def test_skip_unknown_1qm():
    builder = stimflow.ChunkBuilder(allowed_qubits=[0, 1, 2, 3])
    builder.append("M", [2, -1, 1, 25, 3], unknown_qubit_append_mode="skip")
    assert builder.circuit == stim.Circuit(
        """
        M 1 2 3
    """
    )
    assert builder.lookup_mids([1]) == [0]
    assert builder.lookup_mids([2]) == [1]
    assert builder.lookup_mids([3]) == [2]


def test_skip_unknown_2qm():
    builder = stimflow.ChunkBuilder(allowed_qubits=[0, 1, 2, 3])
    builder.append("MZZ", [(2, 3), (-1, 5), (0, 1)], unknown_qubit_append_mode="skip")
    assert builder.q2i == {0: 0, 1: 1, 2: 2, 3: 3}
    assert builder.circuit == stim.Circuit(
        """
        MZZ 0 1 2 3
    """
    )
    assert builder.lookup_mids([(0, 1)]) == builder.lookup_mids([(1, 0)]) == [0]
    assert builder.lookup_mids([(2, 3)]) == builder.lookup_mids([(3, 2)]) == [1]


def test_partial_observable_include_memory_experiment():
    obs_x = stimflow.PauliMap.from_xs([0, 1]).with_obs_name("LX")
    obs_z = stimflow.PauliMap.from_zs([0, 1j]).with_obs_name("LZ")
    stab_z0 = stimflow.PauliMap.from_zs([0, 1])
    stab_z1 = stimflow.PauliMap.from_zs([1j, 1 + 1j])
    stab_x = stimflow.PauliMap.from_xs([0, 1, 1j, 1 + 1j])

    builder_init = stimflow.ChunkBuilder()
    builder_init.append("R", [0, 1, 1j, 1 + 1j])
    builder_init.append("OBSERVABLE_INCLUDE", obs_x)
    builder_init.add_flow(end=stab_z0)
    builder_init.add_flow(end=stab_z1)
    builder_init.add_flow(end=obs_z)
    builder_init.add_flow(end=obs_x)
    builder_init.add_discarded_flow_output(stab_x)
    chunk_init = builder_init.finish_chunk()

    builder_bulk = stimflow.ChunkBuilder([0, 1, 1j, 1 + 1j])
    builder_bulk.append("MPP", [stab_z0])
    builder_bulk.append("MPP", [stab_z1])
    builder_bulk.append("MPP", [stab_x])

    builder_bulk.add_flow(start=stab_z0, ms=[stab_z0])
    builder_bulk.add_flow(start=stab_z1, ms=[stab_z1])
    builder_bulk.add_flow(start=stab_x, ms=[stab_x])
    builder_bulk.add_flow(end=stab_z0, ms=[stab_z0])
    builder_bulk.add_flow(end=stab_z1, ms=[stab_z1])
    builder_bulk.add_flow(end=stab_x, ms=[stab_x])
    builder_bulk.add_flow(start=obs_x, end=obs_x)
    builder_bulk.add_flow(start=obs_z, end=obs_z)
    chunk_bulk = builder_bulk.finish_chunk()

    builder_end = stimflow.ChunkBuilder()
    builder_end.append("OBSERVABLE_INCLUDE", obs_z)
    builder_end.append("MX", [0, 1, 1j, 1 + 1j])
    builder_end.add_discarded_flow_input(stab_z0)
    builder_end.add_discarded_flow_input(stab_z1)
    builder_end.add_flow(start=obs_z)
    builder_end.add_flow(start=stab_x, ms=stab_x.keys())
    builder_end.add_flow(start=obs_x, ms=obs_x.keys())
    chunk_end = builder_end.finish_chunk()

    chunk_init.verify()
    chunk_bulk.verify()
    chunk_end.verify()

    compiler = stimflow.ChunkCompiler()
    compiler.append(chunk_init)
    compiler.append(chunk_bulk * 3)
    compiler.append(chunk_end)

    circuit = compiler.finish_circuit()
    assert circuit == stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        QUBIT_COORDS(1, 0) 2
        QUBIT_COORDS(1, 1) 3
        R 0 2 1 3
        OBSERVABLE_INCLUDE(0) X0 X2
        TICK
        MPP Z0*Z2 Z1*Z3 X0*X1*X2*X3
        DETECTOR(0.5, 0, 0) rec[-3]
        DETECTOR(0.5, 1, 0) rec[-2]
        SHIFT_COORDS(0, 0, 1)
        TICK
        REPEAT 2 {
            MPP Z0*Z2 Z1*Z3 X0*X1*X2*X3
            DETECTOR(0.5, 0, 0) rec[-6] rec[-3]
            DETECTOR(0.5, 1, 0) rec[-5] rec[-2]
            DETECTOR(0.5, 0.5, 0) rec[-4] rec[-1]
            SHIFT_COORDS(0, 0, 1)
            TICK
        }
        OBSERVABLE_INCLUDE(1) Z0 Z1
        MX 0 1 2 3
        DETECTOR(0.5, 0.5, 0) rec[-5] rec[-4] rec[-3] rec[-2] rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-4] rec[-2]
    """
    )

    circuit.detector_error_model()  # Check detector error model exists.

    # Check flip sampling behaves as expected.
    dets, obs = circuit.compile_detector_sampler().sample(128, separate_observables=True)
    assert np.count_nonzero(dets) == 0
    assert np.count_nonzero(obs) == 0

    # Check measurement-to-obs conversion behaves as expected.
    ms = circuit.compile_sampler().sample(512)
    dets, obs = circuit.compile_m2d_converter().convert(measurements=ms, separate_observables=True)
    assert np.count_nonzero(dets) == 0
    assert 200 <= np.count_nonzero(obs[:, 0]) <= 300  # Some measurements included in partial X obs
    assert np.count_nonzero(obs[:, 1]) == 0  # No measurements included in partial Z obs
