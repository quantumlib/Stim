import stim # type: ignore[import-untyped]

import stimside.util.known_states as ksu


def test_known_states_measurements():
    circuit = stim.Circuit(
        """
        R 0 1
        X 1
        M 0 1
        H 0
        M 0 1
        MPAD 1
        M 0 1
    """
    )
    known_measurements = ksu.compute_known_measurements(circuit)
    assert known_measurements == [False, True, None, True, True, None, True]

    known_states = ksu.compute_known_states(circuit)
    assert known_states == [
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
        [stim.PauliString("+Z"), stim.PauliString("-Z")],
        [stim.PauliString("+Z"), stim.PauliString("-Z")],
        [stim.PauliString("+X"), stim.PauliString("-Z")],
        [stim.PauliString("+_"), stim.PauliString("-Z")],
        [stim.PauliString("+_"), stim.PauliString("-Z")],
        [stim.PauliString("+_"), stim.PauliString("-Z")],
    ]


def test_known_states_cxs():
    circuit = stim.Circuit(
        """
        R 0 1
        H 0
        CX 0 1
        TICK
        CX 1 0
        TICK
        CX 0 1
        H 1
        M 0 1
    """
    )
    known_measurements = ksu.compute_known_measurements(circuit)
    assert known_measurements == [False, False]

    known_states = ksu.compute_known_states(circuit)
    assert known_states == [
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
        [stim.PauliString("+X"), stim.PauliString("+Z")],
        [stim.PauliString("+_"), stim.PauliString("+_")],
        [stim.PauliString("+_"), stim.PauliString("+_")],
        [stim.PauliString("+_"), stim.PauliString("+_")],
        [stim.PauliString("+_"), stim.PauliString("+_")],
        [stim.PauliString("+Z"), stim.PauliString("+X")],
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
    ]


def test_known_states_pair_measurements():
    circuit = stim.Circuit(
        """
        R 0 1
        MZZ 0 1
        TICK
        MZZ 0 1
        M 0 1
    """
    )
    known_measurements = ksu.compute_known_measurements(circuit)
    assert known_measurements == [False, False, False, False]

    known_states = ksu.compute_known_states(circuit)
    assert known_states == [
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
        [stim.PauliString("+_"), stim.PauliString("+_")],
        [stim.PauliString("+_"), stim.PauliString("+_")],
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
        [stim.PauliString("+Z"), stim.PauliString("+Z")],
    ]


def test_known_states_flipfloping():
    circuit = stim.Circuit(
        """
        R 0
        H 0
        TICK[keep the two Hs from combining]
        H 0
        M 0
        H 0
        M 0
        H 0
        M 0
    """
    )
    known_measurements = ksu.compute_known_measurements(circuit)
    assert known_measurements == [0] + [None] * 2

    known_states = ksu.compute_known_states(circuit)
    assert known_states == [
        [stim.PauliString("+Z")],  # init
        [stim.PauliString("+Z")],  # R 0
        [stim.PauliString("+X")],  # H 0
        [stim.PauliString("+X")],  # TICK
        [stim.PauliString("+Z")],  # H 0
        [stim.PauliString("+Z")],  # M 0, known meas
        [stim.PauliString("+X")],  # H 0
        [stim.PauliString("+_")],  # M 0, unknown meas
        [stim.PauliString("+_")],  # H 0
        [stim.PauliString("+_")],  # M 0
    ]


def test_known_states_with_repeat():
    circuit = stim.Circuit(
        """
        R 0
        REPEAT 5 {
            H 0
            TICK
        }
        REPEAT 5 {
            H 0
            M 0
            TICK
        }
    """
    )
    known_measurements = ksu.compute_known_measurements(circuit)
    assert known_measurements == [0] + [None] * 4

    known_states = ksu.compute_known_states(circuit)
    assert known_states == [
        [stim.PauliString("+Z")],  # init
        [stim.PauliString("+Z")],  # after R 0
        [stim.PauliString("+X")],  # H 0
        [stim.PauliString("+X")],  # TICK
        [stim.PauliString("+Z")],  # H 0
        [stim.PauliString("+Z")],  # TICK
        [stim.PauliString("+X")],  # H 0
        [stim.PauliString("+X")],  # TICK
        [stim.PauliString("+Z")],  # H 0
        [stim.PauliString("+Z")],  # TICK
        [stim.PauliString("+X")],  # H 0
        [stim.PauliString("+X")],  # TICK, end of first repeat
        [stim.PauliString("+Z")],  # H 0
        [stim.PauliString("+Z")],  # M 0, known measurement
        [stim.PauliString("+Z")],  # TICK
        [stim.PauliString("+X")],  # H 0
        [stim.PauliString("+_")],  # M 0, unknown measurement
        [stim.PauliString("+_")],  # TICK
        [stim.PauliString("+_")],  # H 0
        [stim.PauliString("+_")],  # M 0
        [stim.PauliString("+_")],  # TICK
        [stim.PauliString("+_")],  # H 0
        [stim.PauliString("+_")],  # M 0
        [stim.PauliString("+_")],  # TICK
        [stim.PauliString("+_")],  # H 0
        [stim.PauliString("+_")],  # M 0
        [stim.PauliString("+_")],  # TICK, end of first repeat
    ]


def test_convert_paulis_to_arrays():
    known_states = [
        stim.PauliString("+X"),
        stim.PauliString("-X"),
        stim.PauliString("+Y"),
        stim.PauliString("-Y"),
        stim.PauliString("+Z"),
        stim.PauliString("-Z"),
        stim.PauliString("_"),
    ]
    in_pX, in_mX, in_pY, in_mY, in_pZ, in_mZ = ksu.convert_paulis_to_arrays(known_states)
    assert in_pX.tolist() == [True, False, False, False, False, False, False]
    assert in_mX.tolist() == [False, True, False, False, False, False, False]
    assert in_pY.tolist() == [False, False, True, False, False, False, False]
    assert in_mY.tolist() == [False, False, False, True, False, False, False]
    assert in_pZ.tolist() == [False, False, False, False, True, False, False]
    assert in_mZ.tolist() == [False, False, False, False, False, True, False]
