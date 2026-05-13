from __future__ import annotations

import collections
from collections.abc import Callable
from typing import Literal

import stim


def circuit_with_xz_flipped(circuit: stim.Circuit) -> stim.Circuit:
    result = stim.Circuit()
    for inst in circuit:
        if isinstance(inst, stim.CircuitRepeatBlock):
            result.append(
                stim.CircuitRepeatBlock(
                    body=circuit_with_xz_flipped(inst.body_copy()), repeat_count=inst.repeat_count
                )
            )
        else:
            other = stim.gate_data(inst.name).hadamard_conjugated(unsigned=True).name
            if other is None:
                raise NotImplementedError(f"{inst=}")
            result.append(
                stim.CircuitInstruction(other, inst.targets_copy(), inst.gate_args_copy())
            )
    return result


def circuit_to_dem_target_measurement_records_map(
    circuit: stim.Circuit,
) -> dict[stim.DemTarget, list[int]]:
    result: dict[stim.DemTarget, list[int]] = {}
    for k in range(circuit.num_observables):
        result[stim.target_logical_observable_id(k)] = []
    num_d = 0
    num_m = 0
    for inst in circuit.flattened():
        if inst.name == "DETECTOR":
            result[stim.target_relative_detector_id(num_d)] = [
                num_m + t.value for t in inst.targets_copy()
            ]
            num_d += 1
        elif inst.name == "OBSERVABLE_INCLUDE":
            result[stim.target_logical_observable_id(int(inst.gate_args_copy()[0]))].extend(
                num_m + t.value for t in inst.targets_copy()
            )
        else:
            c = stim.Circuit()
            c.append(inst)
            num_m += c.num_measurements
    return result


def count_measurement_layers(circuit: stim.Circuit) -> int:
    saw_measurement = False
    result = 0
    for instruction in circuit:
        if isinstance(instruction, stim.CircuitRepeatBlock):
            result += count_measurement_layers(instruction.body_copy()) * instruction.repeat_count
        elif isinstance(instruction, stim.CircuitInstruction):
            saw_measurement |= stim.gate_data(instruction.name).produces_measurements
            if instruction.name == "TICK":
                result += saw_measurement
                saw_measurement = False
        else:
            raise NotImplementedError(f"{instruction=}")
    result += saw_measurement
    return result


def gate_counts_for_circuit(circuit: stim.Circuit) -> collections.Counter[str]:
    """Determines gates used by a circuit, disambiguating MPP/feedback cases.

    MPP instructions are expanded into what they actually measure, such as
    "MXX" for MPP X1*X2 and "MXYZ" for MPP X4*Y5*Z7.

    Feedback instructions like `CX rec[-1] 0` become the gate "feedback".

    Sweep instructions like `CX sweep[2] 0` become the gate "sweep".
    """
    ANNOTATION_OPS = {
        "DETECTOR",
        "OBSERVABLE_INCLUDE",
        "QUBIT_COORDS",
        "SHIFT_COORDS",
        "TICK",
        "MPAD",
    }

    out: collections.Counter[str] = collections.Counter()
    for instruction in circuit:
        if isinstance(instruction, stim.CircuitRepeatBlock):
            for gate_name, v in gate_counts_for_circuit(instruction.body_copy()).items():
                out[gate_name] += v * instruction.repeat_count

        elif instruction.name in ["CX", "CY", "CZ", "XCZ", "YCZ"]:
            targets = instruction.targets_copy()
            for k in range(0, len(targets), 2):
                if (
                    targets[k].is_measurement_record_target
                    or targets[k + 1].is_measurement_record_target
                ):
                    out["feedback"] += 1
                elif targets[k].is_sweep_bit_target or targets[k + 1].is_sweep_bit_target:
                    out["sweep"] += 1
                else:
                    out[instruction.name] += 1

        elif instruction.name == "MPP":
            op = "M"
            targets = instruction.targets_copy()
            is_continuing = True
            for t in targets:
                if t.is_combiner:
                    is_continuing = True
                    continue
                p = (
                    "X"
                    if t.is_x_target
                    else "Y" if t.is_y_target else "Z" if t.is_z_target else "?"
                )
                if is_continuing:
                    op += p
                    is_continuing = False
                else:
                    if op == "MZ":
                        op = "M"
                    out[op] += 1
                    op = "M" + p
            if op:
                if op == "MZ":
                    op = "M"
                out[op] += 1

        elif stim.gate_data(instruction.name).is_two_qubit_gate:
            out[instruction.name] += len(instruction.targets_copy()) // 2
        elif (
            instruction.name in ANNOTATION_OPS
            or instruction.name == "E"
            or instruction.name == "ELSE_CORRELATED_ERROR"
        ):
            out[instruction.name] += 1
        else:
            out[instruction.name] += len(instruction.targets_copy())

    return out


