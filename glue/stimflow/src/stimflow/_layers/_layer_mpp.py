from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer import Layer


@dataclasses.dataclass
class LayerMpp(Layer):
    targets: list[list[stim.GateTarget]] = dataclasses.field(default_factory=list)

    def copy(self) -> LayerMpp:
        return LayerMpp(targets=[list(e) for e in self.targets])

    def touched(self) -> set[int]:
        return {t.value for mpp in self.targets for t in mpp}

    def to_z_basis(self) -> list[Layer]:
        return [self]

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        flat_targets = []
        for group in self.targets:
            for t in group:
                flat_targets.append(t)
                flat_targets.append(stim.target_combiner())
            flat_targets.pop()
        out.append("MPP", flat_targets)
