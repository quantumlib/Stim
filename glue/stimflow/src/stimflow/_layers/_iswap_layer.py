from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer import Layer


@dataclasses.dataclass
class ISwapLayer(Layer):
    """A layer of iswap gates."""

    targets1: list[int] = dataclasses.field(default_factory=list)
    targets2: list[int] = dataclasses.field(default_factory=list)

    def copy(self) -> ISwapLayer:
        return ISwapLayer(targets1=list(self.targets1), targets2=list(self.targets2))

    def touched(self) -> set[int]:
        return set(self.targets1 + self.targets2)

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        pairs = []
        for k in range(len(self.targets1)):
            t1 = self.targets1[k]
            t2 = self.targets2[k]
            t1, t2 = sorted([t1, t2])
            pairs.append((t1, t2))
        for pair in sorted(pairs):
            out.append("ISWAP", pair)

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        return [self, next_layer]
