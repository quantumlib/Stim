from __future__ import annotations

import dataclasses
from typing import Any, cast, Literal, TypeVar

import stim

from stimflow._core import min_max_complex
from stimflow._layers._layer_det_obs_annotation import DetObsAnnotationLayer
from stimflow._layers._layer_empty import LayerEmpty
from stimflow._layers._layer_feedback import LayerFeedback
from stimflow._layers._layer_interact import LayerInteract
from stimflow._layers._layer_interact_swap import LayerInteractSwap
from stimflow._layers._layer_iswap import LayerISwap
from stimflow._layers._layer import Layer
from stimflow._layers._layer_loop import LayerLoop
from stimflow._layers._layer_measure import LayerMeasure
from stimflow._layers._layer_mpp import LayerMpp
from stimflow._layers._layer_noise import LayerNoise
from stimflow._layers._layer_qubit_coord_annotation import LayerQubitCoordAnnotation
from stimflow._layers._layer_reset import LayerReset
from stimflow._layers._layer_rotation import LayerRotation
from stimflow._layers._layer_shift_coord_annotation import LayerShiftCoordAnnotation
from stimflow._layers._layer_sqrt_pp import LayerSqrtPP
from stimflow._layers._layer_swap import LayerSwap
from stimflow._layers._layer_tag import LayerTag

TLayer = TypeVar("TLayer")


