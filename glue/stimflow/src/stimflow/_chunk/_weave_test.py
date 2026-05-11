import pytest
import stim

from stimflow._chunk._weave import StimCircuitLoom


def test_sq():
    # check targets are correctly de-duplicated
    a = stim.Circuit(
        """
    S 0 1 2
    """
    )
    b = stim.Circuit(
        """
    S 3 2 4 1 5
    """
    )
    c = stim.Circuit(
        """
    S 0 1 2 3 4 5
    """
    )
    assert StimCircuitLoom.weave(a, b) == c

    with pytest.raises(ValueError, match="Duplicate"):
        a = stim.Circuit(
            """
            S 0 0
            """
        )
        b = stim.Circuit(
            """
            S 1
            """
        )
        StimCircuitLoom.weave(a, b)


def test_m():
    # check M targets are combined correctly,
    # and that later references to them are remapped correctly
    a = stim.Circuit(
        """
    M 0 1 2
    DETECTOR rec[-1]
    """
    )
    b = stim.Circuit(
        """
    M 3 4 5 0
    DETECTOR rec[-1]
    """
    )
    c = stim.Circuit(
        """
    M 0 1 2 3 4 5
    DETECTOR rec[-4]
    DETECTOR rec[-6]
    """
    )
    assert StimCircuitLoom.weave(a, b) == c


def test_det_skipping():
    a = stim.Circuit(
        """
    M 0 1 2
    DETECTOR rec[-3]
    DETECTOR rec[-2]
    DETECTOR rec[-1]
    TICK
    M 0 1 2
    DETECTOR rec[-1]
    """
    )
    b = stim.Circuit(
        """
    M
    TICK
    M
    """
    )  # annoyingly we need the tick to prevent the Ms combining
    c = stim.Circuit(
        """
    M 0 1 2
    DETECTOR rec[-3]
    DETECTOR rec[-2]
    DETECTOR rec[-1]
    TICK
    M 0 1 2
    DETECTOR rec[-1]
    """
    )
    assert StimCircuitLoom.weave(a, b) == c


def test_m_duplicates():
    # check M targets are combined correctly,
    # and that later references to them are remapped correctly
    a = stim.Circuit(
        """
    M 0 1
    DETECTOR rec[-2]
    DETECTOR rec[-1]
    """
    )
    b = stim.Circuit(
        """
    M 1 2
    DETECTOR rec[-2]
    DETECTOR rec[-1]
    """
    )
    c = stim.Circuit(
        """
    M 0 1 2
    DETECTOR rec[-3]
    DETECTOR rec[-2]
    DETECTOR rec[-2]
    DETECTOR rec[-1]
    """
    )
    assert StimCircuitLoom.weave(a, b) == c

    with pytest.raises(ValueError, match="Duplicate"):
        a = stim.Circuit(
            """
            M 0 0
            """
        )
        b = stim.Circuit(
            """
            M 1
            """
        )
        StimCircuitLoom.weave(a, b)

    with pytest.raises(ValueError, match="Duplicate"):
        a = stim.Circuit(
            """
            M 0
            """
        )
        b = stim.Circuit(
            """
            M 1 1
            """
        )
        StimCircuitLoom.weave(a, b)


def test_sweep_bits():
    a = stim.Circuit(
        """
    CX sweep[0] 0 sweep[1] 2
    """
    )
    b = stim.Circuit(
        """
    CX sweep[0] 1 sweep[1] 3
    """
    )

    sweep_bit_func = lambda circuit_idx, bit_idx: 10 * bit_idx + circuit_idx
    c = stim.Circuit(
        """
    CX sweep[00] 0 sweep[10] 2 sweep[01] 1 sweep[11] 3
    """
    )
    assert StimCircuitLoom.weave(a, b, sweep_bit_func) == c

    with pytest.raises(ValueError, match="sweep_bit_func is not provided"):
        StimCircuitLoom.weave(a, b)


