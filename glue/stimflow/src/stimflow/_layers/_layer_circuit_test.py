import stim

from stimflow._layers._layer_interact import LayerInteract
from stimflow._layers._layer_circuit import LayerCircuit
from stimflow._layers._layer_reset import LayerReset


def test_with_squashed_rotations():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                    S 0 1 2 3
                    TICK
                    CZ 1 2
                    TICK
                    H 0 3
                    TICK
                    CZ 1 2
                    TICK
                    S 0 1 2 3
                """
            )
        )
        .with_clearable_rotation_layers_cleared()
        .to_stim_circuit()
        == stim.Circuit(
            """
                C_XNYZ 0 3
                S 1 2
                TICK
                CZ 1 2
                TICK
                CZ 1 2
                TICK
                S 0 1 2 3
            """
        )
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        S 0 1 2 3
        TICK
        CZ 0 2
        TICK
        H 0 3
        TICK
        CZ 1 2
        TICK
        S 0 1 2 3
    """
            )
        )
        .with_clearable_rotation_layers_cleared()
        .to_stim_circuit()
        == stim.Circuit(
            """
                C_XNYZ 3
                S 0 1 2
                TICK
                CZ 0 2
                TICK
                CZ 1 2
                TICK
                C_ZYX 0
                S 1 2 3
            """
        )
    )


def test_with_rotations_before_resets_removed():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                    H 0 1 2 3
                    TICK
                    R 0 1
                """
            )
        )
        .with_rotations_before_resets_removed()
        .to_stim_circuit()
        == stim.Circuit(
            """
                H 2 3
                TICK
                R 0 1
            """
        )
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                    H 0 1 2 3
                    TICK
                    REPEAT 100 {
                        R 0 1
                        TICK
                        H 0 1 2 3
                        TICK
                    }
                    R 1 2
                    TICK
                """
            )
        )
        .with_rotations_before_resets_removed()
        .to_stim_circuit()
        == stim.Circuit(
            """
                H 2 3
                TICK
                REPEAT 100 {
                    R 0 1
                    TICK
                    H 0 2 3
                    TICK
                }
                R 1 2
            """
        )
    )


def test_with_rotations_merged_earlier():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                    S 0 1 2 3
                    TICK
                    CZ 1 2 3 4
                    TICK
                    H 0 1 2 3
                    TICK
                    CZ 1 2
                    TICK
                    S 0 1 2 3
                """
            )
        )
        .with_rotations_merged_earlier()
        .to_stim_circuit()
        == stim.Circuit(
            """
                S 1 2 3
                SQRT_X_DAG 0
                TICK
                CZ 1 2 3 4
                TICK
                C_ZYX 3
                H 1 2
                TICK
                CZ 1 2
                TICK
                S 1 2
            """
        )
    )


def test_with_qubit_coords_at_start():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        QUBIT_COORDS(2, 3) 0
        SHIFT_COORDS(0, 0, 100)
        QUBIT_COORDS(5, 7) 1
        SHIFT_COORDS(0, 200)
        QUBIT_COORDS(11, 13) 2
        SHIFT_COORDS(300)
        R 0 1
        TICK
        QUBIT_COORDS(17) 3
        H 3
        TICK
        QUBIT_COORDS(19, 23, 29) 4
        REPEAT 10 {
            M 0 1 2 3
            DETECTOR rec[-1]
        }
    """
            )
        )
        .with_qubit_coords_at_start()
        .to_stim_circuit()
        == stim.Circuit(
            """
        QUBIT_COORDS(2, 3) 0
        QUBIT_COORDS(5, 7) 1
        QUBIT_COORDS(11, 213) 2
        QUBIT_COORDS(317) 3
        QUBIT_COORDS(319, 223, 129) 4
        SHIFT_COORDS(0, 0, 100)
        SHIFT_COORDS(0, 200)
        SHIFT_COORDS(300)
        R 0 1
        TICK
        H 3
        TICK
        REPEAT 10 {
            M 0 1 2 3
            DETECTOR rec[-1]
            TICK
        }
    """
        )
    )


