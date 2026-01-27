from typing import cast, Literal
import re

import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageConditioningParams,
    LeakageMeasurementParams,
)

CONDITION_TAG_MATCH = re.compile(
    r"(?P<name>CONDITIONED_ON_[A-Z]+)\s*:(?P<args>[^:]+):*(?P<targets>.+)*"
)
## Conditional tag. Specififies if qubit is in certain states, do op otherwise do nothing
## Examples:
##  "CONDITIONED_ON_SELF: U 2 3" : 1 qubit op
##  DEPOLARIZE1[CONDITIONED_ON_OTHER: U 2 3: 10 12 14 16](0.1) 9 11 13 15 : 1 qubit op
##  "CONDITIONED_ON_PAIR: (2, 3) (U, 2) (U, 3)" : 2 qubit op
## Can condition on U, L, or specific state

MEASURE_TAG_MATCH = re.compile(r"LEAKAGE_MEASUREMENT\s*:(?P<args>.+):(?P<targets>.+)")
## "MPAD[LEAKAGE_MEASUREMENT: (0.1, 0) (0.2, 1) (0.9, 2) (0.99, 3) : 1 3 5 7]"

LEAKAGE_TAG_MATCH = re.compile(r"(?P<name>LEAKAGE_\w+):(?P<args>.+)")
# you can use this by doing
# match = LEAKAGE_TAG_MATCH.fullmatch(tag)
# the result is None if the tag isn't formatted correctly
# otherwise the result has named groups:
#   match.group('name')
#   match.group('args')

LEAKAGE_TRANSITIONS_1_MATCH = re.compile(
    r"(?P<s0>[\dU])(?P<direction>[-<]->)(?P<s1>[\dU])"
)
LEAKAGE_TRANSITIONS_2_MATCH = re.compile(
    r"(?P<s0>[\dU])_(?P<s1>[\dU])(?P<direction>[-<]->)(?P<s2>[\dUVDXYZ])_(?P<s3>[\dUVDXYZ])"
)


def _parse_leakage_transition_1(
    args_tuples: list[tuple[str, ...]], tag: str
) -> LeakageTransition1Params:
    """Parse a LEAKAGE_TRANSITION_1"""
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
                f"'first_state{{-->,<->}}second_state'"
            )
        s0 = try_as_integer(match.group("s0"))
        s1 = try_as_integer(match.group("s1"))

        transitions.append((p, s0, s1))

        if match.group("direction") == "<->":
            transitions.append((p, s1, s0))

    return LeakageTransition1Params(
        args=cast(
            tuple[tuple[float, int | Literal["U"], int | Literal["U"]]],
            tuple(transitions),
        ),
        from_tag=tag,
    )


def _parse_leakage_transition_2(
    args_tuples: list[tuple[str, ...]], tag: str
) -> LeakageTransition2Params:
    """Parse a LEAKAGE_TRANSITION_2 tag."""
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
    """Parse a LEAKAGE_PROJECTION_Z tag."""
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

def _combine_conditioned_states(states: list[int | str]) -> list[int | str]:
    """
    Combine a list of conditioned states into a minimal representation.
    For example, [0, 1, 2, 'U'] -> ['U', 2]
    or [0, 1, 2, 2] -> [2, 'U']
    Only valid input states are integers 0-9 and 'U'.
    """
    if "U" in states:
        new_states: list[int | str] = ["U"]
        for st in states:
            if isinstance(st, int):
                if st > 1 and st < 10:
                    new_states.append(st)
                elif st < 0 or st > 9:
                    raise ValueError(
                        f"Qubit state {st} can not be negative or larger than 9."
                    )
            elif st != "U":
                raise ValueError(
                    "The conditioned state list in _combine_conditioned_states has an element"
                    "that is not U or an integer."
                )
        return new_states
    else:
        new_states = []
        qubit_states = []
        for st in states:
            assert isinstance(st, int), (
                "The conditioned state list in _combine_conditioned_states has an element"
                + "that is not U or an integer."
            )
            if st > 1 and st < 10:
                new_states.append(st)
            elif st in [0, 1]:
                qubit_states.append(st)
            else:
                raise ValueError(
                    f"Qubit state {st} can not be negative or larger than 9."
                )
        if 0 in qubit_states and 1 in qubit_states:
            new_states.append("U")
        else:
            new_states += qubit_states

        return new_states


def _parse_list_of_states(args: str) -> set[str | int]:
    """Parse a space-separated list of states, combining them appropriately."""
    states = []
    for st in args.split():
        states.append(try_as_integer(st.strip()))
    return set(_combine_conditioned_states(states))


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


def _parse_conditioned_on_self(args: str, tag: str) -> LeakageConditioningParams:
    """Parse a CONDITIONED_ON_SELF tag."""
    states = _parse_list_of_states(args)
    return LeakageConditioningParams(args=(tuple(states),), from_tag=tag, targets=None)


def _parse_conditioned_on_other(
    args: str, targets: str, tag: str
) -> LeakageConditioningParams:
    """Parse a CONDITIONED_ON_OTHER tag."""
    states = _parse_list_of_states(args)

    target_list = _parse_list_of_targets(targets)
    return LeakageConditioningParams(
        args=(tuple(states),), from_tag=tag, targets=tuple(target_list)
    )


