import stim

import stimflow


def test_to_cz_circuit_rotation_folding():
    assert stimflow.transpile_to_z_basis_interaction_circuit(stim.Circuit()) == stim.Circuit()

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    C_XYZ 0
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_XYZ 0
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    I 0
                    TICK
                    C_XYZ 0
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_XYZ 0
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    C_XYZ 0
                    TICK
                    I 0
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_XYZ 0
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    H 0
                    TICK
                    H 0
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    H 0 1 2 3 4 5
                    TICK
                    I 0
                    H_YZ 1
                    H_XY 2
                    C_XYZ 3
                    C_ZYX 4
                    H 5
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_XYNZ 1
                C_ZYNX 2
                H 0
                S 4
                SQRT_X_DAG 3
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    H_XY 0 1 2 3 4 5
                    TICK
                    I 0
                    H_YZ 1
                    H_XY 2
                    C_XYZ 3
                    C_ZYX 4
                    H 5
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_NXYZ 5
                C_ZNYX 1
                H_XY 0
                SQRT_X 4
                SQRT_Y_DAG 3
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    H_YZ 0 1 2 3 4 5
                    TICK
                    I 0
                    H_YZ 1
                    H_XY 2
                    C_XYZ 3
                    C_ZYX 4
                    H 5
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_NZYX 5
                C_XNYZ 2
                H_YZ 0
                SQRT_Y 4
                S_DAG 3
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    C_XYZ 0 1 2 3 4 5
                    TICK
                    I 0
                    H_YZ 1
                    H_XY 2
                    C_XYZ 3
                    C_ZYX 4
                    H 5
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_XYZ 0
                C_ZYX 3
                SQRT_X_DAG 2
                SQRT_Y_DAG 1
                S_DAG 5
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    C_ZYX 0 1 2 3 4 5
                    TICK
                    I 0
                    H_YZ 1
                    H_XY 2
                    C_XYZ 3
                    C_ZYX 4
                    H 5
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_XYZ 4
                C_ZYX 0
                S 1
                SQRT_X 5
                SQRT_Y 2
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    I 0 1 2 3 4 5
                    TICK
                    I 0
                    H_YZ 1
                    H_XY 2
                    C_XYZ 3
                    C_ZYX 4
                    H 5
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                C_XYZ 3
                C_ZYX 4
                H 5
                H_XY 2
                H_YZ 1
                TICK
                M 0 1 2 3
            """
        )
    )


def test_to_cz_circuit_loop_boundary_folding():
    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    H 2
                    TICK
                    CX rec[-1] 2
                    TICK
                    S 2
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                CZ rec[-1] 2
                C_ZYX 2
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    MRX 0
                    DETECTOR rec[-1]
                    TICK
                    M 0
                    TICK
                    H 0
                """
            )
        )
        == stim.Circuit(
            """
                H 0
                TICK
                M 0
                TICK
                R 0
                DETECTOR rec[-1]
                TICK
                H 0
                TICK
                M 0
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    REPEAT 100 {
                        C_XYZ 0
                        TICK
                        CZ 0 1
                        TICK
                        H 0
                        TICK
                    }
                    M 0
                """
            )
        )
        == stim.Circuit(
            """
                H 0
                TICK
                REPEAT 100 {
                    SQRT_X_DAG 0
                    TICK
                    CZ 0 1
                    TICK
                }
                H 0
                TICK
                M 0
            """
        )
    )


