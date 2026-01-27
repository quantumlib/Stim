import dataclasses
from typing import ClassVar

import numpy as np

from stimside.util.alias_table import build_alias_table, sample_from_alias_table


@dataclasses.dataclass(frozen=True)
class LeakageTransitionZParams:

    name: ClassVar[str] = "LEAKAGE_TRANSITION_Z"
    args: tuple[tuple[float, int, int], ...]
    from_tag: str

    # COMPUTED ATTRIBUTES
    args_by_input_state: dict[int, tuple[tuple[int, float]]] = dataclasses.field(
        init=False, default_factory=dict
    )
    alias_table_by_input_state: dict[int, tuple[np.ndarray, np.ndarray, np.ndarray]] = (
        dataclasses.field(init=False, default_factory=dict)
    )

    def __post_init__(self):
        object.__setattr__(
            self, "args_by_input_state", self._build_args_by_input_state()
        )
        object.__setattr__(
            self, "alias_table_by_input_state", self._build_alias_table_by_input_state()
        )
        self._validate()

    def __eq__(self, other):
        if not isinstance(other, LeakageTransitionZParams):
            return False
        return (self.args == other.args) and (self.from_tag == other.from_tag)

    def __repr__(self):
        return f"LeakageTransitionZParams(args={self.args}, from_tag={self.from_tag})"

    def _validate(self):
        for s0, transitions in self.args_by_input_state.items():
            total_p = sum(p for s1, p in transitions)
            if total_p > 1 and not np.isclose(total_p, 1):
                raise ValueError(
                    f"{self.name} total probability {total_p}>1 for input state {s0}"
                )

        for p, s0, s1 in self.args:
            if not isinstance(s0, int):
                raise ValueError(f"{self.name} has input state {s0}, must be an int")
            if not isinstance(s1, int):
                raise ValueError(f"{self.name} has input state {s1}, must be an int")
            if p < 0 or p > 1:
                raise ValueError(
                    f"{self.name} has probability argument {p} outside [0,1]"
                )
            if s0 == s1:
                raise ValueError(f"{self.name} has identity transition {s0}->{s1}")
            if s0 in ["V"]:
                raise ValueError(f"{self.name} has invalid input state {s0}->{s1}")
            if s1 in ["V"]:
                raise ValueError(f"{self.name} has invalid output state {s0}->{s1}")

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
        self, input_state: int, num_samples: int, np_rng: np.random.Generator
    ) -> np.ndarray:
        """for a given input state, return num_sample output states as determined by args."""
        bases, aliases, thresholds = self.alias_table_by_input_state[input_state]
        return sample_from_alias_table(
            num_samples=num_samples,
            bases=bases,
            aliases=aliases,
            thresholds=thresholds,
            np_rng=np_rng,
        )