def gates_used_by_circuit(circuit: stim.Circuit) -> set[str]:
    """Determines gates used by a circuit, disambiguating MPP/feedback cases.

    MPP instructions are expanded into what they actually measure, such as
    "MXX" for MPP X1*X2 and "MXYZ" for MPP X4*Y5*Z7.

    Feedback instructions like `CX rec[-1] 0` become the gate "feedback".

    Sweep instructions like `CX sweep[2] 0` become the gate "sweep".
    """
    out = set()
    for instruction in circuit:
        if isinstance(instruction, stim.CircuitRepeatBlock):
            out |= gates_used_by_circuit(instruction.body_copy())

        elif instruction.name in ["CX", "CY", "CZ", "XCZ", "YCZ"]:
            for a, b in instruction.target_groups():
                if a.is_measurement_record_target or b.is_measurement_record_target:
                    out.add("feedback")
                elif a.is_sweep_bit_target or b.is_sweep_bit_target:
                    out.add("sweep")
                else:
                    out.add(instruction.name)

        elif instruction.name == "MPP":
            op = "M"
            targets = instruction.targets_copy()
            is_continuing = True
            for t in targets:
                if t.is_combiner:
                    is_continuing = True
                    continue
                p = (
                    "X"
                    if t.is_x_target
                    else "Y" if t.is_y_target else "Z" if t.is_z_target else "?"
                )
                if is_continuing:
                    op += p
                    is_continuing = False
                else:
                    if op == "MZ":
                        op = "M"
                    out.add(op)
                    op = "M" + p
            if op:
                if op == "MZ":
                    op = "M"
                out.add(op)

        else:
            out.add(instruction.name)

    return out


def stim_circuit_with_transformed_coords(
    circuit: stim.Circuit, transform: Callable[[complex], complex]
) -> stim.Circuit:
    """Returns an equivalent circuit, but with the qubit and detector position metadata modified.
    The "position" is assumed to be the first two coordinates. These are mapped to the real and
    imaginary values of a complex number which is then transformed.

    Note that `SHIFT_COORDS` instructions that modify the first two coordinates are not supported.
    This is because supporting them requires flattening loops, or promising that the given
    transformation is affine.

    Args:
        circuit: The circuit with qubits to reposition.
        transform: The transformation to apply to the positions. The positions are given one by one
            to this method, as complex numbers. The method returns the new complex number for the
            position.

    Returns:
        The transformed circuit.
    """
    result = stim.Circuit()
    for instruction in circuit:
        if isinstance(instruction, stim.CircuitInstruction):
            if instruction.name == "QUBIT_COORDS" or instruction.name == "DETECTOR":
                args = list(instruction.gate_args_copy())
                while len(args) < 2:
                    args.append(0)
                c = transform(args[0] + args[1] * 1j)
                args[0] = c.real
                args[1] = c.imag
                result.append(instruction.name, instruction.targets_copy(), args)
                continue
            if instruction.name == "SHIFT_COORDS":
                args = instruction.gate_args_copy()
                if any(args[:2]):
                    raise NotImplementedError(f"Shifting first two coords: {instruction=}")

        if isinstance(instruction, stim.CircuitRepeatBlock):
            result.append(
                stim.CircuitRepeatBlock(
                    repeat_count=instruction.repeat_count,
                    body=stim_circuit_with_transformed_coords(instruction.body_copy(), transform),
                )
            )
            continue

        result.append(instruction)
    return result


def stim_circuit_with_transformed_moments(
    circuit: stim.Circuit, *, moment_func: Callable[[stim.Circuit], stim.Circuit]
) -> stim.Circuit:
    """Applies a transformation to regions of a circuit separated by TICKs and blocks.

    For example, in this circuit:

        H 0
        X 0
        TICK

        H 1
        X 1
        REPEAT 100 {
            H 2
            X 2
        }
        H 3
        X 3

        TICK
        H 4
        X 4

    `moment_func` would be called five times, each time with one of the H and X instruction pairs.
    The result from the method would then be substituted into the circuit, replacing each of the H
    and X instruction pairs.

    Args:
        circuit: The circuit to return a transformed result of.
        moment_func: The transformation to apply to regions of the circuit. Returns a new circuit
            for the result.

    Returns:
        A transformed circuit.
    """

    result = stim.Circuit()
    current_moment = stim.Circuit()

    for instruction in circuit:
        if isinstance(instruction, stim.CircuitRepeatBlock):
            # Implicit tick at transition into REPEAT?
            if current_moment:
                result += moment_func(current_moment)
                current_moment.clear()

            transformed_body = stim_circuit_with_transformed_moments(
                instruction.body_copy(), moment_func=moment_func
            )
            result.append(
                stim.CircuitRepeatBlock(
                    repeat_count=instruction.repeat_count, body=transformed_body
                )
            )
        elif isinstance(instruction, stim.CircuitInstruction) and instruction.name == "TICK":
            # Explicit tick. Process even if empty.
            result += moment_func(current_moment)
            result.append("TICK")
            current_moment.clear()
        else:
            current_moment.append(instruction)

    # Implicit tick at end of circuit?
    if current_moment:
        result += moment_func(current_moment)

    return result


