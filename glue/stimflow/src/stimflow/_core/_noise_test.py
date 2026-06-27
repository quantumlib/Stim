import stim

import stimflow
from stimflow._core._noise import (
    _iter_split_op_moments,
    _measure_basis,
    NoiseModel,
    occurs_in_classical_control_system,
)


def test_measure_basis():
    f = lambda e: _measure_basis(split_op=stim.Circuit(e)[0])
    assert f("H") is None
    assert f("H 0") is None
    assert f("R 0 1 2") is None

    assert f("MX") == "X"
    assert f("MX(0.01) 1") == "X"
    assert f("MY 0 1") == "Y"
    assert f("MZ 0 1 2") == "Z"
    assert f("M 0 1 2") == "Z"

    assert f("MRX") == "X"

    assert f("MPP X5") == "X"
    assert f("MPP X0*X2") == "XX"
    assert f("MPP Y0*Z2*X3") == "YZX"


def test_iter_split_op_moments():
    assert (
        list(
            _iter_split_op_moments(
                stim.Circuit(
                    """
    """
                ),
                immune_qubit_indices=set(),
            )
        )
        == []
    )

    assert (
        list(
            _iter_split_op_moments(
                stim.Circuit(
                    """
        H 0
    """
                ),
                immune_qubit_indices=set(),
            )
        )
        == [[stim.CircuitInstruction("H", [0])]]
    )

    assert (
        list(
            _iter_split_op_moments(
                stim.Circuit(
                    """
        H 0
        TICK
    """
                ),
                immune_qubit_indices=set(),
            )
        )
        == [[stim.CircuitInstruction("H", [0])]]
    )

    assert (
        list(
            _iter_split_op_moments(
                stim.Circuit(
                    """
        H 0 1
        TICK
    """
                ),
                immune_qubit_indices=set(),
            )
        )
        == [[stim.CircuitInstruction("H", [0, 1])]]
    )

    assert (
        list(
            _iter_split_op_moments(
                stim.Circuit(
                    """
        H 0 1
        TICK
    """
                ),
                immune_qubit_indices={3},
            )
        )
        == [[stim.CircuitInstruction("H", [0]), stim.CircuitInstruction("H", [1])]]
    )

    assert (
        list(
            _iter_split_op_moments(
                stim.Circuit(
                    """
        H 0
        TICK
        H 1
    """
                ),
                immune_qubit_indices=set(),
            )
        )
        == [[stim.CircuitInstruction("H", [0])], [stim.CircuitInstruction("H", [1])]]
    )

    assert (
        list(
            _iter_split_op_moments(
                stim.Circuit(
                    """
        CX rec[-1] 0 1 2 3 4
        MPP X5*X6 Y5
        CX 8 9 10 11
        TICK
        H 0
    """
                ),
                immune_qubit_indices=set(),
            )
        )
        == [
            [
                stim.CircuitInstruction("CX", [stim.target_rec(-1), 0]),
                stim.CircuitInstruction("CX", [1, 2]),
                stim.CircuitInstruction("CX", [3, 4]),
                stim.CircuitInstruction(
                    "MPP", [stim.target_x(5), stim.target_combiner(), stim.target_x(6)]
                ),
                stim.CircuitInstruction("MPP", [stim.target_y(5)]),
                stim.CircuitInstruction("CX", [8, 9, 10, 11]),
            ],
            [stim.CircuitInstruction("H", [0])],
        ]
    )


def test_occurs_in_classical_control_system():
    assert not occurs_in_classical_control_system(op=stim.CircuitInstruction("H", [0]))
    assert not occurs_in_classical_control_system(op=stim.CircuitInstruction("CX", [0, 1, 2, 3]))
    assert not occurs_in_classical_control_system(op=stim.CircuitInstruction("M", [0, 1, 2, 3]))

    assert occurs_in_classical_control_system(
        op=stim.CircuitInstruction("CX", [stim.target_rec(-1), 0])
    )
    assert occurs_in_classical_control_system(
        op=stim.CircuitInstruction("DETECTOR", [stim.target_rec(-1)])
    )
    assert occurs_in_classical_control_system(op=stim.CircuitInstruction("TICK", []))
    assert occurs_in_classical_control_system(op=stim.CircuitInstruction("SHIFT_COORDS", []))


