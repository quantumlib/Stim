from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer import Layer
from stimflow._layers._rotation_layer import RotationLayer


@dataclasses.dataclass
class MeasureLayer(Layer):
    """A layer of single qubit Pauli basis measurement operations."""

    targets: list[int] = dataclasses.field(default_factory=list)
    bases: list[str] = dataclasses.field(default_factory=list)

    def copy(self) -> MeasureLayer:
        return MeasureLayer(targets=list(self.targets), bases=list(self.bases))

    def touched(self) -> set[int]:
        return set(self.targets)

    def to_z_basis(self) -> list[Layer]:
        rot = RotationLayer(
            {
                q: "I" if b == "Z" else "H" if b == "X" else "H_YZ"
                for q, b in zip(self.targets, self.bases)
            }
        )
        return [
            rot,
            MeasureLayer(targets=list(self.targets), bases=["Z"] * len(self.targets)),
            rot.copy(),
        ]

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        for t, b in zip(self.targets, self.bases):
            out.append("M" + b, [t])

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        if isinstance(next_layer, MeasureLayer) and set(self.targets).isdisjoint(
            next_layer.targets
        ):
            return [
                MeasureLayer(
                    targets=self.targets + next_layer.targets, bases=self.bases + next_layer.bases
                )
            ]
        if isinstance(next_layer, RotationLayer) and set(self.targets).isdisjoint(
            next_layer.named_rotations.keys()
        ):
            return [next_layer, self]
        return [self, next_layer]
