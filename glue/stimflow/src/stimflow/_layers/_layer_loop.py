from __future__ import annotations

import dataclasses
from typing import TYPE_CHECKING

import stim

from stimflow._layers._layer import Layer

if TYPE_CHECKING:
    from stimflow._layers._layer_circuit import LayerCircuit


@dataclasses.dataclass
class LayerLoop(Layer):
    body: LayerCircuit
    repetitions: int

    def copy(self) -> LayerLoop:
        return LayerLoop(body=self.body.copy(), repetitions=self.repetitions)

    def touched(self) -> set[int]:
        return self.body.touched()

    def to_z_basis(self) -> list[Layer]:
        return [LayerLoop(body=self.body.to_z_basis(), repetitions=self.repetitions)]

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        optimized = LayerLoop(
            body=self.body.with_locally_optimized_layers(), repetitions=self.repetitions
        )
        return [optimized, next_layer]

    def implies_eventual_tick_after(self) -> bool:
        return False

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        body = self.body.to_stim_circuit()
        body.append("TICK")
        out.append(stim.CircuitRepeatBlock(repeat_count=self.repetitions, body=body))
