from __future__ import annotations

import dataclasses

import stim

from stimflow._layers._data import (
    single_qubit_clifford_inverse_table,
    single_qubit_clifford_multiplication_table,
)
from stimflow._layers._layer import Layer


@dataclasses.dataclass
class LayerRotation(Layer):
    """A layer of single qubit Clifford rotation gates."""

    named_rotations: dict[int, str] = dataclasses.field(default_factory=dict)

    def touched(self) -> set[int]:
        return {k for k, v in self.named_rotations.items() if v != "I"}

    def copy(self) -> LayerRotation:
        return LayerRotation(dict(self.named_rotations))

    def inverse(self) -> LayerRotation:
        t = single_qubit_clifford_inverse_table()
        return LayerRotation({q: t[r] for q, r in self.named_rotations.items()})

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        gate2targets: dict[str, list[int]] = {}
        for key, val in self.named_rotations.items():
            gate2targets.setdefault(val, []).append(key)

        for gate, qs in sorted(gate2targets.items()):
            if gate != "I":
                qs = sorted(qs)
                if "*" in gate:
                    after, before = gate.split("*")
                    out.append(before, qs)
                    out.append(after, qs)
                else:
                    out.append(gate, qs)

    def prepend_named_rotation(self, name: str, target: int):
        m = single_qubit_clifford_multiplication_table()
        cur = self.named_rotations.get(target, "I")
        new_val = m[(cur, name)]
        if new_val == "I":
            self.named_rotations.pop(target, None)
        else:
            self.named_rotations[target] = new_val

    def append_named_rotation(self, name: str, target: int):
        m = single_qubit_clifford_multiplication_table()
        cur = self.named_rotations.get(target, "I")
        new_val = m[(name, cur)]
        if new_val == "I":
            self.named_rotations.pop(target, None)
        else:
            self.named_rotations[target] = new_val

    def is_vacuous(self) -> bool:
        return not any(self.named_rotations.values())

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        from stimflow._layers._layer_det_obs_annotation import DetObsAnnotationLayer
        from stimflow._layers._layer_feedback import LayerFeedback
        from stimflow._layers._layer_reset import LayerReset
        from stimflow._layers._layer_shift_coord_annotation import LayerShiftCoordAnnotation

        if isinstance(next_layer, (DetObsAnnotationLayer, LayerShiftCoordAnnotation)):
            return [next_layer, self]
        if isinstance(next_layer, LayerFeedback):
            return [next_layer.before(self), self]
        if isinstance(next_layer, LayerReset):
            trimmed = self.copy()
            for t in next_layer.targets.keys():
                trimmed.named_rotations.pop(t, None)
            if trimmed.named_rotations:
                return [trimmed, next_layer]
            else:
                return [next_layer]
        if isinstance(next_layer, LayerRotation):
            result = self.copy()
            for q, r in next_layer.named_rotations.items():
                result.append_named_rotation(r, q)
            return [result]
        return [self, next_layer]
