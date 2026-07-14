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
        if isinstance(self.circuit[0], stim.CircuitRepeatBlock):
            raise NotImplementedError(
                "No tags allowed on circuit repeat blocks, "
                "since I need target_groups for defining which qubits are touched"
            )
        touched_set = {
            target.qubit_value
            for target_group in self.circuit[0].target_groups()
            for target in target_group
            if target.is_qubit_target
        }
        return {q for q in touched_set if isinstance(q, int)}

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