def _parse_conditioned_on_pair(args: str, tag: str) -> LeakageConditioningParams:
    """Parse a CONDITIONED_ON_PAIR tag."""
    states: list[list[int | str]] = [[], []]
    for arg in (args.strip())[1:-1].split(") ("):
        state_pair = arg.strip().split(",")
        if len(state_pair) != 2:
            raise ValueError(
                f"The argument in a CONDITIONED_ON_PAIR tag {tag} does not have exactly"
                "two qubit states prescribed in a paranthesis."
            )
        state_pair_converted: list[int | str] = [
            try_as_integer(state) for state in state_pair
        ]
        for state in state_pair_converted:
            if isinstance(state, str) and state not in ["U"]:
                raise ValueError(
                    f"In tag {tag}, the state {state} conditioned on is not valid."
                    "It can only be an integer,"
                    "or the letter U as an unleaked state."
                )
        states[0].append(state_pair_converted[0])
        states[1].append(state_pair_converted[1])

    return LeakageConditioningParams(
        args=(tuple(states[0]), tuple(states[1])), from_tag=tag, targets=None
    )

LEAKAGE_TAG_PARSERS = {
    "LEAKAGE_TRANSITION_1": _parse_leakage_transition_1,
    "LEAKAGE_TRANSITION_2": _parse_leakage_transition_2,
    "LEAKAGE_PROJECTION_Z": _parse_leakage_projection_z,
}

CONDITION_TAG_PARSERS = [
    "CONDITIONED_ON_SELF",
    "CONDITIONED_ON_OTHER",
    "CONDITIONED_ON_PAIR",
]

ADDITIONAL_TAGS = [
    "LEAKAGE_MEASUREMENT",
]


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

    if tag.startswith("CONDITIONED_ON"):
        match = CONDITION_TAG_MATCH.fullmatch(tag)
        if match is None:
            raise ValueError(
                f"Malformed CONDITIONED_ON tag {tag}. "
                "If a tag begins with CONDITIONED_ON, we demand it match one of the patterns "
                '"CONDITIONED_ON_SELF: state ..." or '
                '"CONDITIONED_ON_OTHER: state ... : target ..." or '
                '"CONDITIONED_ON_PAIR: (p, state) ..."'
            )
        name = match.group("name")

        if (
            name in ["CONDITIONED_ON_SELF", "CONDITIONED_ON_OTHER"]
            and not gd.is_single_qubit_gate
        ):
            raise ValueError(
                f"1Q condition tag {op.tag} attached to not-1Q stim gate {op.name}. "
            )
        elif name in ["CONDITIONED_ON_PAIR"] and not gd.is_two_qubit_gate:
            raise ValueError(
                f"2Q condition tag {op.tag} attached to not-2Q stim gate {op.name}. "
            )

        if name in ["CONDITIONED_ON_SELF"]:
            args = match.group("args")
            return _parse_conditioned_on_self(args, tag)
        elif name in ["CONDITIONED_ON_PAIR"]:
            args = match.group("args")
            return _parse_conditioned_on_pair(args, tag)
        elif name in ["CONDITIONED_ON_OTHER"]:
            args = match.group("args")
            targets = match.group("targets")
            if targets:
                return _parse_conditioned_on_other(args, targets, tag)
            else:
                raise ValueError(
                    "CONDITIONED_ON_OTHER tag needs to specify targets. Please follow the pattern"
                    '"CONDITIONED_ON_OTHER: state ... : target ..."'
                )
        else:
            raise ValueError(
                f"CONDITIONED_ON tags can only be one of {CONDITION_TAG_PARSERS}"
            )

    # if it doesn't start with LEAKAGE, given up
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
        args_list = re.split(r"\)\s*,*\s*\(", stripped_args[1:-1])  # .split(") (")
        args_tuples = [tuple(b.strip() for b in a.split(",")) for a in args_list]

    # Check tag is attached to a reasonable gate
    # first check qubit-arity
    if (
        name in ["LEAKAGE_TRANSITION_1", "LEAKAGE_PROJECTION_Z"]
        and not gd.is_single_qubit_gate
    ):
        raise ValueError(
            f"1Q leakage tag {op.tag} attached to not-1Q stim gate {op.name}. "
        )
    if (
        name in ["LEAKAGE_TRANSITION_2"]
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
        raise ValueError(
            f"LEAKAGE_TRANSITION_Z is not used in the tableau simulator."
            " Use LEAKAGE_TRANSITION_1 instead."
        )
    elif name == "LEAKAGE_CONTROLLED_ERROR":
        raise ValueError(
            f"LEAKAGE_CONTROLLED_ERROR is not used in the tableau simulator."
            " Use CONDITIONED_ON_* instead."
        )
    elif op.name not in ["I", "I_ERROR", "II", "II_ERROR"]:
        raise ValueError(
            f"Leakage tag {name} must be attached to a trivially acting gate "
            f"(I, I_ERROR, II, II_ERROR)"
        )

    if name in LEAKAGE_TAG_PARSERS:
        return LEAKAGE_TAG_PARSERS[name](args_tuples, tag)

    if name in LEAKAGE_TAG_PARSERS:
        raise ValueError(
            f"Failed to recognise existing leakage tag name {name}. "
            "This one is on us, not you. File a bug."
        )
    raise ValueError(
        f"Unrecognised LEAKAGE tag name {name}: "
        f"must be one of {
            list(LEAKAGE_TAG_PARSERS.keys()) + 
            ADDITIONAL_TAGS
            }"
    )


def parse_leakage_in_circuit(
    circuit: stim.Circuit,
) -> dict[stim.CircuitInstruction, LeakageParams]:
    """Parse all present leakage tags in a circuit, including inside repeats."""
    return _parse_leakage_in_circuit_recurse(circuit=circuit)


def _parse_leakage_in_circuit_recurse(
    circuit: stim.Circuit,
) -> dict[stim.CircuitInstruction, LeakageParams]:
    """Helper function to parse all present leakage tags in a circuit, including inside repeats."""
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
    """Try to parse a state as an integer, otherwise return the stripped string."""
    if state.isdigit():
        return int(state)
    return state.strip()
