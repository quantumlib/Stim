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


def test_solve_flow_generators_measurements_multi_target():
    assert stim.Circuit("""
        M 1 2
    """).flow_generators() == [
        stim.Flow("1 -> __Z xor rec[1]"),
        stim.Flow("1 -> _Z_ xor rec[0]"),
        stim.Flow("__Z -> rec[1]"),
        stim.Flow("_Z_ -> rec[0]"),
        stim.Flow("X__ -> X__"),
        stim.Flow("Z__ -> Z__"),
    ]

    assert stim.Circuit("""
        MX 1 2
    """).flow_generators() == [
        stim.Flow("1 -> __X xor rec[1]"),
        stim.Flow("1 -> _X_ xor rec[0]"),
        stim.Flow("__X -> rec[1]"),
        stim.Flow("_X_ -> rec[0]"),
        stim.Flow("X__ -> X__"),
        stim.Flow("Z__ -> Z__"),
    ]

    assert stim.Circuit("""
        MYY 1 2 3 4
    """).flow_generators() == [
        stim.Flow("1 -> ___YY xor rec[1]"),
        stim.Flow("1 -> _YY__ xor rec[0]"),
        stim.Flow("____Y -> ____Y"),
        stim.Flow("___XZ -> ___ZX xor rec[1]"),
        stim.Flow("___ZZ -> ___ZZ"),
        stim.Flow("__Y__ -> __Y__"),
        stim.Flow("_XZ__ -> _ZX__ xor rec[0]"),
        stim.Flow("_ZZ__ -> _ZZ__"),
        stim.Flow("X____ -> X____"),
        stim.Flow("Z____ -> Z____"),
    ]
    assert stim.Circuit("""
        MPP Y1*Y2 Y3*Y4
    """).flow_generators() == [
        stim.Flow("1 -> ___YY xor rec[1]"),
        stim.Flow("1 -> _YY__ xor rec[0]"),
        stim.Flow("____Y -> ____Y"),
        stim.Flow("___XZ -> ___ZX xor rec[1]"),
        stim.Flow("___ZZ -> ___ZZ"),
        stim.Flow("__Y__ -> __Y__"),
        stim.Flow("_XZ__ -> _ZX__ xor rec[0]"),
        stim.Flow("_ZZ__ -> _ZZ__"),
        stim.Flow("X____ -> X____"),
        stim.Flow("Z____ -> Z____"),
    ]


def test_solve_flow_measurements_multi_target():
    assert stim.Circuit("""
        M 1 2
    """).solve_flow_measurements([
        stim.Flow("Z1 -> 1"),
    ]) == [[0]]

    assert stim.Circuit("""
        MX 1 2
    """).solve_flow_measurements([
        stim.Flow("X1 -> 1"),
    ]) == [[0]]

    assert stim.Circuit("""
        MYY 1 2 3 4
    """).solve_flow_measurements([
        stim.Flow("Y1*Y2 -> 1"),
    ]) == [[0]]

    assert stim.Circuit("""
        MPP Y1*Y2 Y3*Y4
    """).solve_flow_measurements([
        stim.Flow("Y1*Y2 -> 1"),
    ]) == [[0]]


def test_solve_flow_measurements_fewer_measurements_heuristic():
    assert stim.Circuit("""
        MPP Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8
        M 0 1 2 3 4 5 6 7 8
    """).solve_flow_measurements([
        stim.Flow("1 -> Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8"),
    ]) == [[0]]

    assert stim.Circuit("""
        MPP Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8
        M 0 1 2 3 4 5 6 7 8
    """).solve_flow_measurements([
        stim.Flow("Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8 -> 1"),
    ]) == [[0]]

    assert stim.Circuit("""
        M 0 1 2 3 4 5 6 7 8
        MPP Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8
    """).solve_flow_measurements([
        stim.Flow("1 -> Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8"),
    ]) == [[9]]

    assert stim.Circuit("""
        M 0 1 2 3 4 5 6 7 8
        MPP Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8
    """).solve_flow_measurements([
        stim.Flow("Z0*Z1*Z2*Z3*Z4*Z5*Z6*Z7*Z8 -> 1"),
    ]) == [[9]]
