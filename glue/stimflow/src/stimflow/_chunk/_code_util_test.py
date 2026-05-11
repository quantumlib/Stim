import pytest
import stim

import stimflow


def test_verify_distance_is_at_least_23():
    stimflow.verify_distance_is_at_least_2(
        stim.Circuit(
            """
            R 0
            X_ERROR(0.125) 0
            M 0
            """
        )
    )

    stimflow.verify_distance_is_at_least_2(
        stim.Circuit(
            """
            R 0
            X_ERROR(0.125) 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-1]
            """
        )
    )

    stimflow.verify_distance_is_at_least_2(
        stim.Circuit(
            """
            R 0
            X_ERROR(0.125) 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-1]
            """
        ).detector_error_model()
    )

    with pytest.raises(ValueError, match="distance 1 error"):
        stimflow.verify_distance_is_at_least_2(
            stim.Circuit(
                """
                R 0
                X_ERROR(0.125) 0
                M 0
                OBSERVABLE_INCLUDE(0) rec[-1]
                """
            )
        )

    with pytest.raises(ValueError, match="distance 1 error"):
        stimflow.verify_distance_is_at_least_3(
            stim.Circuit(
                """
                R 0
                X_ERROR(0.125) 0
                M 0
                OBSERVABLE_INCLUDE(0) rec[-1]
                """
            )
        )

    stimflow.verify_distance_is_at_least_2(
        stim.Circuit.generated(
            code_task="repetition_code:memory",
            distance=2,
            rounds=3,
            after_clifford_depolarization=1e-3,
        )
    )

    stimflow.verify_distance_is_at_least_2(
        stim.Circuit.generated(
            code_task="repetition_code:memory",
            distance=3,
            rounds=3,
            after_clifford_depolarization=1e-3,
        )
    )

    stimflow.verify_distance_is_at_least_2(
        stim.Circuit.generated(
            code_task="repetition_code:memory",
            distance=9,
            rounds=3,
            after_clifford_depolarization=1e-3,
        )
    )

    stimflow.verify_distance_is_at_least_3(
        stim.Circuit.generated(
            code_task="repetition_code:memory",
            distance=3,
            rounds=3,
            after_clifford_depolarization=1e-3,
        )
    )

    stimflow.verify_distance_is_at_least_3(
        stim.Circuit.generated(
            code_task="repetition_code:memory",
            distance=9,
            rounds=3,
            after_clifford_depolarization=1e-3,
        )
    )

    with pytest.raises(ValueError, match="distance 2 error"):
        stimflow.verify_distance_is_at_least_3(
            stim.Circuit.generated(
                code_task="repetition_code:memory",
                distance=2,
                rounds=3,
                after_clifford_depolarization=1e-3,
            )
        )


def test_transversal_code_transition_chunk():
    prev_code = stimflow.StabilizerCode(
        stabilizers=[
            stimflow.PauliMap.from_zs([0, 1]),
            stimflow.PauliMap.from_zs([1 + 2j, 2 + 2j]),
            stimflow.PauliMap.from_xs([1j, 2j]),
            stimflow.PauliMap.from_xs([2, 2 + 1j]),
            stimflow.PauliMap.from_xs([0 + 0j, 1 + 0j, 0 + 1j, 1 + 1j]),
            stimflow.PauliMap.from_zs([1 + 0j, 2 + 0j, 1 + 1j, 2 + 1j]),
            stimflow.PauliMap.from_zs([0 + 1j, 1 + 1j, 0 + 2j, 1 + 2j]),
            stimflow.PauliMap.from_xs([1 + 1j, 2 + 1j, 1 + 2j, 2 + 2j]),
        ],
        logicals=[
            (
                stimflow.PauliMap.from_xs([0, 1, 2]).with_name("X"),
                stimflow.PauliMap.from_zs([0j + 1, 1j + 1, 2j + 1]).with_name("Z"),
            )
        ],
    )
    prev_code.verify()
    assert prev_code.find_distance(max_search_weight=2) == 3
    next_code = prev_code.with_transformed_coords(lambda e: -e.real + e.imag * 1j + 3)

    prev_chunk, reflow, next_chunk = stimflow.transversal_code_transition_chunks(
        prev_code=prev_code,
        next_code=next_code,
        measured=stimflow.PauliMap.from_xs(prev_code.data_set - next_code.data_set),
        reset=stimflow.PauliMap.from_xs(next_code.data_set - prev_code.data_set),
    )
    prev_chunk.verify(expected_in=prev_code)
    next_chunk.verify(expected_out=next_code)
    reflow.verify(expected_in=prev_chunk.end_interface(), expected_out=next_chunk.start_interface())

    compiler = stimflow.ChunkCompiler()
    compiler.append_magic_init_chunk()
    compiler.append(prev_chunk)
    compiler.append(reflow)
    compiler.append(next_chunk)
    compiler.append_magic_end_chunk()
    circuit = compiler.finish_circuit()
    noisy_circuit = stimflow.NoiseModel.uniform_depolarizing(1e-3).noisy_circuit_skipping_mpp_boundaries(
        circuit
    )
    assert len(noisy_circuit.shortest_graphlike_error()) == 2
