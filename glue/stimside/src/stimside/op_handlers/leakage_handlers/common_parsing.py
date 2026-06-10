import re
from typing import Literal

# --- REGEXES ---

# Matches "LEAKAGE_NAME: (arg1) (arg2)"
LEAKAGE_TAG_MATCH = re.compile(r"(?P<name>LEAKAGE_\w+):(?P<args>.+)")

# Matches "CONDITIONED_ON_NAME: args : targets"
CONDITION_TAG_MATCH = re.compile(
    r"(?P<name>CONDITIONED_ON_[A-Z]+)\s*:(?P<args>[^:]+):*(?P<targets>.+)*"
)

# Matches "LEAKAGE_MEASUREMENT: (prob, state), ... : target, ..."
MEASURE_TAG_MATCH = re.compile(r"LEAKAGE_MEASUREMENT\s*:(?P<args>.+):(?P<targets>.+)")

# Matches "state-->state" or "state<->state"
# Allows multi-digit states and 'U'
LEAKAGE_TRANSITIONS_1_MATCH = re.compile(
    r"(?P<s0>[\dU]+)(?P<direction>[-<]->)(?P<s1>[\dU]+)"
)

# Matches "state_state-->state_state"
# Allows states: digits, U, V, D, X, Y, Z
LEAKAGE_TRANSITIONS_2_MATCH = re.compile(
    r"(?P<s0>[\dU])_(?P<s1>[\dU])(?P<direction>[-<]->)(?P<s2>[\dUVDXYZ])_(?P<s3>[\dUVDXYZ])"
)


# --- UTILITIES ---

def try_as_integer(state: str) -> int | str:
    """Try to parse a state as an integer, otherwise return the stripped string."""
    s = state.strip()
    if s.isdigit():
        return int(s)
    return s


def split_arguments(args: str) -> list[tuple[str, ...]]:
    """
    Robustly split parenthesized arguments, e.g., "(0.1, 2) (0.2, 3)" -> [("0.1", "2"), ("0.2", "3")].
    Handles optional commas and varied whitespace between parentheses.
    """
    stripped_args = args.strip()
    if not stripped_args:
        return []
    
    if not (stripped_args.startswith("(") and stripped_args.endswith(")")):
        raise ValueError(f"Arguments must be enclosed in parentheses: '{args}'")
    
    # re.split(r"\)\s*,*\s*\(", ...) handles ") (", "), (", ") ,(", etc.
    args_list = re.split(r"\)\s*,*\s*\(", stripped_args[1:-1])
    return [tuple(b.strip() for b in a.split(",")) for a in args_list]


def parse_list_of_targets(targets: str) -> list[int]:
    """Parse a space-separated list of qubit targets."""
    targets_set = []
    for target in targets.split():
        if target.isdigit():
            targets_set.append(int(target))
        else:
            raise ValueError(
                f"Targets in tag must be integers separated by spaces, got: '{targets}'"
            )
    return targets_set
