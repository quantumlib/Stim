from __future__ import annotations

import dataclasses
from typing import Literal

import stim

from stimflow._layers._layer import Layer
from stimflow._layers._layer_rotation import LayerRotation


@dataclasses.dataclass
class LayerReset(Layer):
    """A layer of reset gates."""

    targets: dict[int, Literal["X", "Y", "Z"]] = dataclasses.field(default_factory=dict)

    def copy(self) -> LayerReset:
        return LayerReset(targets=dict(self.targets))

    def touched(self) -> set[int]:
        return set(self.targets.keys())

    def to_z_basis(self) -> list[Layer]:
        return [
            LayerReset(targets={q: "Z" for q in self.targets.keys()}),
            LayerRotation(
                {
                    q: "I" if b == "Z" else "H" if b == "X" else "H_YZ"
                    for q, b in self.targets.items()
                }
            ),
        ]

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        basis: Literal["X", "Y", "Z"]
        outs: dict[Literal["X", "Y", "Z"], list[int]] = {"X": [], "Y": [], "Z": []}
        for target, basis in self.targets.items():
            outs[basis].append(target)
        for basis, vs in outs.items():
            if vs:
                out.append("R" + basis, vs)

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        if isinstance(next_layer, LayerReset):
            return [
                LayerReset(
                    targets={t: b for layer in [self, next_layer] for t, b in layer.targets.items()}
                )
            ]
        return [self, next_layer]
