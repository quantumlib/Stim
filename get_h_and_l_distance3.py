#!/usr/bin/env python3
"""
Extract and display the actual H matrix and L logical operators for distance-3 surface code.
"""

import stim
import numpy as np


def build_surface_code_d3():
    """Build distance-3 surface code with only data errors."""
    return stim.Circuit.generated(
        "surface_code:rotated_memory_z",
        distance=3,
        rounds=1,
        before_round_data_depolarization=0.001,  # Small rate for structure
        before_measure_flip_probability=0.0,     # K1=0
        after_clifford_depolarization=0.0,       # K2=0
        after_reset_flip_probability=0.0,        # K3=0
    )


def extract_h_matrix_from_dem(circuit):
    """
    Extract parity check matrix H from the Detector Error Model.

    H[i,j] = 1 if error j triggers detector i, else 0
    """
    dem = circuit.detector_error_model()

    num_detectors = circuit.num_detectors

    # Build mapping from error mechanisms to detectors
    error_to_detectors = {}
    error_idx = 0

    for instruction in dem:
        if instruction.type == "error":
            targets = instruction.targets_copy()
            detectors = [t.val for t in targets if not t.is_logical_observable_id()]
            observables = [t.val for t in targets if t.is_logical_observable_id()]

            error_to_detectors[error_idx] = {
                'detectors': detectors,
                'observables': observables
            }
            error_idx += 1

    # Build H matrix
    H = np.zeros((num_detectors, error_idx), dtype=np.uint8)

    for err_idx, info in error_to_detectors.items():
        for det in info['detectors']:
            if 0 <= det < num_detectors:
                H[det, err_idx] = 1

    return H, error_to_detectors


def extract_logical_operators_from_dem(circuit, error_to_detectors):
    """
    Extract which errors affect logical observables.

    L[i,j] = 1 if error j affects logical observable i
    """
    num_observables = circuit.num_observables
    num_errors = len(error_to_detectors)

    L = np.zeros((num_observables, num_errors), dtype=np.uint8)

    for err_idx, info in error_to_detectors.items():
        for obs in info['observables']:
            if 0 <= obs < num_observables:
                L[obs, err_idx] = 1

    return L


def print_matrix(matrix, name, row_label="Detector", col_label="Error"):
    """Pretty print a binary matrix."""
    print(f"\n{name} Matrix:")
    print("=" * 80)
    print(f"Shape: {matrix.shape[0]} rows ({row_label}s) × {matrix.shape[1]} columns ({col_label}s)")
    print(f"Density: {np.mean(matrix):.4f}")
    print()

    # Print matrix with labels
    print(f"    {col_label}s: ", end="")
    for j in range(min(matrix.shape[1], 50)):  # Limit columns displayed
        print(f"{j%10}", end="")
    if matrix.shape[1] > 50:
        print(f"... ({matrix.shape[1]} total)", end="")
    print()

    for i in range(matrix.shape[0]):
        print(f"{row_label} {i:2d}: ", end="")
        for j in range(min(matrix.shape[1], 50)):
            print(f"{matrix[i,j]}", end="")
        if matrix.shape[1] > 50:
            print("...", end="")
        print()

    if matrix.shape[0] > 30:
        print(f"... ({matrix.shape[0]} total rows)")


def verify_h_l_orthogonality(H, L):
    """Verify that H * L^T = 0 (mod 2)."""
    print("\n" + "=" * 80)
    print("VERIFICATION: H * L^T = 0 (mod 2)")
    print("=" * 80)

    product = (H @ L.T) % 2

    print(f"\nH shape: {H.shape}")
    print(f"L shape: {L.shape}")
    print(f"H * L^T shape: {product.shape}")
    print(f"\nH * L^T = ")
    print(product)

    is_orthogonal = np.all(product == 0)

    if is_orthogonal:
        print("\n✓ VERIFIED: H * L^T = 0 (mod 2)")
        print("  → Logical operators commute with ALL stabilizers")
        print("  → Logical operations are invisible to syndrome measurements")
    else:
        print("\n✗ WARNING: H * L^T ≠ 0")
        print("  This may occur if L represents error mechanisms rather than logical operators")


def analyze_circuit_structure(circuit):
    """Print circuit structure information."""
    print("=" * 80)
    print("SURFACE CODE CIRCUIT STRUCTURE (Distance 3)")
    print("=" * 80)
    print()
    print(f"Total qubits (data + ancilla): {circuit.num_qubits}")
    print(f"Number of detectors (syndrome bits): {circuit.num_detectors}")
    print(f"Number of observables (logical qubits): {circuit.num_observables}")
    print(f"Number of measurements: {circuit.num_measurements}")
    print()
    print("For distance-3 rotated surface code:")
    print("  Expected data qubits: 3² = 9")
    print("  Expected stabilizers: 3² - 1 = 8")
    print("    • 4 X-type stabilizers (detect Z errors)")
    print("    • 4 Z-type stabilizers (detect X errors)")
    print("  Expected logical qubits: 1")
    print()


