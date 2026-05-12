from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer import Layer


@dataclasses.dataclass
class DetObsAnnotationLayer(Layer):
    circuit: stim.Circuit = dataclasses.field(default_factory=stim.Circuit)

    def with_rec_targets_shifted_by(self, shift: int) -> DetObsAnnotationLayer:
        result = DetObsAnnotationLayer()
        for inst in self.circuit:
            result.circuit.append(
                inst.name,
                [stim.target_rec(t.value + shift) for t in inst.targets_copy()],
                inst.gate_args_copy(),
            )
        return result

    def copy(self) -> DetObsAnnotationLayer:
        return DetObsAnnotationLayer(circuit=self.circuit.copy())

    def touched(self) -> set[int]:
        return set()

    def requires_tick_before(self) -> bool:
        return False

    def implies_eventual_tick_after(self) -> bool:
        return False

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        out += self.circuit

    def locally_optimized(self, next_layer: None | Layer) -> list[Layer | None]:
        if isinstance(next_layer, DetObsAnnotationLayer):
            return [DetObsAnnotationLayer(self.circuit + next_layer.circuit)]
        return [self, next_layer]
