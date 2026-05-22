from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer import Layer
from stimflow._layers._layer_rotation import LayerRotation


@dataclasses.dataclass
class LayerMeasure(Layer):
    """A layer of single qubit Pauli basis measurement operations."""

    targets: list[int] = dataclasses.field(default_factory=list)
    bases: list[str] = dataclasses.field(default_factory=list)

    def copy(self) -> LayerMeasure:
        return LayerMeasure(targets=list(self.targets), bases=list(self.bases))

    def touched(self) -> set[int]:
        return set(self.targets)

    def to_z_basis(self) -> list[Layer]:
        rot = LayerRotation(
            {
                q: "I" if b == "Z" else "H" if b == "X" else "H_YZ"
                for q, b in zip(self.targets, self.bases)
            }
        )
        return [
            rot,
            LayerMeasure(targets=list(self.targets), bases=["Z"] * len(self.targets)),
            rot.copy(),
        ]

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        for t, b in zip(self.targets, self.bases):
            out.append("M" + b, [t])

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        if isinstance(next_layer, LayerMeasure) and set(self.targets).isdisjoint(
            next_layer.targets
        ):
            return [
                LayerMeasure(
                    targets=self.targets + next_layer.targets, bases=self.bases + next_layer.bases
                )
            ]
        if isinstance(next_layer, LayerRotation) and set(self.targets).isdisjoint(
            next_layer.named_rotations.keys()
        ):
            return [next_layer, self]
        return [self, next_layer]
