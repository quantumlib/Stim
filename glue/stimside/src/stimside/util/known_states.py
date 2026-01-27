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


def compute_known_states(circuit):
    """Find circuit locations with known single qubit states."""

    known_measurements = compute_known_measurements(circuit)
    measurements_consumed = 0

    known_states: list[list[stim.PauliString]] = []

    known_states.append([stim.PauliString("+Z")] * circuit.num_qubits)
    # In stim, all qubits start in the 0 state (+Z)

    # we flatten the circuit
    # this is a brain-saving rather than time-saving measure,
    # in the future, it would be better to do something recursive,
    # and change known_states datastructures into something that saves
    # space when a repeat block is actually repetitive.
    # This does open the door for an annoying bug:
    # stim naturally combines gates that are compatible, eg H 0\nH 0 --> H 0 0
    # this code does not currently handle that case, and raises an error
    # if the user doesn't terminate their loop with a TICK, they might get a confusing error message
    for op in circuit.flattened():

        gd = stim.gate_data(op.name)
        new_known_state = [ps.copy() for ps in known_states[-1]]

        targets = [t.value for t in op.targets_copy()]
        if len(set(targets)) != len(targets):
            raise ValueError(
                f"Can't compute known states for an operation with multiple uses of the same target: {op}"
            )

        if gd.is_reset:
            #  R, RX, RY, RZ, MR, MRX, MRY, MRZ
            pauli = "Z"
            if op.name[-1] in ["X", "Y"]:
                pauli = op.name[-1]
            for t in op.targets_copy():
                new_known_state[t.value] = stim.PauliString(pauli)

        elif gd.produces_measurements:
            #  M, MX, MY, MZ
            #  MPP, MXX, MYY, MZZ,
            #  MPAD, HERALDED_ERASE, HERALDED_PAULI_CHANNEL_1,

            if op.name in ["M", "MZ", "MX", "MY"]:
                pauli = "Z"
                if op.name[-1] in ["X", "Y"]:
                    pauli = op.name[-1]

                for t in targets:
                    this_measurement = known_measurements[measurements_consumed]
                    measurements_consumed += 1
                    if this_measurement is not None:
                        new_known_state[t] = (
                            stim.PauliString(pauli) * (-1) ** this_measurement
                        )
                    else:
                        new_known_state[t] = stim.PauliString("_")

            else:
                # consume the measurements records that don't tell us a qubit state
                measurements_consumed += int(
                    len(op.targets_copy()) / (2 if gd.is_two_qubit_gate else 1)
                )
                # 2Q measurements scramble the qubits, MPAD and errors don't
                if op.name not in [
                    "MPAD",
                    "HERALDED_ERASE",
                    "HERALDED_PAULI_CHANNEL_1",
                ]:
                    # scamble the qubits
                    for t in targets:
                        new_known_state[t] = stim.PauliString("_")

        elif gd.is_noisy_gate:
            # basically all error instructions
            # stim also considers measurements to be noisy, but we've already handled those
            # just leave the known state as is
            pass

        elif gd.is_single_qubit_gate and gd.is_unitary:
            # Basically, any single qubit clifford
            # actually propagate the known state through
            targets = [t.value for t in op.targets_copy()]
            for t, ps in enumerate(known_states[-1]):
                if ps[0] != 0 and t in targets:  # if not I
                    new_known_state[t] = ps.after(
                        stim.CircuitInstruction(
                            name=op.name, targets=[0], gate_args=op.gate_args_copy()
                        )
                    )

        else:  # all other gates scramble what they touch
            targets = [t.value for t in op.targets_copy()]
            for t in targets:
                new_known_state[t] = stim.PauliString("_")

        known_states.append(new_known_state)

    # we now iterate backwards through the circuit:
    # to push known-ness backwards from deterministic measurements.
    # we compute what we know from backproping, and then compare it to what we already know in forward prop

    measurements_consumed = len(known_measurements) - 1
    for i, op in list(enumerate(circuit.flattened()))[::-1]:
        # known_states[i] is the state right _before_ the operation circuit[i]
        # known_states[i+1] is the state right _after_ the operation circuit[i]
        # len(known_states) is len(circuit)+1,
        # so accessing known_states[i] and known_states[i+1] is always fine

        gd = stim.gate_data(op.name)
        new_known_state = [ps.copy() for ps in known_states[i + 1]]

        if gd.is_reset and not gd.produces_measurements:
            #  R, RX, RY, RZ,
            pauli = "Z"
            if op.name[-1] in ["X", "Y"]:
                pauli = op.name[-1]
            for t in op.targets_copy():
                new_known_state[t.value] = stim.PauliString("_")

        elif gd.produces_measurements:
            #  M, MX, MY, MZ
            #  MR, MRX, MRY, MRZ
            #  MPP, MXX, MYY, MZZ,
            #  MPAD, HERALDED_ERASE, HERALDED_PAULI_CHANNEL_1,

            if op.name in ["M", "MZ", "MX", "MY", "MR", "MRX", "MRY", "MRZ"]:
                pauli = "Z"
                if op.name[-1] in ["X", "Y"]:
                    pauli = op.name[-1]

                targets = [t.value for t in op.targets_copy()]
                for t in targets[::-1]:
                    this_measurement = known_measurements[measurements_consumed]
                    measurements_consumed -= 1
                    if this_measurement is not None:
                        new_known_state[t] = (
                            stim.PauliString(pauli) * (-1) ** this_measurement
                        )
                    else:
                        new_known_state[t] = stim.PauliString("_")

            else:
                # consume the measurements records that don't tell us a qubit state
                measurements_consumed -= int(
                    len(op.targets_copy()) / (2 if gd.is_two_qubit_gate else 1)
                )
                # 2Q measurements scramble the qubits, MPAD and errors don't
                if op.name not in [
                    "MPAD",
                    "HERALDED_ERASE",
                    "HERALDED_PAULI_CHANNEL_1",
                ]:
                    # scamble the qubits
                    targets = [t.value for t in op.targets_copy()]
                    for t in targets:
                        new_known_state[t] = stim.PauliString("_")

        elif gd.is_noisy_gate:
            # basically all error instructions
            # stim also considers measurements to be noisy, but we've already handled those
            # just leave the known state as is
            pass

        elif gd.is_single_qubit_gate and gd.is_unitary:
            # Basically, any single qubit clifford
            # actually propagate the known state through
            targets = [t.value for t in op.targets_copy()]
            for t, ps in enumerate(known_states[i + 1]):
                if ps[0] != 0 and t in targets:  # if not I
                    new_known_state[t] = ps.before(
                        stim.CircuitInstruction(
                            name=op.name, targets=[0], gate_args=op.gate_args_copy()
                        )
                    )

        else:  # all other gates scramble what they touch
            targets = [t.value for t in op.targets_copy()]
            for t in targets:
                new_known_state[t] = stim.PauliString("_")

        # now we combine new_known_state from backproping
        # with the already known known_states[i-1]
        already_known_state = [ps.copy() for ps in known_states[i]]
        for q, (a, b) in enumerate(zip(already_known_state, new_known_state)):
            if b[0] != 0 and a != b:
                if a[0] == 0:
                    known_states[i][q] = b
                else:
                    raise ValueError(
                        "Backprop known states disagreed with forward prop known states. "
                        "This is on us, file a bug. "
                        f"Across instruction {i}: {circuit[i]} "
                        f"State after gate {known_states[i+1]}, "
                        f"implied state before gate {new_known_state}, "
                        f"disagreed with known state before gate {known_states[i]}, "
                    )

    return known_states


