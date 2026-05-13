import stim

import stimflow
from stimflow._chunk._flow_util import (
    _solve_auto_flow_ms,
    mbqc_to_unitary_by_solving_feedback,
)


def test_solve_flow_auto_measurements():
    failure_out = []
    assert (
        _solve_auto_flow_ms(
            flows=[
                stimflow.Flow(
                    start=stimflow.PauliMap({"Z": [0 + 1j, 2 + 1j]}), center=-1, flags={"X"}
                )
            ],
            circuit=stim.Circuit(
                """
                R 1
                CX 0 1 2 1
                M 1
                """
            ),
            q2i={0 + 1j: 0, 1 + 1j: 1, 2 + 1j: 2},
            o2i={},
            failure_out=failure_out,
        )
        == [
            stimflow.Flow(
                start=stimflow.PauliMap({"Z": [0 + 1j, 2 + 1j]}), measurement_indices=[0], center=-1, flags={"X"}
            ),
        ]
    )
    assert not failure_out


def test_solve_flow_auto_flow_measurements_with_observable():
    failure_out = []
    assert (
        _solve_auto_flow_ms(
            flows=[
                stimflow.Flow(
                    start=stimflow.PauliMap.from_xs([1, 2]).with_obs_name("L2"),
                    end=stimflow.PauliMap.from_zs([1, 2]).with_obs_name("L2"),
                )
            ],
            circuit=stim.Circuit(
                """
                MYY 1 2
                """
            ),
            q2i={1: 1, 2: 2},
            o2i={"L2": 3},
            failure_out=[],
        )
        == [
            stimflow.Flow(
                start=stimflow.PauliMap.from_xs([1, 2]).with_obs_name("L2"),
                end=stimflow.PauliMap.from_zs([1, 2]).with_obs_name("L2"),
                measurement_indices=[0],
            ),
        ]
    )
    assert not failure_out


def test_mbqc_to_unitary_by_solving_feedback():
    assert (
        mbqc_to_unitary_by_solving_feedback(
            stim.Circuit(
                """
                    MX 1
                    MZZ 0 1
                    MY 1
                    MXX 0 1
                    MZ 1
                    MX 1
                    MZZ 0 1
                    MY 1
                """
            ),
            desired_flow_generators=stim.gate_data("H").flows,
            num_relevant_qubits=1,
        )
        == stim.Circuit(
            """
                MX 1
                MZZ 0 1
                MY 1
                MXX 0 1
                M 1
                MX 1
                MZZ 0 1
                MY 1
                CX rec[-8] 0 rec[-7] 0
                CY rec[-5] 0 rec[-4] 0
                CZ rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0
            """
        )
    )

    assert (
        mbqc_to_unitary_by_solving_feedback(
            stim.Circuit(
                """
                    MX 1
                    MZZ 0 1
                    MY 1
                    MXX 0 1
                    MZ 1
                    MX 1
                    MZZ 0 1
                    MY 1
                """
            ),
            desired_flow_generators=stim.gate_data("SQRT_Y").flows,
            num_relevant_qubits=1,
        )
        == stim.Circuit(
            """
                MX 1
                MZZ 0 1
                MY 1
                MXX 0 1
                M 1
                MX 1
                MZZ 0 1
                MY 1
                X 0
                CX rec[-8] 0 rec[-7] 0
                CY rec[-5] 0 rec[-4] 0
                CZ rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0
            """
        )
    )

    assert (
        mbqc_to_unitary_by_solving_feedback(
            stim.Circuit(
                """
                    MX 2
                    MZZ 0 2
                    MXX 1 2
                    MZ 2
                """
            ),
            desired_flow_generators=stim.gate_data("CX").flows,
            num_relevant_qubits=2,
        )
        == stim.Circuit(
            """
                MX 2
                MZZ 0 2
                MXX 1 2
                M 2
                CX rec[-3] 1 rec[-1] 1
                CZ rec[-4] 0 rec[-2] 0
            """
        )
    )
