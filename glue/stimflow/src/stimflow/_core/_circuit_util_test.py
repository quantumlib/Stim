import stim

import stimflow


def test_circuit_with_xz_flipped():
    assert (
        stimflow.circuit_with_xz_flipped(
            stim.Circuit(
                """
                CX 0 1 2 3
                TICK
                H 0
                TICK
                REPEAT 10 {
                    MXX 0 1
                }
                """
            )
        )
        == stim.Circuit(
            """
            XCZ 0 1 2 3
            TICK
            H 0
            TICK
            REPEAT 10 {
                MZZ 0 1
            }
            """
        )
    )


def test_gates_used_by_circuit():
    assert (
        stimflow.gates_used_by_circuit(
            stim.Circuit(
                """
                H 0
                TICK
                CX 0 1
                """
            )
        )
        == {"H", "TICK", "CX"}
    )

    assert (
        stimflow.gates_used_by_circuit(
            stim.Circuit(
                """
                S 0
                XCZ 0 1
                """
            )
        )
        == {"S", "XCZ"}
    )

    assert (
        stimflow.gates_used_by_circuit(
            stim.Circuit(
                """
                MPP X0*X1 Z2*Z3*Z4 Y0*Z1
                """
            )
        )
        == {"MXX", "MZZZ", "MYZ"}
    )

    assert (
        stimflow.gates_used_by_circuit(
            stim.Circuit(
                """
                CX rec[-1] 1
                """
            )
        )
        == {"feedback"}
    )

    assert (
        stimflow.gates_used_by_circuit(
            stim.Circuit(
                """
                CX sweep[1] 1
                """
            )
        )
        == {"sweep"}
    )

    assert (
        stimflow.gates_used_by_circuit(
            stim.Circuit(
                """
                CX rec[-1] 1 0 1
                """
            )
        )
        == {"feedback", "CX"}
    )


def test_gate_counts_for_circuit():
    assert (
        stimflow.gate_counts_for_circuit(
            stim.Circuit(
                """
                H 0
                TICK
                CX 0 1
                """
            )
        )
        == {"H": 1, "TICK": 1, "CX": 1}
    )

    assert (
        stimflow.gate_counts_for_circuit(
            stim.Circuit(
                """
                S 0
                XCZ 0 1
                """
            )
        )
        == {"S": 1, "XCZ": 1}
    )

    assert (
        stimflow.gate_counts_for_circuit(
            stim.Circuit(
                """
                MPP X0*X1 Z2*Z3*Z4 Y0*Z1
                """
            )
        )
        == {"MXX": 1, "MZZZ": 1, "MYZ": 1}
    )

    assert (
        stimflow.gate_counts_for_circuit(
            stim.Circuit(
                """
                CX rec[-1] 1
                """
            )
        )
        == {"feedback": 1}
    )

    assert (
        stimflow.gate_counts_for_circuit(
            stim.Circuit(
                """
                CX sweep[1] 1
                """
            )
        )
        == {"sweep": 1}
    )

    assert (
        stimflow.gate_counts_for_circuit(
            stim.Circuit(
                """
                CX rec[-1] 1 0 1
                """
            )
        )
        == {"feedback": 1, "CX": 1}
    )

    assert (
        stimflow.gate_counts_for_circuit(
            stim.Circuit(
                """
                H 0 1
                REPEAT 100 {
                    S 0 1 2
                    CX 0 1 2 3
                }
                """
            )
        )
        == {"H": 2, "S": 300, "CX": 200}
    )


def test_count_measurement_layers():
    assert stimflow.count_measurement_layers(stim.Circuit()) == 0
    assert (
        stimflow.count_measurement_layers(
            stim.Circuit(
                """
                M 0 1 2
                """
            )
        )
        == 1
    )
    assert (
        stimflow.count_measurement_layers(
            stim.Circuit(
                """
                M 0 1
                MX 2
                MR 3
                """
            )
        )
        == 1
    )
    assert (
        stimflow.count_measurement_layers(
            stim.Circuit(
                """
                M 0 1
                MX 2
                TICK
                MR 3
                """
            )
        )
        == 2
    )
    assert (
        stimflow.count_measurement_layers(
            stim.Circuit(
                """
                R 0
                CX 0 1
                TICK
                M 0
                """
            )
        )
        == 1
    )
    assert (
        stimflow.count_measurement_layers(
            stim.Circuit(
                """
                R 0
                CX 0 1
                TICK
                M 0
                DETECTOR rec[-1]
                M 1
                DETECTOR rec[-1]
                OBSERVABLE_INCLUDE(0) rec[-1]
                MPP X0*X1
                DETECTOR rec[-1]
                """
            )
        )
        == 1
    )
    assert (
        stimflow.count_measurement_layers(
            stim.Circuit.generated("repetition_code:memory", distance=3, rounds=4)
        )
        == 4
    )
    assert (
        stimflow.count_measurement_layers(
            stim.Circuit.generated("surface_code:rotated_memory_x", distance=3, rounds=1000)
        )
        == 1000
    )


def test_append_reindexed_content_to_circuit():
    circuit = stim.Circuit(
        """
        H 5 1
        OBSERVABLE_INCLUDE(6) X7 rec[-3]
        OBSERVABLE_INCLUDE(5) rec[-1]
        OBSERVABLE_INCLUDE(1) Z7 rec[-5]
        DETECTOR rec[-2]
    """
    )
    new_circuit = stim.Circuit()
    stimflow.append_reindexed_content_to_circuit(
        content=circuit,
        qubit_i2i={5: 15, 7: 27},
        obs_i2i={6: 2, 1: "discard"},
        out_circuit=new_circuit,
    )
    assert new_circuit == stim.Circuit(
        """
        H 15 1
        OBSERVABLE_INCLUDE(2) X27 rec[-3]
        OBSERVABLE_INCLUDE(5) rec[-1]
        DETECTOR rec[-2]
    """
    )
