from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._layer_interact import LayerInteract
from stimflow._layers._layer import Layer
from stimflow._layers._layer_swap import LayerSwap


@dataclasses.dataclass
class LayerInteractSwap(Layer):
    i_layer: LayerInteract = dataclasses.field(default_factory=LayerInteract)

    def copy(self) -> LayerInteractSwap:
        return LayerInteractSwap(i_layer=self.i_layer.copy())

    def touched(self) -> set[int]:
        return self.i_layer.touched()

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        cz_swaps = []
        cx_swaps = []
        reduced_layer = LayerInteract()
        for t1, t2, b1, b2 in zip(
            self.i_layer.targets1, self.i_layer.targets2, self.i_layer.bases1, self.i_layer.bases2
        ):
            if b1 == b2 == "Z":
                if t2 < t1:
                    t1, t2 = t2, t1
                cz_swaps.append(t1)
                cz_swaps.append(t2)
            elif b1 == "X" and b2 == "Z":
                cx_swaps.append(t2)
                cx_swaps.append(t1)
            elif b2 == "X" and b1 == "Z":
                cx_swaps.append(t1)
                cx_swaps.append(t2)
            else:
                reduced_layer.targets1.append(t1)
                reduced_layer.targets2.append(t2)
                reduced_layer.bases1.append(b1)
                reduced_layer.bases2.append(b2)

        if cx_swaps:
            out.append("CXSWAP", cx_swaps)
        if cz_swaps:
            out.append("CZSWAP", cz_swaps)
        if reduced_layer.targets1:
            reduced_layer.append_into_stim_circuit(out)
            out.append("TICK")
            LayerSwap(reduced_layer.targets1, reduced_layer.targets2).append_into_stim_circuit(out)

    def to_z_basis(self) -> list[Layer]:
        return [
            self.i_layer.rotate_to_z_layer(),
            LayerInteractSwap(
                LayerInteract(
                    targets1=list(self.i_layer.targets1),
                    targets2=list(self.i_layer.targets2),
                    bases1=["Z"] * len(self.i_layer.bases1),
                    bases2=["Z"] * len(self.i_layer.bases2),
                )
            ),
            LayerInteract(
                targets1=self.i_layer.targets1,
                targets2=self.i_layer.targets2,
                bases1=self.i_layer.bases2,
                bases2=self.i_layer.bases1,
            ).rotate_to_z_layer(),
        ]

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        return [self, next_layer]
