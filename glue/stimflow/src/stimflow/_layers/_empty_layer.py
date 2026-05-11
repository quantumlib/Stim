from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer import Layer


@dataclasses.dataclass
class EmptyLayer(Layer):
    def copy(self) -> EmptyLayer:
        return EmptyLayer()

    def touched(self) -> set[int]:
        return set()

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        pass

    def locally_optimized(self, next_layer: None | Layer) -> list[Layer | None]:
        return [next_layer]

    def is_vacuous(self) -> bool:
        return True