@dataclasses.dataclass
class LayerCircuit:
    """A stabilizer circuit represented as a series of typed layers.

    For example, the circuit could be a `LayerReset`, then a `LayerRotation`,
    then a few `LayerInteract`s, then a `LayerMeasure`.
    """

    layers: list[Layer] = dataclasses.field(default_factory=list)

    def _min_max_complex_(self) -> tuple[complex, complex]:
        return min_max_complex(
            v[0] + v[1] * 1j
            for v in obj.to_stim_circuit().get_final_qubit_coordinates().values()
        )

    def touched(self) -> set[int]:
        result = set()
        for layer in self.layers:
            result |= layer.touched()
        return result

    def copy(self) -> LayerCircuit:
        return LayerCircuit(layers=[e.copy() for e in self.layers])

    def to_z_basis(self) -> LayerCircuit:
        result = LayerCircuit()
        for layer in self.layers:
            result.layers.extend(layer.to_z_basis())
        return result

    def _feed(self, kind: type[TLayer]) -> TLayer:
        if not self.layers:
            self.layers.append(cast(Layer, kind()))
        elif isinstance(self.layers[-1], LayerEmpty):
            self.layers[-1] = cast(Layer, kind())
        elif not isinstance(self.layers[-1], kind):
            self.layers.append(cast(Layer, kind()))
        return cast(TLayer, self.layers[-1])

    def _feed_reset(self, basis: Literal["X", "Y", "Z"], targets: list[stim.GateTarget]):
        layer = self._feed(LayerReset)
        for t in targets:
            layer.targets[t.value] = basis

    def _feed_tag(self, instruction: stim.CircuitInstruction):
        layer = self._feed(LayerTag)
        layer.circuit.append(instruction)

    def _feed_m(self, basis: Literal["X", "Y", "Z"], targets: list[stim.GateTarget]):
        layer = self._feed(LayerMeasure)
        for t in targets:
            layer.bases.append(basis)
            layer.targets.append(t.value)

    def _feed_mpp(self, targets: list[stim.GateTarget]):
        layer = self._feed(LayerMpp)
        start = 0
        end = 1
        while start < len(targets):
            while end < len(targets) and targets[end].is_combiner:
                end += 2
            layer.targets.append(targets[start:end:2])
            start = end
            end += 1

    def _feed_qubit_coords(self, targets: list[stim.GateTarget], gate_args: list[float]):
        layer = self._feed(LayerQubitCoordAnnotation)
        for target in targets:
            assert target.is_qubit_target
            q = target.value
            if q in layer.coords:
                raise ValueError(f"Qubit coords specified twice for {q}")
            layer.coords[q] = list(gate_args)

    def _feed_shift_coords(self, gate_args: list[float]):
        self._feed(LayerShiftCoordAnnotation).offset_by(gate_args)

    def _feed_named_rotation_instruction(self, instruction: stim.CircuitInstruction):
        layer = self._feed(LayerRotation)
        name = instruction.name
        for t in instruction.targets_copy():
            layer.append_named_rotation(name, t.value)

    def _feed_swap(self, targets: list[stim.GateTarget]):
        layer = self._feed(LayerSwap)
        for k in range(0, len(targets), 2):
            layer.targets1.append(targets[k].value)
            layer.targets2.append(targets[k + 1].value)

    def _feed_cxswap(self, targets: list[stim.GateTarget]):
        layer: LayerInteractSwap = self._feed(LayerInteractSwap)
        for k in range(0, len(targets), 2):
            layer.i_layer.targets1.append(targets[k].value)
            layer.i_layer.targets2.append(targets[k + 1].value)
            layer.i_layer.bases1.append("Z")
            layer.i_layer.bases2.append("X")

    def _feed_swapcx(self, targets: list[stim.GateTarget]):
        layer: LayerInteractSwap = self._feed(LayerInteractSwap)
        for k in range(0, len(targets), 2):
            layer.i_layer.targets1.append(targets[k].value)
            layer.i_layer.targets2.append(targets[k + 1].value)
            layer.i_layer.bases1.append("X")
            layer.i_layer.bases2.append("Z")

    def _feed_iswap(self, targets: list[stim.GateTarget]):
        layer = self._feed(LayerISwap)
        for k in range(0, len(targets), 2):
            layer.targets1.append(targets[k].value)
            layer.targets2.append(targets[k + 1].value)

    def _feed_sqrt_pp(self, basis: Literal["X", "Y", "Z"], targets: list[stim.GateTarget]):
        layer = self._feed(LayerSqrtPP)
        for k in range(0, len(targets), 2):
            layer.targets1.append(targets[k].value)
            layer.targets2.append(targets[k + 1].value)
            layer.bases.append(basis)

    def _feed_c(
        self,
        basis1: Literal["X", "Y", "Z"],
        basis2: Literal["X", "Y", "Z"],
        targets: list[stim.GateTarget],
    ):
        is_feedback = any(t.is_sweep_bit_target or t.is_measurement_record_target for t in targets)
        if is_feedback:
            f_layer: LayerFeedback = self._feed(LayerFeedback)
            for k in range(0, len(targets), 2):
                c = targets[k]
                t = targets[k + 1]
                if t.is_sweep_bit_target or t.is_measurement_record_target:
                    c, t = t, c
                    f_layer.bases.append(basis1)
                else:
                    f_layer.bases.append(basis2)
                f_layer.controls.append(c)
                f_layer.targets.append(t.value)
        else:
            i_layer: LayerInteract = self._feed(LayerInteract)
            for k in range(0, len(targets), 2):
                i_layer.bases1.append(basis1)
                i_layer.bases2.append(basis2)
                i_layer.targets1.append(targets[k].value)
                i_layer.targets2.append(targets[k + 1].value)

    @staticmethod
    def from_stim_circuit(circuit: stim.Circuit) -> LayerCircuit:
        result = LayerCircuit()
        for instruction in circuit:
            gate_data = stim.gate_data(instruction.name)
            if isinstance(instruction, stim.CircuitRepeatBlock):
                result.layers.append(
                    LayerLoop(
                        body=LayerCircuit.from_stim_circuit(instruction.body_copy()),
                        repetitions=instruction.repeat_count,
                    )
                )

            elif instruction.tag:
                result._feed_tag(instruction)

            elif instruction.name == "R":
                result._feed_reset("Z", instruction.targets_copy())
            elif instruction.name == "RX":
                result._feed_reset("X", instruction.targets_copy())
            elif instruction.name == "RY":
                result._feed_reset("Y", instruction.targets_copy())

            elif instruction.name == "M":
                result._feed_m("Z", instruction.targets_copy())
            elif instruction.name == "MX":
                result._feed_m("X", instruction.targets_copy())
            elif instruction.name == "MY":
                result._feed_m("Y", instruction.targets_copy())

            elif instruction.name == "MR":
                result._feed_m("Z", instruction.targets_copy())
                result._feed_reset("Z", instruction.targets_copy())
            elif instruction.name == "MRX":
                result._feed_m("X", instruction.targets_copy())
                result._feed_reset("X", instruction.targets_copy())
            elif instruction.name == "MRY":
                result._feed_m("Y", instruction.targets_copy())
                result._feed_reset("Y", instruction.targets_copy())

            elif instruction.name == "XCX":
                result._feed_c("X", "X", instruction.targets_copy())
            elif instruction.name == "XCY":
                result._feed_c("X", "Y", instruction.targets_copy())
            elif instruction.name == "XCZ":
                result._feed_c("X", "Z", instruction.targets_copy())
            elif instruction.name == "YCX":
                result._feed_c("Y", "X", instruction.targets_copy())
            elif instruction.name == "YCY":
                result._feed_c("Y", "Y", instruction.targets_copy())
            elif instruction.name == "YCZ":
                result._feed_c("Y", "Z", instruction.targets_copy())
            elif instruction.name == "CX":
                result._feed_c("Z", "X", instruction.targets_copy())
            elif instruction.name == "CY":
                result._feed_c("Z", "Y", instruction.targets_copy())
            elif instruction.name == "CZ":
                result._feed_c("Z", "Z", instruction.targets_copy())

            elif gate_data.is_single_qubit_gate and gate_data.is_unitary:
                result._feed_named_rotation_instruction(instruction)

            elif instruction.name == "QUBIT_COORDS":
                result._feed_qubit_coords(instruction.targets_copy(), instruction.gate_args_copy())
            elif instruction.name == "SHIFT_COORDS":
                result._feed_shift_coords(instruction.gate_args_copy())
            elif instruction.name in ["DETECTOR", "OBSERVABLE_INCLUDE"]:
                result._feed(DetObsAnnotationLayer).circuit.append(instruction)

            elif instruction.name in ["ISWAP", "ISWAP_DAG"]:
                result._feed_iswap(instruction.targets_copy())
            elif instruction.name == "MPP":
                result._feed_mpp(instruction.targets_copy())
            elif instruction.name == "SWAP":
                result._feed_swap(instruction.targets_copy())
            elif instruction.name == "CXSWAP":
                result._feed_cxswap(instruction.targets_copy())
            elif instruction.name == "SWAPCX":
                result._feed_swapcx(instruction.targets_copy())

            elif instruction.name == "TICK":
                result.layers.append(LayerEmpty())

            elif instruction.name == "SQRT_XX" or instruction.name == "SQRT_XX_DAG":
                result._feed_sqrt_pp("X", instruction.targets_copy())
            elif instruction.name == "SQRT_YY" or instruction.name == "SQRT_YY_DAG":
                result._feed_sqrt_pp("Y", instruction.targets_copy())
            elif instruction.name == "SQRT_ZZ" or instruction.name == "SQRT_ZZ_DAG":
                result._feed_sqrt_pp("Z", instruction.targets_copy())
            elif (
                instruction.name == "DEPOLARIZE1"
                or instruction.name == "X_ERROR"
                or instruction.name == "Y_ERROR"
                or instruction.name == "Z_ERROR"
                or instruction.name == "DEPOLARIZE2"
            ):
                result._feed(LayerNoise).circuit.append(instruction)

            else:
                raise NotImplementedError(f"{instruction=}")
        return result

    def __repr__(self) -> str:
        result = ["LayerCircuit(layers=["]
        for layer in self.layers:
            r = repr(layer)
            for line in r.split("\n"):
                result.append("\n    " + line)
            result.append(",")
        result.append("\n])")
        return "".join(result)

    def with_qubit_coords_at_start(self) -> LayerCircuit:
        k = len(self.layers)
        merged_layer = LayerQubitCoordAnnotation()
        rev_layers: list[Layer] = []
        while k > 0:
            k -= 1
            layer = self.layers[k]
            if isinstance(layer, LayerQubitCoordAnnotation):
                intersection = merged_layer.coords.keys() & layer.coords.keys()
                if intersection:
                    raise ValueError(
                        f"Qubit coords specified twice for qubits {sorted(intersection)}"
                    )
                merged_layer.coords.update(layer.coords)
            elif isinstance(layer, LayerShiftCoordAnnotation):
                merged_layer.offset_by(layer.shift)
                rev_layers.append(layer)
            elif isinstance(layer, LayerLoop):
                if merged_layer.coords:
                    raise NotImplementedError("Moving qubit coords across a loop.")
                rev_layers.append(layer)
            else:
                rev_layers.append(layer)
        rev_layers.append(merged_layer)
        return LayerCircuit(layers=rev_layers[::-1])

    def with_locally_optimized_layers(self) -> LayerCircuit:
        """Iterates over the circuit aggregating layer.optimized(second_layer)."""
        new_layers: list[Layer] = []

        def do_layer(layer: Layer | None):
            if new_layers:
                new_layers[-1:] = new_layers[-1].locally_optimized(layer)
            else:
                new_layers.append(layer)
            while new_layers and (new_layers[-1] is None or new_layers[-1].is_vacuous()):
                new_layers.pop()

        for e in self.layers:
            for opt in e.locally_optimized(None):
                do_layer(opt)
        do_layer(None)
        return LayerCircuit(layers=new_layers)

    def _resets_at_layer(self, k: int, *, end_resets: set[int]) -> set[int]:
        if k >= len(self.layers):
            return end_resets

        layer = self.layers[k]
        if isinstance(layer, LayerReset):
            return layer.touched()
        if isinstance(layer, LayerLoop):
            return layer.body._resets_at_layer(0, end_resets=set())
        return set()

    def with_rotations_before_resets_removed(
        self, loop_boundary_resets: set[int] | None = None
    ) -> LayerCircuit:
        all_touched = self.touched()
        if loop_boundary_resets is None:
            loop_boundary_resets = set()
        sets: list[set[int]] = [layer.touched() for layer in self.layers]
        sets.append(all_touched)
        resets: list[set[int]] = [
            self._resets_at_layer(k, end_resets=all_touched) for k in range(len(self.layers))
        ]
        if loop_boundary_resets is None:
            resets.append(all_touched)
        elif len(resets) == 0:
            resets.append(set())
        else:
            resets.append(loop_boundary_resets & resets[0])
        new_layers: list[Layer] = [layer.copy() for layer in self.layers]

        for k, layer in enumerate(new_layers):
            if isinstance(layer, LayerLoop):
                layer.body = layer.body.with_rotations_before_resets_removed(
                    loop_boundary_resets=self._resets_at_layer(k + 1, end_resets=all_touched)
                )
            elif isinstance(layer, LayerRotation):
                drops = []
                for q, gate in layer.named_rotations.items():
                    if gate != "I":
                        k2 = k + 1
                        while k2 < len(sets):
                            if q in sets[k2]:
                                if q in resets[k2]:
                                    drops.append(q)
                                break
                            k2 += 1
                for q in drops:
                    del layer.named_rotations[q]

        return LayerCircuit([layer for layer in new_layers if not layer.is_vacuous()])

    def with_clearable_rotation_layers_cleared(self) -> LayerCircuit:
        """Removes rotation layers where every rotation in the layer can be moved to another layer.

        Each individual rotation can move through intermediate non-rotation layers as long as those
        layers don't touch the qubit being rotated.
        """
        sets = [layer.touched() for layer in self.layers]

        def scan(qubit: int, start_layer: int, delta: int) -> int | None:
            while True:
                start_layer += delta
                if start_layer < 0 or start_layer >= len(sets):
                    return None
                if (
                    isinstance(new_layers[start_layer], LayerRotation)
                    and not new_layers[start_layer].is_vacuous()
                ):
                    return start_layer
                if qubit in sets[start_layer]:
                    return None

        new_layers = [layer.copy() for layer in self.layers]
        cur_layer_index = 0
        while cur_layer_index < len(new_layers):
            layer = new_layers[cur_layer_index]
            if isinstance(layer, LayerRotation):
                rewrites = {}
                for q, r in layer.named_rotations.items():
                    if r == "I":
                        continue
                    new_layer_index = scan(q, cur_layer_index, -1)
                    if new_layer_index is None:
                        new_layer_index = scan(q, cur_layer_index, +1)
                    if new_layer_index is not None:
                        rewrites[q] = new_layer_index
                    else:
                        break
                else:
                    for q, r in layer.named_rotations.items():
                        if r == "I":
                            continue
                        new_layer_index = rewrites[q]
                        new_layer: LayerRotation = cast(LayerRotation, new_layers[new_layer_index])
                        if new_layer_index > cur_layer_index:
                            new_layer.prepend_named_rotation(r, q)
                        else:
                            new_layer.append_named_rotation(r, q)
                        if new_layer.named_rotations.get(q) != "I":
                            sets[new_layer_index].add(q)
                        elif q in sets[new_layer_index]:
                            sets[new_layer_index].remove(q)
                    layer.named_rotations.clear()
                    sets[cur_layer_index].clear()
            elif isinstance(layer, LayerLoop):
                layer.body = layer.body.with_clearable_rotation_layers_cleared()
            cur_layer_index += 1
        return LayerCircuit([layer for layer in new_layers if not layer.is_vacuous()])

    def with_rotations_rolled_from_end_of_loop_to_start_of_loop(self) -> LayerCircuit:
        """Rewrites loops so that they only have rotations at the start, not the end.

        This is useful for ensuring loops don't redundantly rotate at the loop boundary,
        by merging the rotations at the end with the rotations at the start or by
        making it clear rotations at the end were not needed because of the
        operations coming next.

        For example, this:

            REPEAT 5 {
                S 2 3 4
                R 0 1
                ...
                M 0 1
                H 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
                DETECTOR rec[-1]
            }

        will become this:

            REPEAT 5 {
                H 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
                S 2 3 4
                R 0 1
                ...
                M 0 1
                DETECTOR rec[-1]
            }

        which later optimization passes can then reduce further.
        """

        new_layers: list[Layer] = []
        for layer in self.layers:
            handled = False
            if isinstance(layer, LayerLoop):
                loop_layers = list(layer.body.layers)
                rot_layer_index = len(loop_layers) - 1
                while rot_layer_index > 0:
                    if isinstance(
                        loop_layers[rot_layer_index],
                        (DetObsAnnotationLayer, LayerShiftCoordAnnotation),
                    ):
                        rot_layer_index -= 1
                        continue
                    if isinstance(loop_layers[rot_layer_index], LayerRotation):
                        break
                    # Loop didn't end with a rotation layer; give up.
                    rot_layer_index = 0
                if rot_layer_index > 0:
                    handled = True
                    popped = cast(LayerRotation, loop_layers.pop(rot_layer_index))
                    loop_layers.insert(0, popped)

                    new_layers.append(popped.inverse())
                    new_layers.append(
                        LayerLoop(body=LayerCircuit(loop_layers), repetitions=layer.repetitions)
                    )
                    new_layers.append(popped.copy())
            if not handled:
                new_layers.append(layer)
        return LayerCircuit([layer for layer in new_layers if not layer.is_vacuous()])

    def with_rotations_merged_earlier(self) -> LayerCircuit:
        sets = [layer.touched() for layer in self.layers]

        def scan(qubit: int, start_layer: int) -> int | None:
            while True:
                start_layer -= 1
                if start_layer < 0:
                    return None
                l = new_layers[start_layer]
                if isinstance(l, LayerRotation) and qubit in l.named_rotations:
                    return start_layer
                if qubit in sets[start_layer]:
                    return None

        new_layers = [layer.copy() for layer in self.layers]
        cur_layer_index = 0
        while cur_layer_index < len(new_layers):
            layer = new_layers[cur_layer_index]
            if isinstance(layer, LayerRotation):
                rewrites = {}
                for q, gate in layer.named_rotations.items():
                    if gate == "I":
                        continue
                    v = scan(q, cur_layer_index)
                    if v is not None:
                        rewrites[q] = v
                for q, dst in rewrites.items():
                    new_layer: LayerRotation = cast(LayerRotation, new_layers[dst])
                    new_layer.append_named_rotation(layer.named_rotations.pop(q), q)
                    sets[cur_layer_index].remove(q)
                    if new_layer.named_rotations.get(q):
                        sets[dst].add(q)
                    elif q in sets[dst]:
                        sets[dst].remove(q)
            elif isinstance(layer, LayerLoop):
                layer.body = layer.body.with_rotations_merged_earlier()
            cur_layer_index += 1
        return LayerCircuit([layer for layer in new_layers if not layer.is_vacuous()])

    def with_whole_rotation_layers_slid_earlier(self) -> LayerCircuit:
        rev_layers: list[Layer] = []
        cur_rot_layer: LayerRotation | None = None
        cur_rot_touched: set[int] | None = None
        for layer in self.layers[::-1]:
            if cur_rot_layer is not None and not layer.touched().isdisjoint(cur_rot_touched):
                rev_layers.append(cur_rot_layer)
                cur_rot_layer = None
                cur_rot_touched = None
            if isinstance(layer, LayerRotation):
                layer = layer.copy()
                if cur_rot_layer is not None:
                    layer.named_rotations.update(cur_rot_layer.named_rotations)
                cur_rot_layer = layer
                cur_rot_touched = cur_rot_layer.touched()
            else:
                rev_layers.append(layer)
        if cur_rot_layer is not None:
            rev_layers.append(cur_rot_layer)
        return LayerCircuit(rev_layers[::-1])

    def with_ejected_loop_iterations(self) -> LayerCircuit:
        """Partially unrolls loops, placing one iteration before and one iteration after.

        This is useful for ensuring the transition into and out of a loop is optimized correctly.
        For example, if a circuit begins with a transversal initialization of data qubits and then
        immediately starts a memory loop, the resets from the data initialization should be merged
        into the same layer as the resets from the measurement initialization at the beginning of
        the loop. But the reset-merging optimization might not see that this is possible across the
        loop boundary. Ejecting an iteration fixes this issue.

        For example, this method would turn this circuit fragment:

            REPEAT 100 {
                X 0
                MR 0
            }

        into this circuit fragment:

            X 0
            MR 0
            REPEAT 98 {
                X 0
                MR 0
            }
            X 0
            MR 0
        """
        new_layers: list[Layer] = []
        for layer in self.layers:
            if isinstance(layer, LayerLoop):
                if layer.repetitions == 0:
                    pass
                elif layer.repetitions == 1:
                    new_layers.extend(layer.body.layers)
                elif layer.repetitions == 2:
                    new_layers.extend(layer.body.layers)
                    new_layers.extend(layer.body.layers)
                else:
                    new_layers.extend(layer.body.layers)
                    new_layers.append(
                        LayerLoop(body=layer.body.copy(), repetitions=layer.repetitions - 2)
                    )
                    new_layers.extend(layer.body.layers)
                    assert layer.repetitions > 2
            else:
                new_layers.append(layer)
        return LayerCircuit(new_layers)

    def without_empty_layers(self) -> LayerCircuit:
        """Removes empty layers from the circuit.

        Empty layers are sometimes created as a byproduct of certain optimizations, or may have been
        present in the original circuit. Usually they are unwanted, and this method removes them.
        """
        new_layers: list[Layer] = []
        for layer in self.layers:
            if isinstance(layer, LayerEmpty):
                pass
            elif isinstance(layer, LayerLoop):
                new_layers.append(LayerLoop(layer.body.without_empty_layers(), layer.repetitions))
            else:
                new_layers.append(layer)
        return LayerCircuit(new_layers)

    def with_cleaned_up_loop_iterations(self) -> LayerCircuit:
        """Attempts to roll up partially unrolled loops.

        Checks if the instructions before a loop correspond to the instruction inside a loop. If so,
        removes the matching instructions beforehand and increases the iteration count by 1. Same
        for instructions after the loop.

        This essentially undoes the effect of `with_ejected_loop_iterations`. A common pattern is
        to do `with_ejected_loop_iterations`, then an optimization, then
        `with_cleaned_up_loop_iterations`. This gives the optimization the chance to optimize across
        a loop boundary, but cleans up after itself if no optimization occurs.

        In some cases this method is useful because of circuit generation code being overly cautious
        about how quickly loop invariants are established, and so emitting the first iteration of a
        loop in a special way. If it happens to be identical, despite the different code path that
        produced it, this method will roll it into the rest of the loop.

        For example, this method would turn this circuit fragment:

            X 0
            MR 0
            REPEAT 98 {
                X 0
                MR 0
            }
            X 0
            MR 0

        into this circuit fragment:

            REPEAT 100 {
                X 0
                MR 0
            }
        """
        new_layers = list(self.without_empty_layers().layers)
        k = 0
        while k < len(new_layers):
            cur_layer = new_layers[k]
            if isinstance(cur_layer, LayerLoop):
                body_layers = cur_layer.body.layers
                reps = cur_layer.repetitions
                while k >= len(body_layers) and new_layers[k - len(body_layers) : k] == body_layers:
                    new_layers[k - len(body_layers) : k] = []
                    k -= len(body_layers)
                    reps += 1
                while (
                    k + len(body_layers) < len(new_layers)
                    and new_layers[k + 1 : k + 1 + len(body_layers)] == body_layers
                ):
                    new_layers[k + 1 : k + 1 + len(body_layers)] = []
                    reps += 1
                new_layers[k] = LayerLoop(LayerCircuit(body_layers), reps)
            k += 1
        return LayerCircuit(new_layers)

    def with_locally_merged_measure_layers(self) -> LayerCircuit:
        """Merges measurement layers together, despite intervening annotation layers.

        For example, this method would turn this circuit fragment:

            M 0
            DETECTOR(0, 0) rec[-1]
            OBSERVABLE_INCLUDE(5) rec[-1]
            SHIFT_COORDS(0, 1)
            M 1
            DETECTOR(0, 0) rec[-1]

        into this circuit fragment:

            M 0 1
            DETECTOR(0, 0) rec[-2]
            OBSERVABLE_INCLUDE(5) rec[-2]
            SHIFT_COORDS(0, 1)
            DETECTOR(0, 0) rec[-1]
        """
        new_layers: list[Layer] = []
        k = 0
        while k < len(self.layers):
            cur_layer = self.layers[k]
            if isinstance(cur_layer, LayerMeasure):
                m1: LayerMeasure = cur_layer
                k2 = k + 1
                while k2 < len(self.layers) and isinstance(
                    self.layers[k2], (DetObsAnnotationLayer, LayerShiftCoordAnnotation)
                ):
                    k2 += 1
                if k2 < len(self.layers) and isinstance(self.layers[k2], LayerMeasure):
                    m2: LayerMeasure = cast(LayerMeasure, self.layers[k2])
                    if set(m1.targets).isdisjoint(set(m2.targets)):
                        new_layers.append(
                            LayerMeasure(targets=m1.targets + m2.targets, bases=m1.bases + m2.bases)
                        )
                        for k3 in range(k + 1, k2):
                            l3: DetObsAnnotationLayer | LayerShiftCoordAnnotation
                            l3 = cast(Any, self.layers[k3])
                            new_layers.append(l3.with_rec_targets_shifted_by(-len(m2.targets)))
                        k = k2 + 1
                        continue
            new_layers.append(self.layers[k].copy())
            k += 1
        return LayerCircuit(new_layers)

    def with_whole_layers_slid_as_to_merge_with_previous_layer_of_same_type(
        self, layer_types: type | tuple[type, ...]
    ) -> LayerCircuit:
        new_layers = list(self.layers)
        k = 0
        while k < len(new_layers):
            if isinstance(new_layers[k], layer_types):
                touched = new_layers[k].touched()
                k_prev = k
                while k_prev > 0 and new_layers[k_prev - 1].touched().isdisjoint(touched):
                    k_prev -= 1
                    if k_prev != k and type(new_layers[k_prev]) == type(new_layers[k]):
                        (new_layer,) = (
                            e
                            for e in new_layers[k_prev].locally_optimized(new_layers[k])
                            if e is not None
                        )
                        del new_layers[k]
                        new_layers[k_prev] = new_layer
                        break
            k += 1
        return LayerCircuit(new_layers)

    def with_whole_layers_slid_as_early_as_possible_for_merge_with_same_layer(
        self, layer_types: type | tuple[type, ...]
    ) -> LayerCircuit:
        new_layers = list(self.layers)
        k = 0
        while k < len(new_layers):
            if isinstance(new_layers[k], layer_types):
                touched = new_layers[k].touched()
                k_prev = k
                while k_prev > 0 and new_layers[k_prev - 1].touched().isdisjoint(touched):
                    k_prev -= 1
                while k_prev < k and type(new_layers[k_prev]) != type(new_layers[k]):
                    k_prev += 1
                if k_prev != k:
                    (new_layer,) = (
                        e
                        for e in new_layers[k_prev].locally_optimized(new_layers[k])
                        if e is not None
                    )
                    del new_layers[k]
                    new_layers[k_prev] = new_layer
                    continue
            k += 1
        return LayerCircuit(new_layers)

    def with_irrelevant_tail_layers_removed(self) -> LayerCircuit:
        irrelevant_layer_types_at_end = (
            LayerReset,
            LayerInteract,
            LayerFeedback,
            LayerRotation,
            LayerSwap,
            LayerISwap,
            LayerInteractSwap,
            LayerEmpty,
        )
        tail = []
        result = list(self.layers)
        while True:
            while len(result) > 0 and isinstance(result[-1], irrelevant_layer_types_at_end):
                result.pop()
            if len(result) > 0 and isinstance(result[-1], DetObsAnnotationLayer):
                tail.append(result.pop())
            else:
                break
        result.extend(tail)
        return LayerCircuit(result)

    def to_stim_circuit(self) -> stim.Circuit:
        """Compiles the layer circuit into a stim circuit and returns it."""
        circuit = stim.Circuit()
        tick_coming = False
        for layer in self.layers:
            if tick_coming and layer.requires_tick_before():
                circuit.append("TICK")
                tick_coming = False
            layer.append_into_stim_circuit(circuit)
            tick_coming |= layer.implies_eventual_tick_after()
        return circuit