def test_2q():
    a = stim.Circuit(
        """
    CX 0 1 2 3
    CZ 0 1
    CX sweep[0] 0 sweep[1] 2
    M 0 1
    CX rec[-2] 3 rec[-1] 1
    """
    )
    b = stim.Circuit(
        """
    CX 0 1 4 5
    CZ 0 1
    CX sweep[0] 1 sweep[1] 3
    M 0 2
    CX rec[-2] 0 rec[-1] 2
    """
    )
    sweep_bit_func = lambda circuit_idx, bit_idx: 2 * bit_idx + circuit_idx
    c = stim.Circuit(
        """
    CX 0 1 2 3 4 5
    CZ 0 1
    CX sweep[0] 0 sweep[2] 2 sweep[1] 1 sweep[3] 3
    M 0 1 2
    CX rec[-3] 3 rec[-2] 1 rec[-3] 0 rec[-1] 2
    """
    )
    assert StimCircuitLoom.weave(a, b, sweep_bit_func) == c

    with pytest.raises(ValueError, match="Duplicate"):
        a = stim.Circuit(
            """
            CX 0 1 0 1
            """
        )
        b = stim.Circuit(
            """
            CX 0 1
            """
        )
        StimCircuitLoom.weave(a, b)

    with pytest.raises(ValueError, match="Duplicate"):
        a = stim.Circuit(
            """
            CX 0 1
            """
        )
        b = stim.Circuit(
            """
            CX 0 1 0 1
            """
        )
        StimCircuitLoom.weave(a, b)

    with pytest.raises(ValueError, match="Duplicate"):
        a = stim.Circuit(
            """
            CX 0 1
            """
        )
        b = stim.Circuit(
            """
            CX 1 2
            """
        )
        StimCircuitLoom.weave(a, b)

    with pytest.raises(ValueError, match="Duplicate reversed"):
        a = stim.Circuit(
            """
            CZ 0 1
            """
        )
        b = stim.Circuit(
            """
            CZ 1 0
            """
        )
        StimCircuitLoom.weave(a, b)


def test_small_rep_code():
    # 0 1 2 3 4 5 6
    a = stim.Circuit(
        """
        R 1 3
        TICK
        CX 0 1 2 3
        TICK
        CX 2 1 4 3
        TICK
        M 1 3
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    """
    )
    b = stim.Circuit(
        """
        R 3 5
        TICK
        CX 2 3 4 5
        TICK
        CX 4 3 6 5
        TICK
        M 3 5
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    """
    )
    c = stim.Circuit(
        """
        R 1 3 5
        TICK
        CX 0 1 2 3 4 5
        TICK
        CX 2 1 4 3 6 5
        TICK
        M 1 3 5
        DETECTOR rec[-2]
        DETECTOR rec[-3]
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    """
    )
    assert StimCircuitLoom.weave(a, b) == c


def test_weaves_target_recs():
    # copied from rep code test
    a = stim.Circuit(
        """
        R 1 3
        TICK
        CX 0 1 2 3
        TICK
        CX 2 1 4 3
        TICK
        M 1 3
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    """
    )
    b = stim.Circuit(
        """
        R 3 5
        TICK
        CX 2 3 4 5
        TICK
        CX 4 3 6 5
        TICK
        M 3 5
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    """
    )
    c = stim.Circuit(
        """
        R 1 3 5
        TICK
        CX 0 1 2 3 4 5
        TICK
        CX 2 1 4 3 6 5
        TICK
        M 1 3 5
        DETECTOR rec[-2]
        DETECTOR rec[-3]
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    """
    )
    loom = StimCircuitLoom(a, b)
    assert loom.circuit == c  # should pass if above test passes

    assert loom.weaved_target_rec_from_c0(-2) == -3
    assert loom.weaved_target_rec_from_c0(-1) == -2
    assert loom.weaved_target_rec_from_c1(-2) == -2
    assert loom.weaved_target_rec_from_c1(-1) == -1

    assert loom.weaved_target_rec_from_c0(0) == -3
    assert loom.weaved_target_rec_from_c0(1) == -2
    assert loom.weaved_target_rec_from_c1(0) == -2
    assert loom.weaved_target_rec_from_c1(1) == -1
