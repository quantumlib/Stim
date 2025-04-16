import stim


def test_solve_flow_measurements():
    assert stim.Circuit("""
        M 2
    """).solve_flow_measurements([
        stim.Flow("X2 -> X2"),
    ]) == [None]

    assert stim.Circuit("""
        M 2
    """).solve_flow_measurements([
        stim.Flow("X2 -> X2"),
        stim.Flow("Y2 -> Y2"),
        stim.Flow("Z2 -> Z2"),
        stim.Flow("Z2 -> 1"),
    ]) == [None, None, [], [0]]

    assert stim.Circuit("""
        MXX 0 1
    """).solve_flow_measurements([
        stim.Flow("YY -> ZZ"),
        stim.Flow("YY -> YY"),
        stim.Flow("YZ -> ZY"),
    ]) == [[0], [], [0]]
