import pytest
import stim # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers import leakage_tag_parsing_tableau as ltp
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageConditioningParams,
    LeakageMeasurementParams
)

# ##############################################################################
# Tests for successful parsing
# ##############################################################################


def test_parse_tags_successfully():
    """Tests that all valid leakage tags are parsed correctly."""
    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3) (0.2, 4<->U)]  0 1 2 3
    II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 2_2-->3_2) (0.2, 2_U<->U_3) (0.3, 4_U-->5_V)] 0 1 2 3
    M[LEAKAGE_PROJECTION_Z: (0.1, 2) (0.2, 3)] 0
    MPAD[LEAKAGE_MEASUREMENT: (0.1, 0) (0.9, 2) : 1 3 5 7]
    DEPOLARIZE1[CONDITIONED_ON_OTHER: U 2 3: 10 12 14 16](0.1) 9 11 13 15
    CZ[CONDITIONED_ON_PAIR: (2,3) (U,2) (U,3)] 20 21
    I[UNRELATED TAG]
    """
    )
    results = [
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
        LeakageMeasurementParams(
            args=((0.1, 2), (0.2, 3)), 
            targets=None, 
            from_tag="LEAKAGE_PROJECTION_Z: (0.1, 2) (0.2, 3)"
        ),
        LeakageMeasurementParams(
            args=((0.1, 0), (0.9, 2)),
            targets=(1, 3, 5, 7),
            from_tag="LEAKAGE_MEASUREMENT: (0.1, 0) (0.9, 2) : 1 3 5 7",
        ),
        LeakageConditioningParams(
            args=(("U", 2, 3),),
            targets=(10, 12, 14, 16),
            from_tag="CONDITIONED_ON_OTHER: U 2 3: 10 12 14 16",
        ),
        LeakageConditioningParams(
            args=((2, "U", "U"), (3, 2, 3)),
            targets=None,
            from_tag="CONDITIONED_ON_PAIR: (2,3) (U,2) (U,3)",
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
        II_ERROR[LEAKAGE_CONTROLLED_ERROR: (0.1, 2-->X) (0.2, 3-->Z)] 0 1 2 3
        """
    )
    with pytest.raises(ValueError, match="Use CONDITIONED_ON"):
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