def test_merge_shift_coords():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        SHIFT_COORDS(300)
        SHIFT_COORDS(0, 0, 100)
        SHIFT_COORDS(0, 200)
    """
            )
        )
        .with_locally_optimized_layers()
        .to_stim_circuit()
        == stim.Circuit(
            """
        SHIFT_COORDS(300, 200, 100)
    """
        )
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        SHIFT_COORDS(300)
        TICK
        SHIFT_COORDS(0, 0, 100)
        TICK
        SHIFT_COORDS(0, 200)
        TICK
    """
            )
        )
        .with_locally_optimized_layers()
        .to_stim_circuit()
        == stim.Circuit(
            """
        SHIFT_COORDS(300, 200, 100)
    """
        )
    )


def test_merge_resets_and_measurements():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        RX 0 1
        TICK
        RY 2 3
    """
            )
        )
        .with_locally_optimized_layers()
        .to_stim_circuit()
        == stim.Circuit(
            """
        RX 0 1
        RY 2 3
    """
        )
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        RX 0 1
        TICK
        RY 1 2 3
    """
            )
        )
        .with_locally_optimized_layers()
        .to_stim_circuit()
        == stim.Circuit(
            """
        RX 0
        RY 1 2 3
    """
        )
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        MX 0 1
        TICK
        MY 2 3
    """
            )
        )
        .with_locally_optimized_layers()
        .to_stim_circuit()
        == stim.Circuit(
            """
        MX 0 1
        MY 2 3
    """
        )
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        MX 0 1
        TICK
        MY 1 2 3
    """
            )
        )
        .with_locally_optimized_layers()
        .to_stim_circuit()
        == stim.Circuit(
            """
        MX 0 1
        TICK
        MY 1 2 3
    """
        )
    )


def test_swap_cancellation():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        SWAP 0 1 2 3 4 5
        TICK
        SWAP 2 3 4 6 7 8
    """
            )
        ).with_locally_optimized_layers()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        SWAP 0 1 4 5 7 8
        TICK
        SWAP 4 6
    """
            )
        )
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        SWAP 0 1 2 3 4 5
        TICK
        SWAP 2 3
    """
            )
        ).with_locally_optimized_layers()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        SWAP 0 1 4 5
    """
            )
        )
    )


def test_with_rotation_layers_moved_earlier():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        CX 0 1
        TICK
        SWAP 2 3
        TICK
        H 1 4 5 6
    """
            )
        ).with_whole_rotation_layers_slid_earlier()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        CX 0 1
        TICK
        H 1 4 5 6
        TICK
        SWAP 2 3
    """
            )
        )
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        CX 0 1
        TICK
        S 7
        TICK
        SWAP 2 3
        TICK
        H 1 4 5 6
    """
            )
        ).with_whole_rotation_layers_slid_earlier()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        CX 0 1
        TICK
        S 7
        H 1 4 5 6
        TICK
        SWAP 2 3
    """
            )
        )
    )


def test_with_whole_layers_slid_as_early_as_possible_for_merge_with_same_layer():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        R 0 1
        TICK

        R 0 1
        TICK

        CX 0 1
        TICK

        CX 1 0
        TICK

        CX 0 1
        TICK

        M 5
        DETECTOR rec[-1]
        TICK

        R 2 3
        TICK

        CX 2 3
        TICK

        CX 3 4
    """
            )
        )
        .with_whole_layers_slid_as_to_merge_with_previous_layer_of_same_type(LayerReset)
        .with_whole_layers_slid_as_early_as_possible_for_merge_with_same_layer(LayerInteract)
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        R 0 1
        TICK

        R 0 1 2 3
        TICK

        CX 0 1 2 3
        TICK

        CX 1 0 3 4
        TICK

        CX 0 1
        TICK

        M 5
        DETECTOR rec[-1]
    """
            )
        )
    )


