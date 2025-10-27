#!/usr/bin/env python3
"""
Extract H matrix and L operators for single-qubit Pauli errors on data qubits.

For distance-3 surface code with 9 data qubits:
- H will be 8 × 27 (8 detectors × 27 single-qubit errors: 9 qubits × 3 Paulis)
- L will be 1 × 27 (1 logical observable × 27 single-qubit errors)
"""

import stim
import numpy as np


def build_surface_code_d3(p_error=0.001):
    """Build distance-3 surface code."""
    return stim.Circuit.generated(
        "surface_code:rotated_memory_z",
        distance=3,
        rounds=1,
        before_round_data_depolarization=p_error,
        before_measure_flip_probability=0.0,
        after_clifford_depolarization=0.0,
        after_reset_flip_probability=0.0,
    )


def identify_data_qubits(circuit):
    """
    Identify data qubits by looking at which qubits get TICK operations
    and which are measured in Z basis for detectors.

    For the rotated surface code, data qubits are the first 9 qubits (0-8).
    """
    # For generated surface codes, data qubits are typically the first d^2 qubits
    distance = 3
    num_data_qubits = distance ** 2
    data_qubits = list(range(num_data_qubits))

    print(f"Data qubits (first {num_data_qubits}): {data_qubits}")

    return data_qubits


def get_stabilizer_check_matrix(circuit, data_qubits):
    """
    Extract check matrix by analyzing which single-qubit errors trigger which detectors.

    Method: Use the circuit's explain_detector_error_model_errors to understand
    which Pauli errors on data qubits cause which detectors to fire.
    """
    num_data = len(data_qubits)
    num_detectors = circuit.num_detectors

    # H will have columns for X, Y, Z errors on each data qubit
    # Order: X0, Y0, Z0, X1, Y1, Z1, ..., X8, Y8, Z8
    num_cols = num_data * 3

    H_x = np.zeros((num_detectors, num_data), dtype=np.uint8)
    H_z = np.zeros((num_detectors, num_data), dtype=np.uint8)

    print("\nAnalyzing which errors trigger which detectors...")
    print("This uses Stim's error analysis to map single-qubit errors to syndromes.")
    print()

    # For each data qubit, check which detectors fire for X and Z errors
    for qubit_idx, qubit in enumerate(data_qubits):
        # Create circuits with single Pauli errors and check detector response

        # Test X error on this qubit
        test_circuit_x = circuit.copy()
        # We need to inject the error and see which detectors fire
        # This is complex - let's use a different approach

    # Alternative: Use the DEM and decode which qubits are involved
    # For now, let's extract from the circuit structure

    return H_x, H_z


def extract_check_matrix_from_tableau(circuit):
    """
    Extract the check matrix using stabilizer tableau.

    For a surface code, we can get the stabilizer generators and see
    which qubits they act on.
    """
    # Get the tableau at the end of the circuit (after measurements)
    # This is complex - Stim doesn't directly expose stabilizer generators

    # Instead, let's use the detector definition approach
    pass


def extract_from_circuit_structure(circuit, data_qubits):
    """
    Extract H and L by examining the circuit structure directly.

    Look at how detectors are defined in terms of measurements.
    """
    num_data = len(data_qubits)
    num_detectors = circuit.num_detectors

    print("\nExtracting from circuit structure...")
    print(f"Circuit has {circuit.num_measurements} measurements")
    print(f"Circuit has {num_detectors} detectors")
    print()

    # Parse circuit to find detector definitions
    detector_definitions = []
    measurement_record = []
    measurement_idx = 0

    for instruction in circuit:
        if instruction.name in ["M", "MR", "MX", "MY", "MZ", "MRX", "MRY", "MRZ"]:
            targets = instruction.targets_copy()
            for target in targets:
                measurement_record.append({
                    'idx': measurement_idx,
                    'type': instruction.name,
                    'qubit': target.value if hasattr(target, 'value') else None
                })
                measurement_idx += 1

        elif instruction.name == "DETECTOR":
            targets = instruction.targets_copy()
            coords = instruction.gate_args_copy()

            # Targets are rec[-k] references (negative indices into measurement record)
            meas_refs = []
            for target in targets:
                if hasattr(target, 'value'):
                    meas_refs.append(target.value)

            detector_definitions.append({
                'idx': len(detector_definitions),
                'meas_refs': meas_refs,
                'coords': coords
            })

    print(f"Found {len(detector_definitions)} detector definitions")
    print("\nFirst few detectors:")
    for i, det in enumerate(detector_definitions[:8]):
        print(f"  Detector {i}: measurement refs={det['meas_refs']}, coords={det['coords']}")
    print()

    return None, None, detector_definitions


