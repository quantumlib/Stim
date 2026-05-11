from __future__ import annotations

import collections
import dataclasses

import stim

from stimflow._layers._layer import Layer


@dataclasses.dataclass
class InteractLayer(Layer):
    """A layer of controlled Pauli gates (like CX, CZ, and XCY)."""

    targets1: list[int] = dataclasses.field(default_factory=list)
    targets2: list[int] = dataclasses.field(default_factory=list)
    bases1: list[str] = dataclasses.field(default_factory=list)
    bases2: list[str] = dataclasses.field(default_factory=list)

    def touched(self) -> set[int]:
        return set(self.targets1 + self.targets2)

    def copy(self) -> InteractLayer:
        return InteractLayer(
            targets1=list(self.targets1),
            targets2=list(self.targets2),
            bases1=list(self.bases1),
            bases2=list(self.bases2),
        )

    def rotate_to_z_layer(self):
        from stimflow._layers._rotation_layer import RotationLayer

        result = RotationLayer()
        for targets, bases in [(self.targets1, self.bases1), (self.targets2, self.bases2)]:
            for q, b in zip(targets, bases):
                if b == "X":
                    result.named_rotations[q] = "H"
                elif b == "Y":
                    result.named_rotations[q] = "H_YZ"
        return result

    def to_z_basis(self) -> list[Layer]:
        rot = self.rotate_to_z_layer()
        return [
            rot,
            InteractLayer(
                targets1=list(self.targets1),
                targets2=list(self.targets2),
                bases1=["Z"] * len(self.targets1),
                bases2=["Z"] * len(self.targets2),
            ),
            rot.copy(),
        ]

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        groups = collections.defaultdict(list)
        for k in range(len(self.targets1)):
            gate = self.bases1[k] + "C" + self.bases2[k]
            t1 = self.targets1[k]
            t2 = self.targets2[k]
            if gate in ["XCZ", "YCZ", "YCX"]:
                t1, t2 = t2, t1
                gate = gate[::-1]
            if gate in ["XCX", "YCY", "ZCZ"]:
                t1, t2 = sorted([t1, t2])
            groups[gate].append((t1, t2))
        for gate in sorted(groups.keys()):
            for pair in sorted(groups[gate]):
                out.append(gate, pair)

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        from stimflow._layers._interact_swap_layer import InteractSwapLayer
        from stimflow._layers._swap_layer import SwapLayer

        if isinstance(next_layer, SwapLayer):
            pairs1 = {frozenset([a, b]) for a, b in zip(self.targets1, self.targets2)}
            pairs2 = {frozenset([a, b]) for a, b in zip(next_layer.targets1, next_layer.targets2)}
            if pairs1 == pairs2:
                return [InteractSwapLayer(i_layer=self.copy())]
        elif isinstance(next_layer, InteractLayer) and self.touched().isdisjoint(
            next_layer.touched()
        ):
            return [
                InteractLayer(
                    targets1=self.targets1 + next_layer.targets1,
                    targets2=self.targets2 + next_layer.targets2,
                    bases1=self.bases1 + next_layer.bases1,
                    bases2=self.bases2 + next_layer.bases2,
                )
            ]
        return [self, next_layer]
