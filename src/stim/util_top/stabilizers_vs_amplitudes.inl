#include "stim/util_bot/twiddle.h"
#include "stim/util_top/circuit_inverse_unitary.h"
#include "stim/util_top/circuit_vs_amplitudes.h"
#include "stim/util_top/circuit_vs_tableau.h"

namespace stim {

template <size_t W>
std::vector<std::vector<std::complex<float>>> tableau_to_unitary(const Tableau<W> &tableau, bool little_endian) {
    auto flat = tableau.to_flat_unitary_matrix(little_endian);
    std::vector<std::vector<std::complex<float>>> result;
    size_t n = 1 << tableau.num_qubits;
    for (size_t row = 0; row < n; row++) {
        result.push_back({});
        auto &back = result.back();
        std::complex<float> *start = &flat[row * n];
        back.insert(back.end(), start, start + n);
    }
    return result;
}

template <size_t W>
Tableau<W> unitary_to_tableau(const std::vector<std::vector<std::complex<float>>> &matrix, bool little_endian) {
    // Verify matrix is square.
    size_t num_amplitudes = matrix.size();
    if (!is_power_of_2(num_amplitudes)) {
        throw std::invalid_argument(
            "Matrix width and height must be a power of 2. Height was " + std::to_string(num_amplitudes));
    }
    for (size_t r = 0; r < num_amplitudes; r++) {
        if (matrix[r].size() != num_amplitudes) {
            std::stringstream ss;
            ss << "Matrix must be square, but row " << r;
            ss << " had width " << matrix[r].size();
            ss << " while matrix had height " << num_amplitudes;
            throw std::invalid_argument(ss.str());
        }
    }

    // Use first column to solve how to get out of superposition and to a phased permutation.
    std::vector<std::complex<float>> first_col;
    for (const auto &row : matrix) {
        first_col.push_back(row[0]);
    }
    Circuit recorded_circuit = stabilizer_state_vector_to_circuit(first_col, true);
    recorded_circuit = circuit_inverse_unitary(recorded_circuit);

    // Use the state channel duality to get the operation into the vector simulator.
    VectorSimulator sim(0);
    float m2v = sqrtf(num_amplitudes);
    sim.state.clear();
    sim.state.reserve(num_amplitudes * num_amplitudes);
    for (size_t r = 0; r < num_amplitudes; r++) {
        for (size_t c = 0; c < num_amplitudes; c++) {
            sim.state.push_back(matrix[c][r] / m2v);
        }
    }
    // Convert to a phased permutation (assuming the matrix was Clifford).
    sim.do_unitary_circuit(recorded_circuit);
    sim.smooth_stabilizer_state(sim.state[0]);

    auto apply = [&](GateType gate_type, uint32_t target) {
        sim.apply(gate_type, target);
        recorded_circuit.safe_append(
            CircuitInstruction(gate_type, {}, std::vector<GateTarget>{GateTarget::qubit(target)}, ""));
    };
    auto apply2 = [&](GateType gate_type, uint32_t target, uint32_t target2) {
        sim.apply(gate_type, target, target2);
        recorded_circuit.safe_append(CircuitInstruction(
            gate_type, {}, std::vector<GateTarget>{GateTarget::qubit(target), GateTarget::qubit(target2)}, ""));
    };

    // Undo the permutation and also single-qubit phases.
    size_t num_qubits = floor_lg2(num_amplitudes);
    for (size_t q = 0; q < num_qubits; q++) {
        size_t c = 1 << q;

        // Find the single entry in the column and move it to the diagonal.
        for (size_t r = 0; r < num_amplitudes; r++) {
            auto ratio = sim.state[c * num_amplitudes + r];
            if (ratio != std::complex<float>{0, 0}) {
                // Move to diagonal.
                if (r != c) {
                    size_t pivot = first_set_bit(r, q);
                    for (size_t b = 0; b < num_qubits; b++) {
                        if (((r >> b) & 1) != 0 && b != pivot) {
                            apply2(GateType::CX, pivot, b);
                        }
                    }
                    if (pivot != q) {
                        apply2(GateType::SWAP, q, pivot);
                    }
                }

                // Undo phasing on this qubit.
                if (ratio.real() == -1) {
                    apply(GateType::Z, q);
                } else if (ratio.imag() == -1) {
                    apply(GateType::S, q);
                } else if (ratio.imag() == +1) {
                    apply(GateType::S_DAG, q);
                }
                break;
            }
        }
    }

    // Undo double qubit phases.
    for (size_t q1 = 0; q1 < num_qubits; q1++) {
        for (size_t q2 = q1 + 1; q2 < num_qubits; q2++) {
            size_t v = (1 << q1) | (1 << q2);
            size_t d = v * num_amplitudes + v;
            if (sim.state[d].real() == -1) {
                apply2(GateType::CZ, q1, q2);
            }
        }
    }

    // Verify that we actually reduced the matrix to the identity.
    // If we failed, it wasn't actually a Clifford.
    for (size_t r = 0; r < num_amplitudes; r++) {
        for (size_t c = 0; c < num_amplitudes; c++) {
            if (sim.state[r * num_amplitudes + c] != std::complex<float>{r == c ? 1.0f : 0.0f}) {
                throw std::invalid_argument("The given unitary matrix wasn't a Clifford operation.");
            }
        }
    }

    // Conjugate by swaps to handle endianness.
    if (!little_endian) {
        for (size_t q = 0; 2 * q + 1 < num_qubits; q++) {
            recorded_circuit.safe_append_u("SWAP", {(uint32_t)q, (uint32_t)(num_qubits - q - 1)});
        }
    }
    recorded_circuit = circuit_inverse_unitary(recorded_circuit);
    if (!little_endian) {
        for (size_t q = 0; 2 * q + 1 < num_qubits; q++) {
            recorded_circuit.safe_append_u("SWAP", {(uint32_t)q, (uint32_t)(num_qubits - q - 1)});
        }
    }

    return circuit_to_tableau<W>(recorded_circuit, false, false, false);
}

}  // namespace stim
