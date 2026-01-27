import dataclasses
from typing import ClassVar, Literal

import numpy as np

from stimside.util.alias_table import build_alias_table, sample_from_alias_table

InState = int | Literal["U"]
OutState = int | Literal["U", "V", "X", "Y", "Z", "D"]
InPairState = tuple[InState, InState]
OutPairState = tuple[OutState, OutState]
Transition2ArgInput = tuple[float, InPairState, OutPairState]
Transition2Arg = tuple[OutPairState, float]

OUT_ERROR_STATES = ["X", "Y", "Z", "D"]


@dataclasses.dataclass
class LeakageTransition2Params:
    name: ClassVar[str] = "LEAKAGE_TRANSITION_2"
    args: tuple[Transition2ArgInput, ...]
    from_tag: str

    # COMPUTED ATTRIBUTES
    args_by_input_state: dict[InPairState, tuple[Transition2Arg]] = dataclasses.field(
        init=False
    )
    alias_table_by_input_state: dict[
        InPairState, tuple[np.ndarray, np.ndarray, np.ndarray]
    ] = dataclasses.field(init=False)

    def __post_init__(self):
        object.__setattr__(
            self, "args_by_input_state", self._build_args_by_input_state()
        )
        object.__setattr__(
            self, "alias_table_by_input_state", self._build_alias_table_by_input_state()
        )
        self._validate()

    def __eq__(self, other):
        if not isinstance(other, LeakageTransition2Params):
            return False
        return (self.args == other.args) and (self.from_tag == other.from_tag)

    def __repr__(self):
        return f"LeakageTransition2Params(args={self.args}, from_tag={self.from_tag})"

    def _validate(self):
        for s0, transitions in self.args_by_input_state.items():
            total_p = sum(p for s1, p in transitions)
            if total_p > 1 and not np.isclose(total_p, 1):
                raise ValueError(
                    f"{self.name} total probability {total_p}>1 "
                    f"for input state {s0}"
                )

        for p, pair0, pair1 in self.args:
            if p < 0 or p > 1:
                raise ValueError(
                    f"{self.name} has probability argument {p} outside [0,1]"
                )
            if pair0 == pair1:
                raise ValueError(
                    f"{self.name} has identity transition {pair0}->{pair1}"
                )

            for state in pair0:
                if not (isinstance(state, str) and state in ["U"]):
                    if not (isinstance(state, int) and state >= 0):
                        raise ValueError(
                            f"{self.name} has invalid start state {pair0}->{pair1}"
                        )

            for state in pair1:
                if not (
                    isinstance(state, str) and state in OUT_ERROR_STATES + ["U", "V"]
                ):
                    if not (isinstance(state, int) and state >= 0):
                        raise ValueError(
                            f"{self.name} has invalid end state {pair0}->{pair1}"
                        )

            if (pair1[0] == "V" and pair0[0] != "U") or (
                pair1[1] == "V" and pair0[1] != "U"
            ):
                raise ValueError(
                    f"{self.name} has invalid transition to V not from U {pair0}->{pair1}"
                )

    def _build_args_by_input_state(self):
        args_by_input = {}
        for p, s0, s1 in self.args:
            if s0 not in args_by_input:
                args_by_input[s0] = []
            args_by_input[s0].append((s1, p))
        return {k: tuple(v) for k, v in args_by_input.items()}

    def _build_alias_table_by_input_state(self):
        alias_tables_by_input_state = {}
        for input_state, args in self.args_by_input_state.items():
            sum_probs = sum(a[1] for a in args)
            args_list = [*args, (input_state, 1 - sum_probs)]
            alias_table = build_alias_table(args_list)
            alias_tables_by_input_state[input_state] = alias_table
        return alias_tables_by_input_state

    def sample_transitions_from_state(
        self, input_state: InPairState, num_samples: int, np_rng: np.random.Generator
    ) -> np.ndarray:
        """for a given input pair state, return num_sample output pair states."""
        bases, aliases, thresholds = self.alias_table_by_input_state[input_state]
        return sample_from_alias_table(
            num_samples=num_samples,
            bases=bases,
            aliases=aliases,
            thresholds=thresholds,
            np_rng=np_rng,
        )