def test_si_1000():
    model = NoiseModel.si1000(1e-3)
    assert (
        model.noisy_circuit(
            stim.Circuit(
                """
        R 0 1 2 3
        TICK
        ISWAP 0 1 2 3 4 5
        TICK
        H 4 5 6 7
        TICK
        M 0 1 2 3
    """
            )
        )
        == stim.Circuit(
            """
        R 0 1 2 3
        X_ERROR(0.002) 0 1 2 3
        DEPOLARIZE1(0.0001) 4 5 6 7
        DEPOLARIZE1(0.002) 4 5 6 7
        TICK
        ISWAP 0 1 2 3 4 5
        DEPOLARIZE2(0.001) 0 1 2 3 4 5
        DEPOLARIZE1(0.0001) 6 7
        TICK
        H 4 5 6 7
        DEPOLARIZE1(0.0001) 4 5 6 7 0 1 2 3
        TICK
        M(0.005) 0 1 2 3
        DEPOLARIZE1(0.001) 0 1 2 3
        DEPOLARIZE1(0.0001) 4 5 6 7
        DEPOLARIZE1(0.002) 4 5 6 7
    """
        )
    )


def test_measure_any():
    model = NoiseModel(
        any_clifford_1q_rule=stimflow.NoiseRule(after={}),
        any_clifford_2q_rule=stimflow.NoiseRule(after={}),
        any_measurement_rule=stimflow.NoiseRule(after={"DEPOLARIZE1": 0.125}, flip_result=0.25),
        measure_rules={"XX": stimflow.NoiseRule(flip_result=0.375, after={})},
    )
    assert (
        model.noisy_circuit(
            stim.Circuit(
                """
        H 0
        TICK
        CX 0 1
        TICK
        M 0 1
        TICK
        MPP Z0*Z1 X2*X3 X4*X5*X6
    """
            )
        )
        == stim.Circuit(
            """
        H 0
        TICK
        CX 0 1
        TICK
        M(0.25) 0 1
        DEPOLARIZE1(0.125) 0 1
        TICK
        MPP(0.25) Z0*Z1
        MPP(0.375) X2*X3
        MPP(0.25) X4*X5*X6
        DEPOLARIZE1(0.125) 0 1 4 5 6
    """
        )
    )


def test_tick_depolarization():
    model = NoiseModel(
        any_clifford_1q_rule=stimflow.NoiseRule(after={}),
        any_clifford_2q_rule=stimflow.NoiseRule(after={}),
        tick_noise=stimflow.NoiseRule(after={"DEPOLARIZE1": 0.375}),
        any_measurement_rule=stimflow.NoiseRule(after={}),
    )
    assert (
        model.noisy_circuit(
            stim.Circuit(
                """
        H 0
        TICK
        CX 0 1
        TICK
        M 0 1
        TICK
        MPP Z0*Z1 X2*X3 X4*X5*X6
    """
            )
        )
        == stim.Circuit(
            """
        H 0
        DEPOLARIZE1(0.375) 0 1 2 3 4 5 6
        TICK
        CX 0 1
        DEPOLARIZE1(0.375) 0 1 2 3 4 5 6
        TICK
        M 0 1
        DEPOLARIZE1(0.375) 0 1 2 3 4 5 6
        TICK
        MPP Z0*Z1 X2*X3 X4*X5*X6
        DEPOLARIZE1(0.375) 0 1 2 3 4 5 6
    """
        )
    )


def test_decomposed_gate_noise():
    assert (
        stimflow.NoiseModel(idle_depolarization=0.125, any_clifford_1q_rule=0.25).noisy_circuit(
            stim.Circuit(
                """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        H 0
        X 0
        TICK
        H 1
    """
            )
        )
        == stim.Circuit(
            """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        H 0
        X 0
        DEPOLARIZE1(0.25) 0
        DEPOLARIZE1(0.125) 1
        TICK
        H 1
        DEPOLARIZE1(0.25) 1
        DEPOLARIZE1(0.125) 0
    """
        )
    )

    assert (
        stimflow.NoiseModel(idle_depolarization=0.125, any_clifford_1q_rule=0.25).noisy_circuit(
            stim.Circuit(
                """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        X 0
        H 0
        TICK
        H 1
    """
            )
        )
        == stim.Circuit(
            """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        X 0
        H 0
        DEPOLARIZE1(0.25) 0
        DEPOLARIZE1(0.125) 1
        TICK
        H 1
        DEPOLARIZE1(0.25) 1
        DEPOLARIZE1(0.125) 0
    """
        )
    )

    assert (
        stimflow.NoiseModel(idle_depolarization=0.125, any_clifford_1q_rule=0.25).noisy_circuit(
            stim.Circuit(
                """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        X 0 1
        H 0
        TICK
        H 1
    """
            )
        )
        == stim.Circuit(
            """
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        X 0 1
        H 0
        DEPOLARIZE1(0.25) 1 0
        TICK
        H 1
        DEPOLARIZE1(0.25) 1
        DEPOLARIZE1(0.125) 0
    """
        )
    )
