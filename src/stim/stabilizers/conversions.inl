#include "stim/probability_util.h"
#include "stim/simulators/graph_simulator.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/conversions.h"
#include "stim/util_top/circuit_inverse_unitary.h"
#include "stim/util_top/circuit_vs_tableau.h"

namespace stim {

inline size_t biggest_index(const std::vector<std::complex<float>> &state_vector) {
    size_t best_index = 0;
    float best_size = std::norm(state_vector[0]);
    for (size_t k = 1; k < state_vector.size(); k++) {
        float size = std::norm(state_vector[k]);
        if (size > best_size) {
            best_size = size;
            best_index = k;
        }
    }
    return best_index;
}

inline size_t compute_occupation(const std::vector<std::complex<float>> &state_vector) {
    size_t c = 0;
    for (const auto &v : state_vector) {
        if (v != std::complex<float>{0, 0}) {
            c++;
        }
    }
    return c;
}

template <size_t W>
Circuit stabilizer_state_vector_to_circuit(const std::vector<std::complex<float>> &state_vector, bool little_endian) {
    if (!is_power_of_2(state_vector.size())) {
        std::stringstream ss;
        ss << "Expected number of amplitudes to be a power of 2.";
        ss << " The given state vector had " << state_vector.size() << " amplitudes.";
        throw std::invalid_argument(ss.str());
    }

    uint8_t num_qubits = floor_lg2(state_vector.size());
    double weight = 0;
    for (const auto &c : state_vector) {
        weight += std::norm(c);
    }
    if (abs(weight - 1) > 0.125) {
        throw std::invalid_argument(
            "The given state vector wasn't a unit vector. It had a length of " + std::to_string(weight) + ".");
    }

    VectorSimulator sim(num_qubits);
    sim.state = state_vector;

    Circuit recorded;
    auto apply = [&](GateType gate_type, uint32_t target) {
        sim.apply(gate_type, target);
        recorded.safe_append(
            gate_type,
            std::vector<GateTarget>{
                GateTarget::qubit(little_endian ? target : (num_qubits - target - 1)),
            },
            {});
    };
    auto apply2 = [&](GateType gate_type, uint32_t target, uint32_t target2) {
        sim.apply(gate_type, target, target2);
        recorded.safe_append(
            gate_type,
            std::vector<GateTarget>{
                GateTarget::qubit(little_endian ? target : (num_qubits - target - 1)),
                GateTarget::qubit(little_endian ? target2 : (num_qubits - target2 - 1)),
            },
            {});
    };

    // Move biggest amplitude to start of state vector..
    size_t pivot = biggest_index(state_vector);
    for (size_t q = 0; q < num_qubits; q++) {
        if ((pivot >> q) & 1) {
            apply(GateType::X, q);
        }
    }
    sim.smooth_stabilizer_state(sim.state[0]);
    size_t occupation = compute_occupation(sim.state);
    if (!is_power_of_2(occupation)) {
        throw std::invalid_argument("State vector isn't a stabilizer state.");
    }

    // Repeatedly cancel amplitudes
    while (occupation > 1) {
        size_t k = 1;
        for (; k < state_vector.size(); k++) {
            if (sim.state[k].real() || sim.state[k].imag()) {
                break;
            }
        }
        if (k == state_vector.size()) {
            break;
        }

        size_t base_qubit = SIZE_MAX;
        for (size_t q = 0; q < num_qubits; q++) {
            if ((k >> q) & 1) {
                if (base_qubit == SIZE_MAX) {
                    base_qubit = q;
                } else {
                    apply2(GateType::CX, base_qubit, q);
                }
            }
        }

        auto s = sim.state[1 << base_qubit];
        assert(s != (std::complex<float>{0, 0}));
        if (s == std::complex<float>{-1, 0}) {
            apply(GateType::Z, base_qubit);
        } else if (s == std::complex<float>{0, 1}) {
            apply(GateType::S_DAG, base_qubit);
        } else if (s == std::complex<float>{0, -1}) {
            apply(GateType::S, base_qubit);
        }
        apply(GateType::H, base_qubit);

        sim.smooth_stabilizer_state(sim.state[0]);
        if (compute_occupation(sim.state) * 2 != occupation) {
            throw std::invalid_argument("State vector isn't a stabilizer state.");
        }
        occupation >>= 1;
    }

    recorded = unitary_circuit_inverse(recorded);
    if (recorded.count_qubits() < num_qubits) {
        recorded.safe_append_u("I", {(uint32_t)(num_qubits - 1)});
    }

    return recorded;
}

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
std::vector<std::complex<float>> circuit_to_output_state_vector(const Circuit &circuit, bool little_endian) {
    Tableau<W> result(circuit.count_qubits());
    TableauSimulator<W> sim(std::mt19937_64(0), circuit.count_qubits());

    circuit.for_each_operation([&](const CircuitInstruction &op) {
        const auto &flags = GATE_DATA[op.gate_type].flags;
        if (flags & GATE_IS_UNITARY) {
            sim.do_gate(op);
        } else if (flags & (GATE_IS_NOISY | GATE_IS_RESET | GATE_PRODUCES_RESULTS)) {
            throw std::invalid_argument(
                "The circuit has no well-defined tableau because it contains noisy or dissipative operations.\n"
                "The first such operation is: " +
                op.str());
        } else {
            // Operation should be an annotation like TICK or DETECTOR.
        }
    });

    return sim.to_state_vector(little_endian);
}

inline size_t first_set_bit(size_t value, size_t min_result) {
    size_t t = min_result;
    value >>= min_result;
    assert(value);
    while (!(value & 1)) {
        value >>= 1;
        t += 1;
    }
    return t;
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
    Circuit recorded_circuit = unitary_circuit_inverse(stabilizer_state_vector_to_circuit<W>(first_col, true));

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
        recorded_circuit.safe_append(gate_type, std::vector<GateTarget>{GateTarget::qubit(target)}, {});
    };
    auto apply2 = [&](GateType gate_type, uint32_t target, uint32_t target2) {
        sim.apply(gate_type, target, target2);
        recorded_circuit.safe_append(
            gate_type, std::vector<GateTarget>{GateTarget::qubit(target), GateTarget::qubit(target2)}, {});
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
    recorded_circuit = unitary_circuit_inverse(recorded_circuit);
    if (!little_endian) {
        for (size_t q = 0; 2 * q + 1 < num_qubits; q++) {
            recorded_circuit.safe_append_u("SWAP", {(uint32_t)q, (uint32_t)(num_qubits - q - 1)});
        }
    }

    return circuit_to_tableau<W>(recorded_circuit, false, false, false);
}

template <size_t W>
Tableau<W> stabilizers_to_tableau(
    const std::vector<PauliString<W>> &stabilizers, bool allow_redundant, bool allow_underconstrained, bool invert) {
    size_t num_qubits = 0;
    for (const auto &e : stabilizers) {
        num_qubits = std::max(num_qubits, e.num_qubits);
    }

    simd_bit_table<W> buf_xs(stabilizers.size(), num_qubits);
    simd_bit_table<W> buf_zs(stabilizers.size(), num_qubits);
    simd_bits<W> buf_signs(stabilizers.size());
    for (size_t k = 0; k < stabilizers.size(); k++) {
        memcpy(buf_xs[k].u8, stabilizers[k].xs.u8, stabilizers[k].xs.num_u8_padded());
        memcpy(buf_zs[k].u8, stabilizers[k].zs.u8, stabilizers[k].zs.num_u8_padded());
        buf_signs[k] = stabilizers[k].sign;
    }
    buf_xs = buf_xs.transposed();
    buf_zs = buf_zs.transposed();

    Circuit elimination_instructions;

    size_t used = 0;
    for (size_t k = 0; k < stabilizers.size(); k++) {
        // Find a non-identity term in the Pauli string past the region used by other stabilizers.
        size_t pivot;
        for (pivot = used; pivot < num_qubits; pivot++) {
            if (buf_xs[pivot][k] || buf_zs[pivot][k]) {
                break;
            }
        }

        // Check for incompatible / redundant stabilizers.
        if (pivot == num_qubits) {
            if (buf_signs[k]) {
                throw std::invalid_argument("Some of the given stabilizers contradict each other.");
            }
            if (!allow_redundant) {
                throw std::invalid_argument(
                    "Didn't specify allow_redundant=True but one of the given stabilizers is a product of the others. "
                    "To allow redundant stabilizers, pass the argument allow_redundant=True.");
            }
            continue;
        }

        // Change pivot basis to the Z axis.
        if (buf_xs[pivot][k]) {
            GateType g = buf_zs[pivot][k] ? GateType::H_YZ : GateType::H;
            GateTarget t = GateTarget::qubit(pivot);
            CircuitInstruction instruction{g, {}, &t};
            elimination_instructions.safe_append(instruction);
            size_t q = pivot;
            simd_bits_range_ref<W> xs1 = buf_xs[q];
            simd_bits_range_ref<W> zs1 = buf_zs[q];
            simd_bits_range_ref<W> ss = buf_signs;
            switch (g) {
                case GateType::H_YZ:
                    ss.for_each_word(xs1, zs1, [](auto &s, auto &x, auto &z) {
                        x ^= z;
                        s ^= z.andnot(x);
                    });
                    break;
                case GateType::H:
                    ss.for_each_word(xs1, zs1, [](auto &s, auto &x, auto &z) {
                        std::swap(x, z);
                        s ^= x & z;
                    });
                    break;
                default:
                    throw std::invalid_argument("Unrecognized gate type.");
            }
        }

        // Cancel other terms in Pauli string.
        for (size_t q = 0; q < num_qubits; q++) {
            int p = buf_xs[q][k] + buf_zs[q][k] * 2;
            if (p && q != pivot) {
                std::array<GateTarget, 2> targets{GateTarget::qubit(pivot), GateTarget::qubit(q)};
                GateType g = p == 1 ? GateType::XCX : p == 2 ? GateType::XCZ : GateType::XCY;
                CircuitInstruction instruction{g, {}, targets};
                elimination_instructions.safe_append(instruction);
                size_t q1 = targets[0].qubit_value();
                size_t q2 = targets[1].qubit_value();
                simd_bits_range_ref<W> ss = buf_signs;
                simd_bits_range_ref<W> xs1 = buf_xs[q1];
                simd_bits_range_ref<W> zs1 = buf_zs[q1];
                simd_bits_range_ref<W> xs2 = buf_xs[q2];
                simd_bits_range_ref<W> zs2 = buf_zs[q2];
                switch (g) {
                    case GateType::XCX:
                        ss.for_each_word(xs1, zs1, xs2, zs2, [](auto &s, auto &x1, auto &z1, auto &x2, auto &z2) {
                            s ^= (x1 ^ x2) & z1 & z2;
                            x1 ^= z2;
                            x2 ^= z1;
                        });
                        break;
                    case GateType::XCY:
                        ss.for_each_word(xs1, zs1, xs2, zs2, [](auto &s, auto &x1, auto &z1, auto &x2, auto &z2) {
                            x1 ^= x2 ^ z2;
                            x2 ^= z1;
                            z2 ^= z1;
                            s ^= x1.andnot(z1) & x2.andnot(z2);
                            s ^= x1 & z1 & z2.andnot(x2);
                        });
                        break;
                    case GateType::XCZ:
                        ss.for_each_word(xs1, zs1, xs2, zs2, [](auto &s, auto &x1, auto &z1, auto &x2, auto &z2) {
                            z2 ^= z1;
                            x1 ^= x2;
                            s ^= (z2 ^ x1).andnot(z1 & x2);
                        });
                        break;
                    default:
                        throw std::invalid_argument("Unrecognized gate type.");
                }
            }
        }

        // Move pivot to diagonal.
        if (pivot != used) {
            std::array<GateTarget, 2> targets{GateTarget::qubit(pivot), GateTarget::qubit(used)};
            CircuitInstruction instruction{GateType::SWAP, {}, targets};
            elimination_instructions.safe_append(instruction);
            buf_xs[pivot].swap_with(buf_xs[used]);
            buf_zs[pivot].swap_with(buf_zs[used]);
        }

        // Fix sign.
        if (buf_signs[k]) {
            GateTarget t = GateTarget::qubit(used);
            CircuitInstruction instruction{GateType::X, {}, &t};
            elimination_instructions.safe_append(instruction);
            buf_signs ^= buf_zs[used];
        }

        used++;
    }

    // All stabilizers will have been mapped into Z products, if they commuted.
    for (size_t q = 0; q < num_qubits; q++) {
        if (buf_xs[q].not_zero()) {
            for (size_t k1 = 0; k1 < stabilizers.size(); k1++) {
                for (size_t k2 = k1 + 1; k2 < stabilizers.size(); k2++) {
                    if (!stabilizers[k1].ref().commutes(stabilizers[k2])) {
                        std::stringstream ss;
                        ss << "Some of the given stabilizers anticommute.\n";
                        ss << "For example:\n    ";
                        ss << stabilizers[k1];
                        ss << "\nanticommutes with\n";
                        ss << stabilizers[k2] << "\n";
                        throw std::invalid_argument(ss.str());
                    }
                }
            }
            throw std::invalid_argument("The given stabilizers commute but the solver failed in a way that suggests they anticommute. Please report this as a bug.");
        }
    }

    if (used < num_qubits) {
        if (!allow_underconstrained) {
            throw std::invalid_argument(
                "There weren't enough stabilizers to uniquely specify the state. "
                "To allow underspecifying the state, pass the argument allow_underconstrained=True.");
        }
    }

    if (num_qubits > 0) {
        // Force size of resulting tableau to be correct.
        GateTarget t = GateTarget::qubit(num_qubits - 1);
        elimination_instructions.safe_append(CircuitInstruction{GateType::X, {}, &t});
        elimination_instructions.safe_append(CircuitInstruction{GateType::X, {}, &t});
    }

    if (invert) {
        return circuit_to_tableau<W>(elimination_instructions.inverse(), false, false, false, true);
    }
    return circuit_to_tableau<W>(elimination_instructions, false, false, false, true);
}

}  // namespace stim
