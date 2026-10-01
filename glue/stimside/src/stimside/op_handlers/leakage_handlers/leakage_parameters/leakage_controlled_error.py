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
    args_by_input_state: dict[int, tuple[tuple[Pauli, float], ...]] = dataclasses.field(
        init=False
    )

    def __post_init__(self):
        object.__setattr__(self, "arg_by_input_state", self._build_arg_by_input_state())
        object.__setattr__(
            self, "args_by_input_state", self._build_args_by_input_state()
        )
        self._validate()

    def __eq__(self, other):
        if not isinstance(other, LeakageControlledErrorParams):
            return False
        return (self.args == other.args) and (self.from_tag == other.from_tag)

    def _validate(self):
        seen_state_paulis: set[tuple[int, str]] = set()
        probs_by_state: dict[int, float] = {}
        for p, state, pauli in self.args:
            if p < 0 or p > 1:
                raise ValueError(
                    f"{self.name} has probability argument {p} outside [0,1]"
                )
            if not isinstance(state, int) or state <= 1:
                raise ValueError(
                    f"{self.name} state argument must be a leakage state >=2, got {state}"
                )
            if pauli not in ["X", "Y", "Z"]:
                raise ValueError(
                    f"{self.name} pauli was {pauli}, must be in ['X','Y','Z']"
                )
            if (state, pauli) in seen_state_paulis:
                raise ValueError(
                    f"{self.name} has repeated state {state} with pauli {pauli}"
                )
            seen_state_paulis.add((state, pauli))
            probs_by_state[state] = probs_by_state.get(state, 0.0) + p

        for state, total_p in probs_by_state.items():
            if total_p > 1 and not np.isclose(total_p, 1):
                raise ValueError(
                    f"{self.name} total probability {total_p}>1 for state {state}"
                )

    def _build_arg_by_input_state(self):
        arg_for_input_state = {}
        for prob, state, pauli in self.args:
            arg_for_input_state[state] = (prob, state, pauli)
        return arg_for_input_state

    def _build_args_by_input_state(self):
        args_for_input_state: dict[int, list[tuple[Pauli, float]]] = {}
        for prob, state, pauli in self.args:
            if state not in args_for_input_state:
                args_for_input_state[state] = []
            args_for_input_state[state].append((pauli, prob))
        return {k: tuple(v) for k, v in args_for_input_state.items()}

