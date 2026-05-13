from __future__ import annotations

import collections
import dataclasses

import stim

from stimflow._layers._layer_interact import LayerInteract
from stimflow._layers._layer import Layer
from stimflow._layers._layer_rotation import LayerRotation


@dataclasses.dataclass
class LayerSqrtPP(Layer):
    targets1: list[int] = dataclasses.field(default_factory=list)
    targets2: list[int] = dataclasses.field(default_factory=list)
    bases: list[str] = dataclasses.field(default_factory=list)

    def touched(self) -> set[int]:
        return set(self.targets1 + self.targets2)

    def copy(self) -> LayerSqrtPP:
        return LayerSqrtPP(
            targets1=list(self.targets1), targets2=list(self.targets2), bases=list(self.bases)
        )

    def to_z_basis(self) -> list[Layer]:
        interact = LayerInteract()
        rot = LayerRotation()
        for q1, q2, b in zip(self.targets1, self.targets2, self.bases):
            interact.targets1.append(q1)
            interact.targets2.append(q2)
            interact.bases1.append(b)
            interact.bases1.append(b)
            if b == "X":
                r = "SQRT_X"
            elif b == "Y":
                r = "SQRT_Y"
            elif b == "Z":
                r = "S"
            else:
                raise NotImplementedError(f"{b=}")
            rot.append_named_rotation(r, q1)
            rot.append_named_rotation(r, q2)

        return [rot, *interact.to_z_basis()]

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        groups = collections.defaultdict(list)
        for q1, q2, b in zip(self.targets1, self.targets2, self.bases):
            gate = f"SQRT_{b}{b}"
            if q2 < q1:
                q1, q2 = q2, q1
            groups[gate].append((q1, q2))
        for gate in sorted(groups.keys()):
            for pair in sorted(groups[gate]):
                out.append(gate, pair)
