import re
from typing import Literal, cast

import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageControlledErrorParams,
    LeakageParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageTransitionZParams,
    LeakageMeasurementParams
)

LEAKAGE_TAG_MATCH = re.compile(r"(?P<name>LEAKAGE_\w+):(?P<args>.+)")
# you can use this by doing
# match = LEAKAGE_TAG_MATCH.fullmatch(tag)
# the result is None if the tag isn't formatted correctly
# otherwise the result has named groups:
#   match.group('name')
#   match.group('args')

MEASURE_TAG_MATCH = re.compile(r"LEAKAGE_MEASUREMENT\s*:(?P<args>.+):(?P<targets>.+)")
## "MPAD[LEAKAGE_MEASUREMENT: (0.1, 0) (0.2, 1) (0.9, 2) (0.99, 3) : 1 3 5 7]"

LEAKAGE_TRANSITIONS_1_MATCH = re.compile(
    r"(?P<s0>[\dU]+)(?P<direction>[-<]->)(?P<s1>[\dU]+)"
)
LEAKAGE_TRANSITIONS_2_MATCH = re.compile(
    r"(?P<s0>[\dU])_(?P<s1>[\dU])(?P<direction>[-<]->)(?P<s2>[\dUV])_(?P<s3>[\dUV])"
)


def _parse_leakage_controlled_error(
    args_tuples: list[tuple[str, ...]], tag: str
) -> LeakageControlledErrorParams:
    errors: list[tuple[float, int, Literal["X", "Y", "Z"]]] = []
    for a in args_tuples:
        if len(a) != 2:
            raise ValueError(
                f"Malformed parsed argument '{a}' in tag '{tag}'. "
                f"Arguments should be comma separated and in the form "
                "'(probability, leakage_state --> pauli)'"
            )
        a_parsed = [a[0]] + [s.strip() for s in a[1].split("-->")]
        if len(a_parsed) != 3:
            raise ValueError(
                f"Malformed parsed argument {a_parsed} in {tag}. "
                f"Arguments should be comma separated and in the form "
                "(probability, leakage_state --> pauli)"
            )
        p = float(a_parsed[0])
        state = try_as_integer(a_parsed[1])
        if not isinstance(state, int) or state <= 1:
            raise ValueError(f"State argument must be a leakage state >=2, got {state}")
        pauli = a_parsed[2]
        errors.append((p, state, cast(Literal["X", "Y", "Z"], pauli)))
    return LeakageControlledErrorParams(args=tuple(errors), from_tag=tag)


def _parse_leakage_transition_1_or_z(
    args_tuples: list[tuple[str, ...]], tag: str
) -> LeakageTransition1Params | LeakageTransitionZParams:
    name = tag.split(":")[0]
    transitions = []
    for a in args_tuples:
        if len(a) != 2:
            raise ValueError(
                f"Malformed parsed argument {a} in {tag}. "
                f"Arguments should be comma separated and in the form "
                "probability, leakage_transitions"
            )
        p = float(a[0])
        this_transition = a[1]
        match = LEAKAGE_TRANSITIONS_1_MATCH.fullmatch(this_transition)
        if not match:  # the tag failed the regex:
            raise ValueError(
                f"Malformed {name} argument '{a[1]}' in tag '{tag}'. "
                f"The transitions arguments should match the form "
                f"'int_first_state{{-->,<->}}int_second_state'"
            )
        s0 = try_as_integer(match.group("s0"))
        s1 = try_as_integer(match.group("s1"))

        transitions.append((p, s0, s1))

        if match.group("direction") == "<->":
            transitions.append((p, s1, s0))

    if tag.startswith("LEAKAGE_TRANSITION_1"):
        return LeakageTransition1Params(
            args=cast(
                tuple[tuple[float, int | Literal["U"], int | Literal["U"]]],
                tuple(transitions),
            ),
            from_tag=tag,
        )
    else:
        return LeakageTransitionZParams(
            args=cast(tuple[tuple[float, int, int]], tuple(transitions)), from_tag=tag
        )


