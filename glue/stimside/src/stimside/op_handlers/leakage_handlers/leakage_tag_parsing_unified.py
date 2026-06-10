import stim  # type: ignore[import-untyped]
from typing import Literal, Any, cast

from stimside.op_handlers.leakage_handlers.common_parsing import (
    LEAKAGE_TRANSITIONS_1_MATCH,
    LEAKAGE_TRANSITIONS_2_MATCH,
    try_as_integer,
    split_arguments,
    parse_list_of_targets,
)

from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageTransitionZParams,
    LeakageConditioningParams,
    LeakageMeasurementParams,
    LeakageControlledErrorParams,
)

# --- PARSER HELPERS ---

def _combine_conditioned_states(states: list[int | str]) -> list[int | str]:
    if "U" in states:
        new_states: list[int | str] = ["U"]
        for st in states:
            if isinstance(st, int):
                if 1 < st < 10: new_states.append(st)
                elif st < 0 or st > 9: raise ValueError(f"State {st} out of range 0-9.")
            elif st != "U": raise ValueError("Invalid state element.")
        return new_states
    else:
        new_states, qubit_states = [], []
        for st in states:
            if 1 < st < 10: new_states.append(st)
            elif st in [0, 1]: qubit_states.append(st)
            else: raise ValueError(f"State {st} out of range 0-9.")
        if 0 in qubit_states and 1 in qubit_states: new_states.append("U")
        else: new_states += qubit_states
        return new_states

def _parse_list_of_states(args: str) -> set[str | int]:
    states = [try_as_integer(st.strip()) for st in args.split()]
    return set(_combine_conditioned_states(cast(list[int | str], states)))

# --- TAG-SPECIFIC PARSER FUNCTIONS ---

def _parse_controlled_error(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageControlledErrorParams:
    args_tuples = split_arguments(match.group("args"))
    errors = []
    for a in args_tuples:
        if len(a) != 2: raise ValueError(f"Malformed argument '{a}' in tag '{op.tag}'.")
        a_parsed = [a[0]] + [s.strip() for s in a[1].split("-->")]
        if len(a_parsed) != 3: raise ValueError(f"Malformed transition '{a_parsed}' in {op.tag}.")
        p, state, pauli = float(a_parsed[0]), try_as_integer(a_parsed[1]), a_parsed[2]
        if not isinstance(state, int) or state <= 1: raise ValueError(f"State must be >= 2, got {state}")
        errors.append((p, state, cast(Literal["X", "Y", "Z"], pauli)))
    return LeakageControlledErrorParams(args=tuple(errors), from_tag=op.tag)

def _parse_transition_1(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageTransition1Params:
    args_tuples = split_arguments(match.group("args"))
    transitions = []
    for a in args_tuples:
        p = float(a[0])
        m = LEAKAGE_TRANSITIONS_1_MATCH.fullmatch(a[1])
        if not m: raise ValueError(f"Malformed transition '{a[1]}' in tag '{op.tag}'.")
        s0, s1 = try_as_integer(m.group("s0")), try_as_integer(m.group("s1"))
        transitions.append((p, s0, s1))
        if m.group("direction") == "<->": transitions.append((p, s1, s0))
    return LeakageTransition1Params(args=cast(Any, tuple(transitions)), from_tag=op.tag)

def _parse_transition_z(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageTransitionZParams:
    # Use transition 1 logic but return Z params
    res = _parse_transition_1(op, match, sim)
    return LeakageTransitionZParams(args=cast(Any, res.args), from_tag=op.tag)

def _parse_transition_2(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageTransition2Params:
    args_tuples = split_arguments(match.group("args"))
    transitions = []
    for a in args_tuples:
        p = float(a[0])
        m = LEAKAGE_TRANSITIONS_2_MATCH.fullmatch(a[1])
        if not m: raise ValueError(f"Malformed transition '{a[1]}' in tag '{op.tag}'.")
        s0, s1, s2, s3 = (try_as_integer(m.group(g)) for g in ["s0", "s1", "s2", "s3"])
        transitions.append((p, (s0, s1), (s2, s3)))
        if m.group("direction") == "<->": transitions.append((p, (s2, s3), (s0, s1)))
    return LeakageTransition2Params(args=cast(Any, tuple(transitions)), from_tag=op.tag)

def _parse_projection_z(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageMeasurementParams:
    args_tuples = split_arguments(match.group("args"))
    projections = [(float(a[0]), int(a[1])) for a in args_tuples]
    return LeakageMeasurementParams(args=tuple(projections), targets=None, from_tag=op.tag)

def _parse_measurement(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageMeasurementParams:
    args, target_args = match.group("args"), match.group("targets")
    projections = []
    if args:
        for a in split_arguments(args):
            p, state = float(a[0]), try_as_integer(a[1])
            if isinstance(state, str): raise ValueError(f"State {state} is not an integer.")
            projections.append((p, state))
    return LeakageMeasurementParams(
        args=tuple(projections), 
        targets=tuple(parse_list_of_targets(target_args)), 
        from_tag=op.tag
    )

def _parse_conditioned_self(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageConditioningParams:
    states = _parse_list_of_states(match.group("args"))
    return LeakageConditioningParams(args=(tuple(states),), from_tag=op.tag, targets=None)

def _parse_conditioned_other(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageConditioningParams:
    args, targets = match.group("args"), match.group("targets")
    if not targets: raise ValueError("CONDITIONED_ON_OTHER tag needs to specify targets.")
    states = _parse_list_of_states(args)
    return LeakageConditioningParams(
        args=(tuple(states),), 
        from_tag=op.tag, 
        targets=tuple(parse_list_of_targets(targets))
    )

def _parse_conditioned_pair(op: stim.CircuitInstruction, match: Any, sim: str) -> LeakageConditioningParams:
    states: list[list[int | str]] = [[], []]
    for pair in split_arguments(match.group("args")):
        if len(pair) != 2: raise ValueError(f"CONDITIONED_ON_PAIR must have pairs of states.")
        converted = [try_as_integer(s) for s in pair]
        for s in converted:
            if isinstance(s, str) and s != "U": raise ValueError(f"Invalid state {s}.")
        states[0].append(converted[0])
        states[1].append(converted[1])
    return LeakageConditioningParams(args=(tuple(states[0]), tuple(states[1])), from_tag=op.tag, targets=None)


