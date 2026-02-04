import pytest
import stim # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers import leakage_tag_parsing_flip as ltp
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageControlledErrorParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageTransitionZParams,
    LeakageMeasurementParams
)

# ##############################################################################
# Tests for successful parsing
# ##############################################################################


def test_parse_tags_successfully():
    """Tests that all valid leakage tags are parsed correctly."""
    circuit = stim.Circuit(
        """
    II_ERROR[LEAKAGE_CONTROLLED_ERROR: (0.1, 2-->X) (0.2, 3-->Z)] 0 1 2 3
    I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3) (0.2, 4<->U)]  0 1 2 3
    II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 2_2-->3_2) (0.2, 2_U<->U_3) (0.3, 4_U-->5_V)] 0 1 2 3
    I_ERROR[LEAKAGE_TRANSITION_Z: (0.1, 2-->3) (0.2, 3<->4)] 0
    M[LEAKAGE_PROJECTION_Z: (0.1, 2) (0.2, 3)] 0
    I[UNRELATED TAG]
    """
    )
    results = [
        LeakageControlledErrorParams(
            args=((0.1, 2, "X"), (0.2, 3, "Z")),
            from_tag="LEAKAGE_CONTROLLED_ERROR: (0.1, 2-->X) (0.2, 3-->Z)",
        ),
        LeakageTransition1Params(
            args=((0.1, 2, 3), (0.2, 4, "U"), (0.2, "U", 4)),
            from_tag="LEAKAGE_TRANSITION_1: (0.1, 2-->3) (0.2, 4<->U)",
        ),
        LeakageTransition2Params(
            args=(
                (0.1, (2, 2), (3, 2)),
                (0.2, (2, "U"), ("U", 3)),
                (0.2, ("U", 3), (2, "U")),
                (0.3, (4, "U"), (5, "V")),
            ),
            from_tag="LEAKAGE_TRANSITION_2: (0.1, 2_2-->3_2) (0.2, 2_U<->U_3) (0.3, 4_U-->5_V)",
        ),
        LeakageTransitionZParams(
            args=((0.1, 2, 3), (0.2, 3, 4), (0.2, 4, 3)),
            from_tag="LEAKAGE_TRANSITION_Z: (0.1, 2-->3) (0.2, 3<->4)",
        ),
        LeakageMeasurementParams(
            args=((0.1, 2), (0.2, 3)), 
            targets=None, 
            from_tag="LEAKAGE_PROJECTION_Z: (0.1, 2) (0.2, 3)"
        ),
        None,
    ]
    for op, r in zip(circuit, results):
        got = ltp.parse_leakage_tag(op)
        assert got == r, f"Failed on parsing {op.tag}, expected {r}, got {got}"


# ##############################################################################
# Tests for expected failures
# ##############################################################################


def test_bad_tags():
    """Tests that various malformed and unrecognized tags raise ValueErrors."""
    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE MALFORMED TAG]
    """
    )
    with pytest.raises(ValueError, match="Malformed LEAKAGE tag"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE_UNRECOGNIZED_TAG: (arg)]
    """
    )
    with pytest.raises(ValueError, match="Unrecognised LEAKAGE tag"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_CONTROLLED_ERROR: BAD_ARGS ] 0 1
        """
    )
    with pytest.raises(ValueError, match="Arguments must be enclosed in parentheses"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_CONTROLLED_ERROR: (BAD_ARG, BAD_ARG) ]
        """
    )
    with pytest.raises(ValueError, match="Malformed parsed argument"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 1-->2, 3)] 0
        """
    )
    with pytest.raises(ValueError, match="Malformed parsed argument"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 1->2)] 0
        """
    )
    with pytest.raises(ValueError, match="Malformed LEAKAGE_TRANSITION_1 argument"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 1_1-->2_0, 3)] 0 1
        """
    )
    with pytest.raises(ValueError, match="Malformed parsed argument"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 1_1->2_0)] 0 1
        """
    )
    with pytest.raises(ValueError, match="Malformed LEAKAGE_TRANSITIONS_2 transitions argument"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_CONTROLLED_ERROR: (0.1, 1, Z)] 0 1
        """
    )
    with pytest.raises(ValueError, match="Malformed parsed argument"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_CONTROLLED_ERROR: ] 0 1
        """
    )
    with pytest.raises(ValueError, match="Empty arguments"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_CONTROLLED_ERROR: arg] 0 1
        """
    )
    with pytest.raises(ValueError, match="Arguments must be enclosed in parentheses"):
        ltp.parse_leakage_tag(circuit[0])
    """Tests that leakage tags are attached to gates of the correct arity."""
    circuit = stim.Circuit(
        """
    II_ERROR[LEAKAGE_TRANSITION_1: (0.1,2-->3) (0.2,3<->4)]  0 1 2 3
    """
    )
    with pytest.raises(ValueError, match="not-1Q stim gate"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE_TRANSITION_2: (0.1,2_2-->3_2) (0.2,2_U<->U_3)] 0 1 2 3
    """
    )
    with pytest.raises(ValueError, match="not-2Q stim gate"):
        ltp.parse_leakage_tag(circuit[0])


# ##############################################################################
# Tests for circuit-level parsing
# ##############################################################################


def test_parse_in_circuit():
    """Tests parsing leakage tags in a simple circuit, including REPEAT blocks."""
    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE_TRANSITION_1: (0.2, 2-->3)] 0
    REPEAT 2 {
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3)] 0
    }
    M 0
    """
    )
    parsed = ltp.parse_leakage_in_circuit(circuit)
    assert len(parsed) == 2
    assert parsed[circuit[0]] == LeakageTransition1Params(
        args=((0.2, 2, 3),), from_tag="LEAKAGE_TRANSITION_1: (0.2, 2-->3)"
    )
    assert parsed[circuit[1].body_copy()[0]] == LeakageTransition1Params(
        args=((0.1, 2, 3),), from_tag="LEAKAGE_TRANSITION_1: (0.1, 2-->3)"
    )


def test_parse_in_circuit_nested_repeats():
    """Tests parsing leakage tags in a circuit with nested REPEAT blocks."""
    circuit = stim.Circuit(
        """
    REPEAT 2 {
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3)] 0
        REPEAT 3 {
            I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3)] 0
        }
    }
    """
    )
    parsed = ltp.parse_leakage_in_circuit(circuit)
    assert len(parsed) == 1
    assert parsed[circuit[0].body_copy()[0]] == LeakageTransition1Params(
        args=((0.1, 2, 3),), from_tag="LEAKAGE_TRANSITION_1: (0.1, 2-->3)"
    )
    assert parsed[circuit[0].body_copy()[1].body_copy()[0]] == LeakageTransition1Params(
        args=((0.1, 2, 3),), from_tag="LEAKAGE_TRANSITION_1: (0.1, 2-->3)"
    )


def test_try_as_integer():
    """Tests the try_as_integer helper function."""
    assert ltp.try_as_integer("2") == 2
    assert ltp.try_as_integer("U") == "U"