def append_reindexed_content_to_circuit(
    *,
    out_circuit: stim.Circuit,
    content: stim.Circuit,
    qubit_i2i: dict[int, int],
    obs_i2i: dict[int, int | Literal["discard"]],
    rewrite_detector_time_coordinates: bool = False,
) -> None:
    """Reindexes content and appends it to a circuit.

    Note that QUBIT_COORDS instructions are skipped.

    Args:
        out_circuit: The output circuit. The circuit being edited.
        content: The circuit to be appended to the output circuit.
        qubit_i2i: A dictionary specifying how qubit indices are remapped. Indices outside the
            map are not changed.
        obs_i2i: A dictionary specifying how observable indices are remapped. Indices outside the
            map are not changed.
        rewrite_detector_time_coordinates: Defaults to False. When set to True, SHIFT_COORD and
            DETECTOR instructions are automatically rewritten to track the passage of time without
            using the same detector position twice at the same time.
    """

    def rewritten_targets(inst: stim.CircuitInstruction) -> list[stim.GateTarget]:
        new_targets: list[int | stim.GateTarget] = []
        for t in inst.targets_copy():
            if t.is_qubit_target:
                new_targets.append(qubit_i2i.get(t.value, t.value))
            elif t.is_x_target:
                new_targets.append(stim.target_x(qubit_i2i.get(t.value, t.value)))
            elif t.is_y_target:
                new_targets.append(stim.target_y(qubit_i2i.get(t.value, t.value)))
            elif t.is_z_target:
                new_targets.append(stim.target_z(qubit_i2i.get(t.value, t.value)))
            elif t.is_combiner:
                new_targets.append(t)
            elif t.is_measurement_record_target:
                new_targets.append(t)
            elif t.is_sweep_bit_target:
                new_targets.append(t)
            else:
                raise NotImplementedError(f"{inst=}")
        return new_targets

    det_offset_needed = 0
    for inst in content:
        if inst.name == "REPEAT":
            block = stim.Circuit()
            append_reindexed_content_to_circuit(
                content=inst.body_copy(),
                qubit_i2i=qubit_i2i,
                out_circuit=block,
                rewrite_detector_time_coordinates=rewrite_detector_time_coordinates,
                obs_i2i=obs_i2i,
            )
            out_circuit.append(
                stim.CircuitRepeatBlock(repeat_count=inst.repeat_count, body=block, tag=inst.tag)
            )
        elif inst.name == "QUBIT_COORDS":
            continue
        elif inst.name == "SHIFT_COORDS":
            if rewrite_detector_time_coordinates:
                args = inst.gate_args_copy()
                if len(args) > 2:
                    det_offset_needed -= args[2]
                    out_circuit.append("SHIFT_COORDS", [], [0, 0, args[2]], tag=inst.tag)
            else:
                out_circuit.append(inst)
        elif inst.name == "OBSERVABLE_INCLUDE":
            (obs_index,) = inst.gate_args_copy()
            obs_index = int(round(obs_index))
            obs_index = obs_i2i.get(obs_index, obs_index)
            if obs_index != "discard":
                out_circuit.append(
                    "OBSERVABLE_INCLUDE", rewritten_targets(inst), obs_index, tag=inst.tag
                )
        elif inst.name == "MPAD":
            out_circuit.append(inst)
        elif inst.name == "DETECTOR":
            args = inst.gate_args_copy()
            t = args[2] if len(args) > 2 else 0
            det_offset_needed = max(det_offset_needed, t + 1)
            out_circuit.append(inst)
        else:
            out_circuit.append(
                inst.name, rewritten_targets(inst), inst.gate_args_copy(), tag=inst.tag
            )

    if rewrite_detector_time_coordinates and det_offset_needed > 0:
        out_circuit.append("SHIFT_COORDS", [], (0, 0, det_offset_needed))
