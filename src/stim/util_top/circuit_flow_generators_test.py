import stim


def test_solve_flow_measurements():
    assert (
        stim.Circuit(
            """
        M 2
    """
        ).solve_flow_measurements(
            [
                stim.Flow("X2 -> X2"),
            ]
        )
        == [None]
    )

    assert (
        stim.Circuit(
            """
        M 2
    """
        ).solve_flow_measurements(
            [
                stim.Flow("X2 -> X2"),
                stim.Flow("Y2 -> Y2"),
                stim.Flow("Z2 -> Z2"),
                stim.Flow("Z2 -> 1"),
            ]
        )
        == [None, None, [], [0]]
    )

    assert (
        stim.Circuit(
            """
        MXX 0 1
    """
        ).solve_flow_measurements(
            [
                stim.Flow("YY -> ZZ"),
                stim.Flow("YY -> YY"),
                stim.Flow("YZ -> ZY"),
            ]
        )
        == [[0], [], [0]]
    )


def test_solve_flow_measurements_fewer_measurements_heuristic():
    assert (
        stim.Circuit(
            """
        MPP Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8
        M 0 1 2 3 4 5 6 7 8
    """
        ).solve_flow_measurements(
            [
                stim.Flow("1 -> Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8"),
            ]
        )
        == [[0]]
    )

    assert (
        stim.Circuit(
            """
        MPP Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8
        M 0 1 2 3 4 5 6 7 8
    """
        ).solve_flow_measurements(
            [
                stim.Flow("Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8 -> 1"),
            ]
        )
        == [[0]]
    )

    assert (
        stim.Circuit(
            """
        M 0 1 2 3 4 5 6 7 8
        MPP Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8
    """
        ).solve_flow_measurements(
            [
                stim.Flow("1 -> Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8"),
            ]
        )
        == [[9]]
    )

    assert (
        stim.Circuit(
            """
        M 0 1 2 3 4 5 6 7 8
        MPP Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8
    """
        ).solve_flow_measurements(
            [
                stim.Flow("Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8 -> 1"),
            ]
        )
        == [[9]]
    )
