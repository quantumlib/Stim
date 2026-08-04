import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_parameters import LeakageParams
from stimside.op_handlers.leakage_handlers.tag_registry import parse_leakage_tag as _unified_parse_tag
from stimside.op_handlers.leakage_handlers.tag_registry import _parse_leakage_in_circuit_recurse as _unified_parse_circuit


def parse_leakage_tag(op: stim.CircuitInstruction) -> LeakageParams | None:
    """parse the string leakage tag to extract the included numbers.

    args:
        op: the stim.CircuitInstruction to parse the tag for

    returns:
        a LeakageParameters object corresponding to the parsed instruction

    raises:
        Value errors if:
            the leakage tag name is unrecognised
    """
    gd = stim.gate_data(op.name)
    tag = op.tag

    # start unpacking - if it doesn't start with LEAKAGE, given up immediately
    if not tag.startswith("LEAKAGE"):
        return None
    
    if tag.startswith("LEAKAGE_MEASUREMENT"):
        if op.name != "MPAD":
            raise ValueError("Only MPAD can have a LEAKAGE_MEASUREMENT tag.")
        return _parse_leakage_measurement(tag)
    elif tag == "LEAKAGE_SWAP":
        if op.name not in ["II_ERROR", "SWAP", "II"]:
            raise ValueError("Only II_ERROR and SWAP can have a LEAKAGE_SWAP tag.")
        return None
    elif tag.startswith("LEAKAGE_DETECTOR"):
        if op.name != "DETECTOR":
            raise ValueError("Only DETECTOR can have a LEAKAGE_DETECTOR tag.")
        return None

    # from here on out, we raise an error on anything malformed
    match = LEAKAGE_TAG_MATCH.fullmatch(tag)
    if match is None:  # the tag failed the regex:
        raise ValueError(
            f"Malformed LEAKAGE tag {tag}. "
            "If a tag begins with LEAKAGE, we demand it match the pattern "
            "LEAKAGE_NAME: (arg) (arg) ..."
        )
    name = match.group("name")
    args = match.group("args")

    args_tuples = []
    if args:
        stripped_args = args.strip()
        if not stripped_args:
            raise ValueError(f"Empty arguments in tag '{tag}'")
        if not (stripped_args.startswith("(") and stripped_args.endswith(")")):
            raise ValueError(
                f"Arguments must be enclosed in parentheses in tag '{tag}'"
            )
        # Strip outer parens and split by ') ('
        args_list = stripped_args[1:-1].split(") (")
        args_tuples = [tuple(b.strip() for b in a.split(",")) for a in args_list]

    # Check tag is attached to a reasonable gate
    # first check qubit-arity
    if (
        name in ["LEAKAGE_TRANSITION_1", "LEAKAGE_TRANSITION_Z", "LEAKAGE_PROJECTION_Z"]
        and not gd.is_single_qubit_gate
    ):
        raise ValueError(
            f"1Q leakage tag {op.tag} attached to not-1Q stim gate {op.name}. "
        )
    if (
        name in ["LEAKAGE_CONTROLLED_ERROR", "LEAKAGE_TRANSITION_2"]
        and not gd.is_two_qubit_gate
    ):
        raise ValueError(
            f"2Q leakage tag {op.tag} attached to not-2Q stim gate {op.name}. "
        )

    # then check specific gate attachment
    if name == "LEAKAGE_PROJECTION_Z":
        if op.name != "M":
            raise ValueError(f"LEAKAGE_PROJECTION_Z must be attached to an M gate")
    elif name == "LEAKAGE_TRANSITION_Z":
        if op.name not in ["I", "I_ERROR", "R"]:
            raise ValueError(
                f"LEAKAGE_TRANSITION_Z must be attached to an I, I_ERROR or R gate"
            )
    elif op.name not in ["I", "I_ERROR", "II", "II_ERROR"]:
        raise ValueError(
            f"Leakage tag {name} must be attached to a trivially acting gate "
            f"(I, I_ERROR, II, II_ERROR)"
        )

    if name in TAG_PARSERS:
        return TAG_PARSERS[name](args_tuples, tag)

    if name in TAG_PARSERS:
        raise ValueError(
            f"Failed to recognise existing leakage tag name {name}. "
            "This one is on us, not you. File a bug."
        )
    raise ValueError(
        f"Unrecognised LEAKAGE tag name {name}: "
        f"must be one of {list(TAG_PARSERS.keys())}"
    )


def parse_leakage_in_circuit(
    circuit: stim.Circuit,
) -> dict[stim.CircuitInstruction, LeakageParams]:
    """Parse all present leakage tags in a circuit, including inside repeats."""
    return _unified_parse_circuit(circuit, simulator="flip")
