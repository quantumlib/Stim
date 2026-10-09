from typing import Any, cast

import numpy as np
import stim  # type: ignore[import-untyped]

from stimside.util.numpy_types import Bool1DArray


def compute_known_measurements(circuit) -> list[int | None]:
    """Make a sample describing each deterministic measurement outcome.

    Basically, like a reference sample, except for non-deterministic measurements,
    which are registered as None instead of receiving any valid value.
    """
    # just straight measurement sample the circuit:
    # if the measurement is deterministic, it'll be deterministic,
    # otherwise, it'll be 50:50.
    reference_measurement_sampler = circuit.without_noise().compile_sampler()

    reference_samples = reference_measurement_sampler.sample(
        shots=128, bit_packed=False
    )
    # shape is (shots, #measurements)
    # shots sets how likely a spurious failure is
    # the likelihood of a 50:50 process flipping N heads in a row is (1/2)**N
    # at N=128 (which is the AVX instruction width), P=3E-39
    # i.e. not going to happen to you

    determined = np.all(reference_samples == reference_samples[0], axis=0)
    # determined has shape (#measurements)

    # np.where does `if determined, copy form reference_sample, else None`
    determined_sample = np.where(determined, reference_samples[0], cast(Any, None))

    return list(determined_sample)


# State encoding in uint8:
# 0: _ (unknown)
# 1: +X, 2: -X
# 3: +Y, 4: -Y
# 5: +Z, 6: -Z
_CODE_TO_PAULI: tuple[stim.PauliString, ...] = (
    stim.PauliString("_"),
    stim.PauliString("+X"),
    stim.PauliString("-X"),
    stim.PauliString("+Y"),
    stim.PauliString("-Y"),
    stim.PauliString("+Z"),
    stim.PauliString("-Z"),
)

_STR_TO_CODE: dict[str, int] = {
    "_": 0,
    "+_": 0,
    "-_": 0,
    "+X": 1,
    "-X": 2,
    "+Y": 3,
    "-Y": 4,
    "+Z": 5,
    "-Z": 6,
}

_GATE_FWD_LUT: dict[str, np.ndarray] = {}
_GATE_BWD_LUT: dict[str, np.ndarray] = {}


def _get_gate_luts(op_name: str) -> tuple[np.ndarray, np.ndarray]:
    if op_name in _GATE_FWD_LUT:
        return _GATE_FWD_LUT[op_name], _GATE_BWD_LUT[op_name]
    fwd = np.zeros(7, dtype=np.uint8)
    bwd = np.zeros(7, dtype=np.uint8)
    inst = stim.CircuitInstruction(name=op_name, targets=[0])
    for code in range(1, 7):
        ps = _CODE_TO_PAULI[code]
        fwd[code] = _STR_TO_CODE[str(ps.after(inst))]
        bwd[code] = _STR_TO_CODE[str(ps.before(inst))]
    _GATE_FWD_LUT[op_name] = fwd
    _GATE_BWD_LUT[op_name] = bwd
    return fwd, bwd


def _unroll_circuit(circuit: stim.Circuit) -> list[stim.CircuitInstruction]:
    """Unroll REPEAT blocks into a flat list of CircuitInstructions without
    stripping SHIFT_COORDS or fusing adjacent gates across loop boundaries.
    """
    ops: list[stim.CircuitInstruction] = []
    for item in circuit:
        if isinstance(item, stim.CircuitRepeatBlock):
            body_ops = _unroll_circuit(item.body_copy())
            ops.extend(body_ops * item.repeat_count)
        else:
            ops.append(item)
    return ops