def test_to_cz_circuit_from_cnot():
    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    CX 0 1 2 3
                    TICK
                    CX 1 0 2 3
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                H 1 3
                TICK
                CZ 0 1 2 3
                TICK
                H 0 1
                TICK
                CZ 0 1 2 3
                TICK
                H 0 3
                TICK
                M 0 1 2 3
            """
        )
    )


def test_to_cz_circuit_from_swap_cnot():
    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    CNOT 0 1
                    TICK
                    SWAP 0 1
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                H 1
                TICK
                CZSWAP 0 1
                TICK
                H 0
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    CNOT 0 1
                    TICK
                    SWAP 1 0
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                H 1
                TICK
                CZSWAP 0 1
                TICK
                H 0
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    SWAP 1 0
                    TICK
                    CNOT 1 0
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                H 1
                TICK
                CZSWAP 0 1
                TICK
                H 0
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    SWAP 0 1
                    TICK
                    CNOT 1 0
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                H 1
                TICK
                CZSWAP 0 1
                TICK
                H 0
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    CNOT 1 0
                    TICK
                    SWAP 0 1
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                H 0
                TICK
                CZSWAP 0 1
                TICK
                H 1
                TICK
                M 0 1 2 3
            """
        )
    )

    assert (
        stimflow.transpile_to_z_basis_interaction_circuit(
            stim.Circuit(
                """
                    SWAP 0 1
                    TICK
                    CNOT 0 1
                    TICK
                    M 0 1 2 3
                """
            )
        )
        == stim.Circuit(
            """
                H 0
                TICK
                CZSWAP 0 1
                TICK
                H 1
                TICK
                M 0 1 2 3
            """
        )
    )


def test_tagged_layers_not_affected():
    builder = stimflow.ChunkBuilder(allowed_qubits=[0 + 0j])
    builder.append("tick")
    builder.append("H", [0 + 0j])
    builder.append("tick")
    builder.append("H", [0 + 0j], tag="tag")
    actual = stimflow.transpile_to_z_basis_interaction_circuit(builder.finish_chunk().circuit)
    assert builder.q2i == {0: 0}
    expected = stim.Circuit(
        """
        H 0
        TICK
        H[tag] 0
        """
    )
    assert actual == expected


def test_tagged_layers_not_affected_2q():

    builder = stimflow.ChunkBuilder(allowed_qubits=[0 + 0j, 1 + 0j])
    builder.append("R", [0 + 0j, 1 + 0j])
    builder.append("tick")
    builder.append("H", [0 + 0j])
    builder.append("tick")
    builder.append("CNOT", [(0 + 0j, 1 + 0j)], tag="test")
    builder.append("tick")
    builder.append("M", [0 + 0j, 1 + 0j])
    actual = stimflow.transpile_to_z_basis_interaction_circuit(builder.finish_chunk().circuit)
    assert builder.q2i == {0: 0, 1: 1}
    expected = stim.Circuit(
        """
        R 0 1
        TICK
        H 0
        TICK
        CX[test] 0 1
        TICK
        M 0 1
    """
    )
    assert actual == expected


def test_tagged_layers_simultaneous_ops():

    builder = stimflow.ChunkBuilder(allowed_qubits=[0 + 0j, 1 + 0j, 2])
    builder.append("R", [0 + 0j, 1 + 0j])
    builder.append("tick")
    builder.append("H", [0 + 0j])
    builder.append("tick")
    builder.append("CNOT", [(0 + 0j, 1 + 0j)], tag="test")
    builder.append("X", [2])
    builder.append("tick")
    builder.append("M", [0 + 0j, 1 + 0j])
    actual = stimflow.transpile_to_z_basis_interaction_circuit(builder.finish_chunk().circuit)
    assert builder.q2i == {0: 0, 1: 1, 2: 2}
    expected = stim.Circuit(
        """
        R 0 1
        TICK
        H 0
        X 2
        TICK
        CX[test] 0 1
        TICK
        M 0 1
    """
    )
    assert actual == expected


def test_tagged_layers_with_measurements():
    builder = stimflow.ChunkBuilder(allowed_qubits=[0 + 0j, 1 + 0j])
    builder.append("R", [0 + 0j, 1 + 0j])
    builder.append("tick")
    builder.append("H", [0 + 0j])
    builder.append("tick")
    builder.append("CNOT", [(0 + 0j, 1 + 0j)], tag="test")
    builder.append("tick")
    builder.append("M", [0 + 0j, 1 + 0j], tag="test again")
    actual = stimflow.transpile_to_z_basis_interaction_circuit(builder.finish_chunk().circuit)
    assert builder.q2i == {0: 0, 1: 1}
    expected = stim.Circuit(
        """
        R 0 1
        TICK
        H 0
        TICK
        CX[test] 0 1
        TICK
        M[test again] 0 1
        """
    )
    assert actual == expected