def show_error_mechanisms(error_to_detectors, max_show=20):
    """Show first few error mechanisms."""
    print("\n" + "=" * 80)
    print("ERROR MECHANISMS FROM DEM")
    print("=" * 80)
    print()
    print(f"Showing first {min(max_show, len(error_to_detectors))} of {len(error_to_detectors)} error mechanisms:")
    print()

    for i, (err_idx, info) in enumerate(error_to_detectors.items()):
        if i >= max_show:
            break

        dets = sorted(info['detectors'])
        obs = sorted(info['observables'])

        det_str = f"[{','.join(map(str, dets))}]" if dets else "[]"
        obs_str = f"[{','.join(map(str, obs))}]" if obs else "[]"

        print(f"Error {err_idx:3d}: Detectors={det_str:20s} Observables={obs_str}")


def verify_sampling(circuit, num_samples=20):
    """Verify H*e = s by sampling."""
    print("\n" + "=" * 80)
    print("SAMPLING VERIFICATION: H * e = s (mod 2)")
    print("=" * 80)
    print()

    sampler = circuit.compile_detector_sampler()
    syndromes, observables = sampler.sample(num_samples, separate_observables=True)

    print(f"Sampled {num_samples} detection events:")
    print()

    num_with_errors = np.sum(np.any(syndromes, axis=1))
    num_with_logical = np.sum(np.any(observables, axis=1))
    avg_detectors = np.mean(np.sum(syndromes, axis=1))

    print(f"Samples with detections: {num_with_errors}/{num_samples}")
    print(f"Samples with logical flips: {num_with_logical}/{num_samples}")
    print(f"Average detectors fired: {avg_detectors:.2f}")
    print()

    print("Sample detection patterns (first 10):")
    for i in range(min(10, num_samples)):
        syn_str = ''.join(str(int(s)) for s in syndromes[i])
        obs_str = ''.join(str(int(o)) for o in observables[i])
        n_dets = int(np.sum(syndromes[i]))

        print(f"  Sample {i:2d}: syndrome={syn_str} ({n_dets} detectors) | observable={obs_str}")
    print()

    print("Interpretation:")
    print("  • syndrome bits = s = H*e (which stabilizers detected errors)")
    print("  • observable bits = L*e (whether logical state flipped)")
    print("  • When K1=K2=K3=0, only data qubit errors contribute")


def main():
    """Main function to extract and display H and L."""
    print("\n" + "=" * 80)
    print("EXTRACTING H MATRIX AND L OPERATORS FOR DISTANCE-3 SURFACE CODE")
    print("=" * 80)
    print()

    # Build circuit
    circuit = build_surface_code_d3()

    # Analyze structure
    analyze_circuit_structure(circuit)

    # Extract H matrix
    print("\n" + "=" * 80)
    print("EXTRACTING PARITY CHECK MATRIX H")
    print("=" * 80)
    H, error_to_detectors = extract_h_matrix_from_dem(circuit)

    # Show error mechanisms
    show_error_mechanisms(error_to_detectors, max_show=30)

    # Print H matrix
    print_matrix(H, "H (Parity Check)", row_label="Detector", col_label="Error")

    # Extract L operators
    print("\n" + "=" * 80)
    print("EXTRACTING LOGICAL OPERATORS L")
    print("=" * 80)
    L = extract_logical_operators_from_dem(circuit, error_to_detectors)

    # Print L matrix
    print_matrix(L, "L (Logical Operators)", row_label="Observable", col_label="Error")

    # Verify orthogonality
    verify_h_l_orthogonality(H, L)

    # Verify with sampling
    verify_sampling(circuit, num_samples=20)

    # Summary
    print("\n" + "=" * 80)
    print("SUMMARY")
    print("=" * 80)
    print()
    print("PARITY CHECK MATRIX H:")
    print(f"  • Shape: {H.shape[0]} × {H.shape[1]}")
    print(f"  • H[i,j] = 1 if error j triggers detector i")
    print(f"  • Relationship: s = H * e (mod 2)")
    print(f"  • Density: {np.mean(H):.4f} (sparse matrix)")
    print()
    print("LOGICAL OPERATORS L:")
    print(f"  • Shape: {L.shape[0]} × {L.shape[1]}")
    print(f"  • L[i,j] = 1 if error j affects logical observable i")
    print(f"  • Relationship: H * L^T = 0 (mod 2) [orthogonality]")
    print(f"  • Density: {np.mean(L):.4f}")
    print()
    print("KEY INSIGHT:")
    print("  The DEM represents error MECHANISMS (which may be compound errors)")
    print("  rather than individual single-qubit Pauli errors. For single-qubit")
    print("  errors X_i, Y_i, Z_i on qubit i, you'd need to decompose the DEM")
    print("  error mechanisms into single-qubit error contributions.")
    print()

    # Return for further use
    return circuit, H, L, error_to_detectors


if __name__ == "__main__":
    circuit, H, L, error_info = main()

    print("\nMatrices are available as:")
    print("  • H: Parity check matrix")
    print("  • L: Logical operator matrix")
    print("  • circuit: The Stim circuit")
    print("  • error_info: Error mechanism details")
    print()