def _parse_leakage_transition_2(
    args_tuples: list[tuple[str, ...]], tag: str
) -> LeakageTransition2Params:
    transitions = []
    for a in args_tuples:
        if len(a) != 2:
            raise ValueError(
                f"Malformed parsed argument {a} in {tag}. "
                f"Arguments should be comma separated and in the form "
                "probability:leakage_transitions"
            )
        p = float(a[0])
        this_transition = a[1]
        match = LEAKAGE_TRANSITIONS_2_MATCH.fullmatch(this_transition)
        if not match:  # the tag failed the regex:
            raise ValueError(
                f"Malformed LEAKAGE_TRANSITIONS_2 transitions argument '{a}' in tag '{tag}'. "
                f"The transitions arguments should match the form "
                f"'state_state{{-->,<->}}state_state'"
            )

        s0 = try_as_integer(match.group("s0"))
        s1 = try_as_integer(match.group("s1"))
        s2 = try_as_integer(match.group("s2"))
        s3 = try_as_integer(match.group("s3"))

        transitions.append((p, (s0, s1), (s2, s3)))

        if match.group("direction") == "<->":
            transitions.append((p, (s2, s3), (s0, s1)))

    return LeakageTransition2Params(
        args=cast(
            tuple[
                tuple[
                    float,
                    tuple[int | Literal["U"], int | Literal["U"]],
                    tuple[
                        Literal["U", "V", "X", "Y", "Z", "D"] | int,
                        Literal["U", "V", "X", "Y", "Z", "D"] | int,
                    ],
                ]
            ],
            tuple(transitions),
        ),
        from_tag=tag,
    )

def _parse_leakage_projection_z(
    args_tuples: list[tuple[str, ...]], tag: str
) -> LeakageMeasurementParams:
    projections = []
    for a in args_tuples:
        if len(a) != 2:
            raise ValueError(
                f"Malformed parsed argument '{a}' in tag '{tag}'. "
                f"Arguments should be comma separated and in the form "
                "'(probability, state)'"
            )
        p = float(a[0])
        state = int(a[1])
        projections.append((p, state))
    return LeakageMeasurementParams(args=tuple(projections), targets=None, from_tag=tag)

def _parse_list_of_targets(targets: str) -> list[int]:
    """Parse a space-separated list of qubit targets."""
    targets_set = []
    for target in targets.split():
        if target.isdigit():
            targets_set += [int(target)]
        else:
            raise ValueError(
                "Targets in tag are not integers separated" f" by spaces: {targets}"
            )
    return targets_set

def _parse_leakage_measurement(tag: str) -> LeakageMeasurementParams:
    """
    Parse a LEAKAGE_MEASUREMENT tag.
    For example:
    "MPAD[LEAKAGE_MEASUREMENT: (0.1, 0) (0.2, 1) (0.9, 2) (0.99, 3) : 1 3 5 7]"
    """
    match = MEASURE_TAG_MATCH.fullmatch(tag)
    if match is None:
        raise ValueError(
            "To use the leakage measurement tag, use the following syntax:"
            '"LEAKAGE_MEASUREMENT: (prob, state), ... : target, ...".'
        )

    args = match.group("args")
    target_args = match.group("targets")

    projections: list[tuple[float, int]] = []
    if args:
        stripped_args = args.strip()
        if not stripped_args:
            raise ValueError(f"Empty arguments in tag '{tag}'")
        if not (stripped_args.startswith("(") and stripped_args.endswith(")")):
            raise ValueError(
                f"Arguments must be enclosed in parentheses in tag '{tag}'"
            )
        # Strip outer parens and split by ') ('
        # re.split(r"\)\s*,*\s*\(", stripped_args[1:-1])
        args_list = stripped_args[1:-1].split(") (")
        args_tuples = [tuple(b.strip() for b in a.split(",")) for a in args_list]

        for a in args_tuples:
            if len(a) != 2:
                raise ValueError(
                    f"Malformed parsed argument '{a}' in tag '{tag}'. "
                    f"Arguments should be comma separated and in the form "
                    "'(probability, state)'"
                )
            p = float(a[0])
            state = try_as_integer(a[1])
            if isinstance(state, str):
                raise ValueError(
                    f"State {state} in tag '{tag}' is not a valid integer."
                )
            else:
                projections.append((p, state))

    targets = _parse_list_of_targets(target_args)
    return LeakageMeasurementParams(
        args=tuple(projections), targets=tuple(targets), from_tag=tag
    )

TAG_PARSERS = {
    "LEAKAGE_CONTROLLED_ERROR": _parse_leakage_controlled_error,
    "LEAKAGE_TRANSITION_1": _parse_leakage_transition_1_or_z,
    "LEAKAGE_TRANSITION_Z": _parse_leakage_transition_1_or_z,
    "LEAKAGE_TRANSITION_2": _parse_leakage_transition_2,
    "LEAKAGE_PROJECTION_Z": _parse_leakage_projection_z,
}


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
    return _parse_leakage_in_circuit_recurse(circuit=circuit)


def _parse_leakage_in_circuit_recurse(
    circuit: stim.Circuit,
) -> dict[stim.CircuitInstruction, LeakageParams]:
    parsed_tags = {}
    for op in circuit:
        if type(op) == stim.CircuitRepeatBlock:
            parsed_tags.update(_parse_leakage_in_circuit_recurse(op.body_copy()))
        elif op.tag != "":
            parsed = parse_leakage_tag(op)
            if parsed is not None:
                parsed_tags[op] = parsed

    return parsed_tags


def try_as_integer(state: str) -> int | str:
    if state.isdigit():
        return int(state)
    return state