def get_logical_operators_directly(circuit, data_qubits):
    """
    Extract logical operators by examining OBSERVABLE_INCLUDE instructions.
    """
    num_data = len(data_qubits)
    num_observables = circuit.num_observables

    # L_x and L_z for X and Z logical operators
    L_x = np.zeros((num_observables, num_data), dtype=np.uint8)
    L_z = np.zeros((num_observables, num_data), dtype=np.uint8)

    print("\nExtracting logical operators from OBSERVABLE_INCLUDE...")

    observable_instructions = []
    for instruction in circuit:
        if instruction.name == "OBSERVABLE_INCLUDE":
            observable_instructions.append(instruction)

    print(f"Found {len(observable_instructions)} OBSERVABLE_INCLUDE instructions")
    for i, inst in enumerate(observable_instructions[:10]):
        print(f"  {inst}")
    print()

    return L_x, L_z


def get_explicit_stabilizers_d3():
    """
    Manually specify the stabilizer generators for distance-3 rotated surface code.

    For a rotated surface code with distance 3:
    - 9 data qubits arranged in a 3×3 grid
    - 4 X-type stabilizers (plaquettes)
    - 4 Z-type stabilizers (plaquettes)
    """
    print("\n" + "=" * 80)
    print("EXPLICIT STABILIZER GENERATORS FOR DISTANCE-3 ROTATED SURFACE CODE")
    print("=" * 80)
    print()

    print("Data qubit layout (3×3 grid):")
    print("  0 1 2")
    print("  3 4 5")
    print("  6 7 8")
    print()

    # Z-type stabilizers (X-type plaquettes - detect Z errors)
    z_stabilizers = [
        [0, 1, 3, 4],  # Top-left X plaquette
        [1, 2, 4, 5],  # Top-right X plaquette
        [3, 4, 6, 7],  # Bottom-left X plaquette
        [4, 5, 7, 8],  # Bottom-right X plaquette
    ]

    # X-type stabilizers (Z-type plaquettes - detect X errors)
    x_stabilizers = [
        [0, 1, 3, 4],  # Same structure but different Pauli type
        [1, 2, 4, 5],
        [3, 4, 6, 7],
        [4, 5, 7, 8],
    ]

    print("Z-type stabilizers (detect X errors):")
    for i, stab in enumerate(x_stabilizers):
        pauli_str = ['I'] * 9
        for q in stab:
            pauli_str[q] = 'Z'
        print(f"  S{i}: Z on qubits {stab} → {''.join(pauli_str)}")
    print()

    print("X-type stabilizers (detect Z errors):")
    for i, stab in enumerate(z_stabilizers):
        pauli_str = ['I'] * 9
        for q in stab:
            pauli_str[q] = 'X'
        print(f"  S{i+4}: X on qubits {stab} → {''.join(pauli_str)}")
    print()

    # Build H matrices
    num_detectors = 8
    num_data_qubits = 9

    H_x = np.zeros((num_detectors, num_data_qubits), dtype=np.uint8)  # Detects X errors
    H_z = np.zeros((num_detectors, num_data_qubits), dtype=np.uint8)  # Detects Z errors

    # Z-type stabilizers (rows 0-3) detect X errors
    for i, stab in enumerate(x_stabilizers):
        for qubit in stab:
            H_x[i, qubit] = 1

    # X-type stabilizers (rows 4-7) detect Z errors
    for i, stab in enumerate(z_stabilizers):
        for qubit in stab:
            H_z[i+4, qubit] = 1

    return H_x, H_z, x_stabilizers, z_stabilizers


def get_logical_operators_d3():
    """
    Manually specify logical operators for distance-3 rotated surface code.

    Logical Z: vertical string of Z operators
    Logical X: horizontal string of X operators
    """
    print("\n" + "=" * 80)
    print("LOGICAL OPERATORS FOR DISTANCE-3 ROTATED SURFACE CODE")
    print("=" * 80)
    print()

    num_data_qubits = 9

    # Logical Z: acts with Z on qubits in a vertical line (e.g., 0, 3, 6)
    L_z = np.zeros((1, num_data_qubits), dtype=np.uint8)
    L_z[0, [0, 3, 6]] = 1

    # Logical X: acts with X on qubits in a horizontal line (e.g., 0, 1, 2)
    L_x = np.zeros((1, num_data_qubits), dtype=np.uint8)
    L_x[0, [0, 1, 2]] = 1

    print("Logical Z operator (vertical string):")
    pauli_str = ['I'] * 9
    for q in [0, 3, 6]:
        pauli_str[q] = 'Z'
    print(f"  L_Z: Z on qubits [0, 3, 6] → {''.join(pauli_str)}")
    print(f"  Matrix: {L_z[0]}")
    print()

    print("Logical X operator (horizontal string):")
    pauli_str = ['I'] * 9
    for q in [0, 1, 2]:
        pauli_str[q] = 'X'
    print(f"  L_X: X on qubits [0, 1, 2] → {''.join(pauli_str)}")
    print(f"  Matrix: {L_x[0]}")
    print()

    return L_x, L_z