def compute_known_states_uint8(
    circuit: stim.Circuit,
    unrolled_ops: list[stim.CircuitInstruction] | None = None,
) -> np.ndarray:
    """Compute known states as a fast (num_ops + 1, num_qubits) uint8 array.

    Codes:
        0: _ (unknown)
        1: +X, 2: -X
        3: +Y, 4: -Y
        5: +Z, 6: -Z
    """
    known_measurements = compute_known_measurements(circuit)
    if unrolled_ops is None:
        unrolled_ops = _unroll_circuit(circuit)

    num_ops = len(unrolled_ops)
    num_qubits = circuit.num_qubits
    states_uint8 = np.empty((num_ops + 1, num_qubits), dtype=np.uint8)
    states_uint8[0, :] = 5  # All qubits start in +Z (|0>)

    measurements_consumed = 0

    for i, op in enumerate(unrolled_ops):
        states_uint8[i + 1] = states_uint8[i]
        gd = stim.gate_data(op.name)

        # Annotations (QUBIT_COORDS, SHIFT_COORDS, DETECTOR, OBSERVABLE_INCLUDE, TICK, etc.)
        if not (
            gd.is_unitary or gd.is_reset or gd.produces_measurements or gd.is_noisy_gate
        ):
            continue

        # Pure noise instructions (e.g. X_ERROR, DEPOLARIZE1, II_ERROR, CORRELATED_ERROR)
        if gd.is_noisy_gate and not gd.produces_measurements:
            continue

        raw_targets = op.targets_copy()
        if op.name == "MPAD":
            measurements_consumed += len(raw_targets)
            continue
        q_targets = np.fromiter(
            (t.qubit_value for t in raw_targets if t.qubit_value is not None),
            dtype=np.intp,
        )
        if len(set(q_targets.tolist())) != len(q_targets):
            raise ValueError(
                f"Can't compute known states for an operation with multiple uses of the same target: {op}"
            )

        if gd.is_reset:
            # R, RX, RY, RZ, MR, MRX, MRY, MRZ
            if op.name[-1] == "X":
                reset_code = 1
            elif op.name[-1] == "Y":
                reset_code = 3
            else:
                reset_code = 5
            states_uint8[i + 1, q_targets] = reset_code
            if gd.produces_measurements:
                measurements_consumed += len(raw_targets)

        elif gd.produces_measurements:
            if op.name in ("M", "MZ", "MX", "MY"):
                if op.name[-1] == "X":
                    base_code = 1
                elif op.name[-1] == "Y":
                    base_code = 3
                else:
                    base_code = 5

                for t in raw_targets:
                    this_measurement = known_measurements[measurements_consumed]
                    measurements_consumed += 1
                    q = t.qubit_value
                    if this_measurement is not None:
                        actual_bit = bool(this_measurement) ^ bool(
                            t.is_inverted_result_target
                        )
                        states_uint8[i + 1, q] = base_code + (1 if actual_bit else 0)
                    else:
                        states_uint8[i + 1, q] = 0
            else:
                measurements_consumed += int(
                    len(raw_targets) / (2 if gd.is_two_qubit_gate else 1)
                )
                if op.name not in (
                    "MPAD",
                    "HERALDED_ERASE",
                    "HERALDED_PAULI_CHANNEL_1",
                ):
                    states_uint8[i + 1, q_targets] = 0

        elif gd.is_single_qubit_gate and gd.is_unitary:
            fwd_lut, _ = _get_gate_luts(op.name)
            states_uint8[i + 1, q_targets] = fwd_lut[states_uint8[i + 1, q_targets]]

        else:
            states_uint8[i + 1, q_targets] = 0

    # Backward pass: propagate known states backwards from deterministic measurements
    measurements_consumed = len(known_measurements) - 1
    for i in range(num_ops - 1, -1, -1):
        op = unrolled_ops[i]
        gd = stim.gate_data(op.name)
        new_known_state = states_uint8[i + 1].copy()

        if not (
            gd.is_unitary or gd.is_reset or gd.produces_measurements or gd.is_noisy_gate
        ):
            pass
        elif gd.is_noisy_gate and not gd.produces_measurements:
            pass
        elif gd.is_reset and not gd.produces_measurements:
            raw_targets = op.targets_copy()
            q_targets = np.fromiter(
                (t.qubit_value for t in raw_targets if t.qubit_value is not None),
                dtype=np.intp,
            )
            new_known_state[q_targets] = 0
        elif gd.produces_measurements:
            raw_targets = op.targets_copy()
            if op.name in ("M", "MZ", "MX", "MY", "MR", "MRX", "MRY", "MRZ"):
                if op.name[-1] == "X":
                    base_code = 1
                elif op.name[-1] == "Y":
                    base_code = 3
                else:
                    base_code = 5

                for t in raw_targets[::-1]:
                    this_measurement = known_measurements[measurements_consumed]
                    measurements_consumed -= 1
                    q = t.qubit_value
                    if this_measurement is not None:
                        actual_bit = bool(this_measurement) ^ bool(
                            t.is_inverted_result_target
                        )
                        new_known_state[q] = base_code + (1 if actual_bit else 0)
                    else:
                        new_known_state[q] = 0
            else:
                measurements_consumed -= int(
                    len(raw_targets) / (2 if gd.is_two_qubit_gate else 1)
                )
                if op.name not in (
                    "MPAD",
                    "HERALDED_ERASE",
                    "HERALDED_PAULI_CHANNEL_1",
                ):
                    q_targets = np.fromiter(
                        (
                            t.qubit_value
                            for t in raw_targets
                            if t.qubit_value is not None
                        ),
                        dtype=np.intp,
                    )
                    new_known_state[q_targets] = 0
        elif gd.is_single_qubit_gate and gd.is_unitary:
            raw_targets = op.targets_copy()
            q_targets = np.fromiter(
                (t.qubit_value for t in raw_targets if t.qubit_value is not None),
                dtype=np.intp,
            )
            _, bwd_lut = _get_gate_luts(op.name)
            new_known_state[q_targets] = bwd_lut[new_known_state[q_targets]]
        else:
            raw_targets = op.targets_copy()
            q_targets = np.fromiter(
                (t.qubit_value for t in raw_targets if t.qubit_value is not None),
                dtype=np.intp,
            )
            new_known_state[q_targets] = 0

        already_known = states_uint8[i]
        conflict = (
            (new_known_state != 0)
            & (already_known != 0)
            & (already_known != new_known_state)
        )
        if np.any(conflict):
            raise ValueError(
                "Backprop known states disagreed with forward prop known states. "
                "This is on us, file a bug. "
                f"Across instruction {i}: {op} "
            )
        fill_mask = (already_known == 0) & (new_known_state != 0)
        states_uint8[i, fill_mask] = new_known_state[fill_mask]

    return states_uint8


