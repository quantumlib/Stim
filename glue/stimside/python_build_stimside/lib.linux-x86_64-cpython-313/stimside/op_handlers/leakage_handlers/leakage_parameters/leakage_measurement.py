import dataclasses
from typing import ClassVar

import numpy as np


@dataclasses.dataclass(frozen=True)
class LeakageMeasurementParams:
    name: ClassVar[str] = "LEAKAGE_MEASUREMENT"
    args: tuple[tuple[float, int], ...]
    targets: tuple[int, ...] | None
    from_tag: str

    # COMPUTED ATTRIBUTES
    prob_for_input_state: dict[int, float] = dataclasses.field(init=False, default_factory=dict)

    def __post_init__(self):
        object.__setattr__(
            self, "prob_for_input_state", self._build_prob_for_input_state()
        )
        self._validate()

    def __eq__(self, other):
        if not isinstance(other, LeakageMeasurementParams):
            return False
        return (
            (self.args == other.args)
            and (self.from_tag == other.from_tag)
            and (self.targets == other.targets)
        )
    
    def __repr__(self):
        return (
            f"LeakageMeasurementParams(args={self.args}," 
            f" targets={self.targets}, from_tag={self.from_tag})"
        )

    def _validate(self):
        seen_states = set()
        for p, state in self.args:
            if not 0 <= p <= 1:
                raise ValueError(
                    f"{self.name} has probability argument {p} outside [0,1]"
                )
            if state < 0 or state > 9 or isinstance(state, str):
                raise ValueError(
                    f"{self.name} has invalid state {state}, must be an int between 0 and 9"
                )
            if state in seen_states:
                raise ValueError(f"{self.name} has repeated state {state}")
            seen_states.add(state)

        if self.targets:
            seen_qubits = set()
            for qubit in self.targets:
                if qubit < 0:
                    raise ValueError(f"{self.name} has target qubit index {qubit} < 0")
                if qubit in seen_qubits:
                    raise ValueError(f"{self.name} has repeated qubit target {qubit}")
                seen_qubits.add(qubit)

    def _build_prob_for_input_state(self):
        prob_for_input_state = {}
        for prob, state in self.args:
            prob_for_input_state[state] = prob
        return prob_for_input_state

    def sample_projections(
        self, state: int, num_samples: int, np_rng: np.random.Generator
    ):
        """given a state, return samples for readout values with the appropriate probability."""
        if state not in self.prob_for_input_state:
            raise ValueError(f"Can't sample projections for unspecified state {state}.")
        return np_rng.random(num_samples) < self.prob_for_input_state[state]