def convert_paulis_to_arrays(
    known_states: list[stim.PauliString],
) -> tuple[
    Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray, Bool1DArray
]:
    """convert a list of known state PauliStrings into a dense metrix representation for fast
    lookups.

    Args:
        known_states: a list of length num_qubits
        of length-1 PauliStrings indicating the known qubit state.

    Returns:
        Returns a set of bool arrays of shape (num_qubits, )
        In order these are:
            in_pX: True if qubit is in +X
            in_mX: True if qubit is in -X
            in_pY: True if qubit is in +Y
            in_mY: True if qubit is in -Y
            in_pZ: True if qubit is in +Z
            in_mZ: True if qubit is in -Z
    """
    num_qubits = len(known_states)
    in_pX = np.zeros(num_qubits, dtype=bool)
    in_mX = np.zeros(num_qubits, dtype=bool)
    in_pY = np.zeros(num_qubits, dtype=bool)
    in_mY = np.zeros(num_qubits, dtype=bool)
    in_pZ = np.zeros(num_qubits, dtype=bool)
    in_mZ = np.zeros(num_qubits, dtype=bool)
    for i, ps in enumerate(known_states):
        if ps == stim.PauliString("+X"):
            in_pX[i] = True
        elif ps == stim.PauliString("-X"):
            in_mX[i] = True
        elif ps == stim.PauliString("+Y"):
            in_pY[i] = True
        elif ps == stim.PauliString("-Y"):
            in_mY[i] = True
        elif ps == stim.PauliString("+Z"):
            in_pZ[i] = True
        elif ps == stim.PauliString("-Z"):
            in_mZ[i] = True
    return in_pX, in_mX, in_pY, in_mY, in_pZ, in_mZ


def print_known_states_and_circuit(known_states, circuit):
    for i, op in enumerate(circuit):
        print("\t", [str(q) for q in known_states[i]])
        print(i, ":", op)
        print("\t", [str(q) for q in known_states[i + 1]])