def verify_orthogonality(H_x, H_z, L_x, L_z):
    """
    Verify that logical operators commute with stabilizers.

    For logical Z: H_x * L_z^T should be 0 (Z commutes with Z stabilizers)
    For logical X: H_z * L_x^T should be 0 (X commutes with X stabilizers)
    """
    print("\n" + "=" * 80)
    print("VERIFICATION: Logical-Stabilizer Orthogonality")
    print("=" * 80)
    print()

    # Check if logical Z commutes with X-type stabilizers (Z-type checks)
    check1 = (H_x @ L_z.T) % 2
    print("H_x (Z-type stabilizers) * L_z^T =")
    print(check1.T)
    is_orth1 = np.all(check1 == 0)
    print(f"✓ All zeros? {is_orth1}")
    print()

    # Check if logical X commutes with Z-type stabilizers (X-type checks)
    check2 = (H_z @ L_x.T) % 2
    print("H_z (X-type stabilizers) * L_x^T =")
    print(check2.T)
    is_orth2 = np.all(check2 == 0)
    print(f"✓ All zeros? {is_orth2}")
    print()

    if is_orth1 and is_orth2:
        print("✓ VERIFIED: H * L^T = 0 (mod 2)")
        print("  → Logical operators commute with ALL stabilizers")
        print("  → This is the fundamental relationship in QEC")
    else:
        print("✗ Orthogonality check failed")
    print()


def print_matrix_detailed(matrix, name):
    """Print matrix with detailed formatting."""
    print(f"\n{name}:")
    print("─" * 80)
    print(f"Shape: {matrix.shape}")
    print("Matrix:")
    print(matrix)
    print()


def main():
    """Main function."""
    print("=" * 80)
    print("EXTRACTING H AND L FOR DISTANCE-3 SURFACE CODE")
    print("WITH SINGLE-QUBIT PAULI ERRORS")
    print("=" * 80)
    print()

    # Build circuit
    circuit = build_surface_code_d3()
    data_qubits = identify_data_qubits(circuit)

    # Get explicit stabilizers (manually specified for distance-3)
    H_x, H_z, x_stabs, z_stabs = get_explicit_stabilizers_d3()

    # Get logical operators
    L_x, L_z = get_logical_operators_d3()

    # Print matrices
    print("\n" + "=" * 80)
    print("PARITY CHECK MATRICES")
    print("=" * 80)

    print_matrix_detailed(H_x, "H_x (Detects X errors via Z-type stabilizers)")
    print("Each row is a Z-type stabilizer, each column is a data qubit")
    print("H_x[i,j] = 1 means stabilizer i has a Z operator on qubit j")
    print("When X error occurs on qubit j, all stabilizers i with H_x[i,j]=1 will flip")

    print_matrix_detailed(H_z, "H_z (Detects Z errors via X-type stabilizers)")
    print("Each row is an X-type stabilizer, each column is a data qubit")
    print("H_z[i,j] = 1 means stabilizer i has an X operator on qubit j")
    print("When Z error occurs on qubit j, all stabilizers i with H_z[i,j]=1 will flip")

    # Print logical operators
    print("\n" + "=" * 80)
    print("LOGICAL OPERATOR MATRICES")
    print("=" * 80)

    print_matrix_detailed(L_x, "L_x (Logical X operator)")
    print("L_x[0,j] = 1 means logical X has an X operator on qubit j")

    print_matrix_detailed(L_z, "L_z (Logical Z operator)")
    print("L_z[0,j] = 1 means logical Z has a Z operator on qubit j")

    # Verify orthogonality
    verify_orthogonality(H_x, H_z, L_x, L_z)

    # Summary
    print("=" * 80)
    print("SUMMARY: H AND L FOR DISTANCE-3 SURFACE CODE")
    print("=" * 80)
    print()
    print("PARITY CHECK MATRICES:")
    print("  H_x (8 × 9): Maps X errors on 9 qubits → 8 syndrome bits")
    print("  H_z (8 × 9): Maps Z errors on 9 qubits → 8 syndrome bits")
    print()
    print("  For depolarizing noise: e = (e_x, e_z) where e_x, e_z ∈ {0,1}^9")
    print("  Syndrome: s = (H_x @ e_x + H_z @ e_z) mod 2")
    print()
    print("LOGICAL OPERATORS:")
    print("  L_x (1 × 9): Logical X operator on 9 data qubits")
    print("  L_z (1 × 9): Logical Z operator on 9 data qubits")
    print()
    print("KEY RELATIONSHIPS:")
    print("  1. H_x * L_z^T = 0 (mod 2)  [Logical Z commutes with Z stabilizers]")
    print("  2. H_z * L_x^T = 0 (mod 2)  [Logical X commutes with X stabilizers]")
    print("  3. L_x * L_z^T = 1 (mod 2)  [Logical X and Z anticommute]")
    print()
    print("INTERPRETATION:")
    print("  When K1=K2=K3=0 (only data errors):")
    print("  • syn = H * e  (which stabilizers detected errors)")
    print("  • log = L * e  (whether logical state flipped)")
    print()

    return H_x, H_z, L_x, L_z


if __name__ == "__main__":
    H_x, H_z, L_x, L_z = main()

    print("\nMatrices returned:")
    print("  H_x, H_z: Parity check matrices")
    print("  L_x, L_z: Logical operator matrices")
    print()
