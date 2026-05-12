from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer import Layer


@dataclasses.dataclass
class LayerTag(Layer):
    circuit: stim.Circuit = dataclasses.field(default_factory=stim.Circuit)

    def copy(self) -> LayerTag:
        return LayerTag(circuit=self.circuit)

    def touched(self) -> set[int]:  # set of qubit touched by it
        tagged_gate_targets = self.circuit[0].target_groups()[0]
        return {gate_target.qubit_value for gate_target in tagged_gate_targets}

    def to_z_basis(self) -> list[Layer]:
        return [self]

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        return [self, next_layer]

    def is_vacuous(self) -> bool:
        return False

    def requires_tick_before(self) -> bool:
        return True

    def implies_eventual_tick_after(self) -> bool:
        return True

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        out += self.circuit
