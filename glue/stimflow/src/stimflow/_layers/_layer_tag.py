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
        def _touched(inst: stim.CircuitInstruction | stim.CircuitRepeatBlock) -> set[int]:
            if isinstance(inst, stim.CircuitRepeatBlock):
                return {q for inner_inst in inst.body_copy() for q in _touched(inner_inst)}
            touched_set = {
                target.qubit_value
                for target_group in inst.target_groups()
                for target in target_group
                if target.is_qubit_target
            }
            return {q for q in touched_set if isinstance(q, int)}

        return _touched(self.circuit[0])

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