def test_with_cleaned_up_loop_iterations():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        R 0 1
        TICK
        S 2
        TICK

        REPEAT 5 {
            R 0 1
            TICK
            S 2
            TICK
        }
    """
            )
        ).with_cleaned_up_loop_iterations()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        REPEAT 6 {
            R 0 1
            TICK
            S 2
            TICK
        }
    """
            )
        ).without_empty_layers()
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        H 1

        R 0 1
        TICK
        S 2
        TICK

        REPEAT 5 {
            R 0 1
            TICK
            S 2
            TICK
        }
    """
            )
        ).with_cleaned_up_loop_iterations()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        H 1

        REPEAT 6 {
            R 0 1
            TICK
            S 2
            TICK
        }
    """
            )
        ).without_empty_layers()
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        R 0 1
        TICK
        S 2
        TICK

        H 1

        R 0 1
        TICK
        S 2
        TICK

        R 0 1
        TICK
        S 2
        TICK

        REPEAT 5 {
            R 0 1
            TICK
            S 2
            TICK
        }
    """
            )
        ).with_cleaned_up_loop_iterations()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
        R 0 1
        TICK
        S 2
        TICK

        H 1

        REPEAT 7 {
            R 0 1
            TICK
            S 2
            TICK
        }
    """
            )
        ).without_empty_layers()
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                    R 0 1
                    TICK
                    S 2
                    TICK

                    H 1

                    R 0 1
                    TICK
                    S 2
                    TICK

                    R 0 1
                    TICK
                    S 2
                    TICK

                    REPEAT 5 {
                        R 0 1
                        TICK
                        S 2
                        TICK
                    }

                    R 0 1
                    TICK
                    S 2
                    TICK
                """
            )
        ).with_cleaned_up_loop_iterations()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                    R 0 1
                    TICK
                    S 2
                    TICK

                    H 1

                    REPEAT 8 {
                        R 0 1
                        TICK
                        S 2
                        TICK
                    }
                """
            )
        ).without_empty_layers()
    )

    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                    R 0 1
                    TICK
                    S 2
                    TICK

                    H 1

                    R 0 1
                    TICK
                    S 2
                    TICK

                    R 0 1
                    TICK
                    S 2
                    TICK

                    REPEAT 5 {
                        R 0 1
                        TICK
                        S 2
                        TICK
                    }

                    R 0 1
                    TICK
                    S 2
                    TICK

                    R 0 1
                    TICK
                    S 2
                    TICK

                    H 0
                    TICK

                    R 0 1
                    TICK
                    S 2
                    TICK
                """
            )
        ).with_cleaned_up_loop_iterations()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                    R 0 1
                    TICK
                    S 2
                    TICK

                    H 1

                    REPEAT 9 {
                        R 0 1
                        TICK
                        S 2
                        TICK
                    }

                    H 0
                    TICK

                    R 0 1
                    TICK
                    S 2
                    TICK
                """
            )
        ).without_empty_layers()
    )


def test_with_locally_merged_measure_layers():
    assert (
        LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                R 0 1 2
                TICK
                CX 0 1
                TICK
                CX 2 1
                TICK
                M 1
                DETECTOR rec[-1]
                TICK
                M 0 2
                DETECTOR rec[-1] rec[-2] rec[-3]
            """
            )
        )
        .with_locally_merged_measure_layers()
        .with_locally_optimized_layers()
        == LayerCircuit.from_stim_circuit(
            stim.Circuit(
                """
                R 0 1 2
                TICK
                CX 0 1
                TICK
                CX 2 1
                TICK
                M 1 0 2
                DETECTOR rec[-3]
                DETECTOR rec[-1] rec[-2] rec[-3]
            """
            )
        )
    )
