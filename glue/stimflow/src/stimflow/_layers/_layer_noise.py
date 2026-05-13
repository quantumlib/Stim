from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer import Layer


@dataclasses.dataclass
class LayerNoise(Layer):
    """A layer of noise operations."""

    circuit: stim.Circuit = dataclasses.field(default_factory=stim.Circuit)

    def copy(self) -> LayerNoise:
        return LayerNoise(circuit=self.circuit.copy())

    def touched(self) -> set[int]:
        return {
            target.qubit_value
            for instruction in self.circuit
            for target in instruction.targets_copy()
        }

    def requires_tick_before(self) -> bool:
        return False

    def implies_eventual_tick_after(self) -> bool:
        return False

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        out += self.circuit
