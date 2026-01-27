import dataclasses
from typing import ClassVar, Literal

import numpy as np

Pauli = Literal["X", "Y", "Z"]
ControlledErrorArg = tuple[float, int, Pauli]


@dataclasses.dataclass(frozen=True)
class LeakageControlledErrorParams:

    name: ClassVar[str] = "LEAKAGE_CONTROLLED_ERROR"
    args: tuple[tuple[float, int, Literal["X", "Y", "Z"]], ...]
    from_tag: str

    # COMPUTED ATTRIBUTES
    arg_by_input_state: dict[int, ControlledErrorArg] = dataclasses.field(init=False)

    def __post_init__(self):
        object.__setattr__(self, "arg_by_input_state", self._build_arg_by_input_state())
        self._validate()

    def __eq__(self, other):
        if not isinstance(other, LeakageControlledErrorParams):
            return False
        return (self.args == other.args) and (self.from_tag == other.from_tag)

    def _validate(self):
        total_p = sum([a[0] for a in self.args])
        if total_p > 1 and not np.isclose(total_p, 1):
            raise ValueError(f"{self.name} total probability {total_p}>1")

        seen_states = set()
        for p, state, pauli in self.args:
            if p < 0 or p > 1:
                raise ValueError(
                    f"{self.name} has probability argument {p} outside [0,1]"
                )
            if not isinstance(state, int) or state <= 1:
                raise ValueError(
                    f"{self.name} state argument must be a leakage state >=2, got {state}"
                )
            if state in seen_states:
                raise ValueError(f"{self.name} has repeated state {state}")
            seen_states.add(state)

            if pauli not in ["X", "Y", "Z"]:
                raise ValueError(
                    f"{self.name} pauli was {pauli}, must be in ['X','Y','Z']"
                )

    def _build_arg_by_input_state(self):
        arg_for_input_state = {}
        for prob, state, pauli in self.args:
            arg_for_input_state[state] = (prob, state, pauli)
        return arg_for_input_state