def compute_known_states(circuit) -> list[list[stim.PauliString]]:
    """Find circuit locations with known single qubit states."""
    states_uint8 = compute_known_states_uint8(circuit)
    return [
        [_CODE_TO_PAULI[int(code)].copy() for code in row] for row in states_uint8
    ]


def convert_paulis_to_arrays(
    known_states: list[stim.PauliString] | np.ndarray,
) -> tuple[
    Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray
]:
    """convert a list of known state PauliStrings into a dense matrix representation for fast
    lookups.
    """
    if isinstance(known_states, np.ndarray):
        return (
            known_states == 1,
            known_states == 2,
            known_states == 3,
            known_states == 4,
            known_states == 5,
            known_states == 6,
        )
    num_qubits = len(known_states)
    codes = np.fromiter(
        (_STR_TO_CODE.get(str(ps), 0) for ps in known_states),
        dtype=np.uint8,
        count=num_qubits,
    )
    return (
        codes == 1,
        codes == 2,
        codes == 3,
        codes == 4,
        codes == 5,
        codes == 6,
    )


def print_known_states_and_circuit(known_states, circuit):
    for i, op in enumerate(_unroll_circuit(circuit)):
        print("\t", [str(q) for q in known_states[i]])
        print(i, ":", op)
        print("\t", [str(q) for q in known_states[i + 1]])

