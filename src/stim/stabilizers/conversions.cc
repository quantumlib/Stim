#include "stim/stabilizers/conversions.h"

#include "stim/probability_util.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/vector_simulator.h"

using namespace stim;

uint8_t stim::floor_lg2(size_t value) {
    uint8_t result = 0;
    while (value > 1) {
        result += 1;
        value >>= 1;
    }
    return result;
}

uint8_t stim::is_power_of_2(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

Circuit stim::unitary_circuit_inverse(const Circuit &unitary_circuit) {
    Circuit inverted;
    unitary_circuit.for_each_operation_reverse([&](const Operation &op) {
        if (!(op.gate->flags & GATE_IS_UNITARY)) {
            throw std::invalid_argument("Not unitary: " + op.str());
        }
        size_t step = (op.gate->flags & GATE_TARGETS_PAIRS) ? 2 : 1;
        auto s = op.target_data.targets.ptr_start;
        const auto &inv_gate = op.gate->inverse();
        for (size_t k = op.target_data.targets.size(); k > 0; k -= step) {
            inverted.append_operation(inv_gate, {s + k - step, s + k}, op.target_data.args);
        }
    });
    return inverted;
}

size_t biggest_index(const std::vector<std::complex<float>> &state_vector) {
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

size_t compute_occupation(const std::vector<std::complex<float>> &state_vector) {
    size_t c = 0;
    for (const auto &v : state_vector) {
        if (v != std::complex<float>{0, 0}) {
            c++;
        }
    }
    return c;
}

Circuit stim::stabilizer_state_vector_to_circuit(
    const std::vector<std::complex<float>> &state_vector, bool little_endian) {
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
    auto apply = [&](const std::string &name, uint32_t target) {
        sim.apply(name, target);
        recorded.append_op(name, {little_endian ? target : (num_qubits - target - 1)});
    };
    auto apply2 = [&](const std::string &name, uint32_t target, uint32_t target2) {
        sim.apply(name, target, target2);
        recorded.append_op(
            name,
            {
                little_endian ? target : (num_qubits - target - 1),
                little_endian ? target2 : (num_qubits - target2 - 1),
            });
    };

    // Move biggest amplitude to start of state vector..
    size_t pivot = biggest_index(state_vector);
    for (size_t q = 0; q < num_qubits; q++) {
        if ((pivot >> q) & 1) {
            apply("X", q);
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
                    apply2("CNOT", base_qubit, q);
                }
            }
        }

        auto s = sim.state[1 << base_qubit];
        assert(s != (std::complex<float>{0, 0}));
        if (s == std::complex<float>{-1, 0}) {
            apply("Z", base_qubit);
        } else if (s == std::complex<float>{0, 1}) {
            apply("S_DAG", base_qubit);
        } else if (s == std::complex<float>{0, -1}) {
            apply("S", base_qubit);
        }
        apply("H", base_qubit);

        sim.smooth_stabilizer_state(sim.state[0]);
        if (compute_occupation(sim.state) * 2 != occupation) {
            throw std::invalid_argument("State vector isn't a stabilizer state.");
        }
        occupation >>= 1;
    }

    recorded = unitary_circuit_inverse(recorded);
    if (recorded.count_qubits() < num_qubits) {
        recorded.append_op("I", {(uint32_t)(num_qubits - 1)});
    }

    return recorded;
}

std::vector<std::vector<std::complex<float>>> stim::tableau_to_unitary(const Tableau &tableau, bool little_endian) {
    auto flat = tableau.to_flat_unitary_matrix(little_endian);
    std::vector<std::vector<std::complex<float>>> result;
    size_t n = 1 << tableau.num_qubits;
    for (size_t row = 0; row < n; row++) {
        result.push_back({});
        result.back().insert(result.back().end(), &flat[row * n], &flat[row * n + n]);
    }
    return result;
}

Tableau stim::circuit_to_tableau(
    const Circuit &circuit, bool ignore_noise, bool ignore_measurement, bool ignore_reset) {
    Tableau result(circuit.count_qubits());
    std::mt19937_64 unused_rng(0);
    TableauSimulator sim(unused_rng, circuit.count_qubits());

    circuit.for_each_operation([&](const Operation &op) {
        if (op.gate->flags & GATE_IS_UNITARY) {
            (sim.*op.gate->tableau_simulator_function)(op.target_data);
        } else if (op.gate->flags & GATE_IS_NOISE) {
            if (!ignore_noise) {
                throw std::invalid_argument(
                    "The circuit has no well-defined tableau because it contains noisy operations.\n"
                    "To ignore noisy operations, pass the argument ignore_noise=True.\n"
                    "The first noisy operation is: " +
                    op.str());
            }
        } else if (op.gate->flags & (GATE_IS_RESET | GATE_PRODUCES_NOISY_RESULTS)) {
            if (!ignore_measurement && (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS)) {
                throw std::invalid_argument(
                    "The circuit has no well-defined tableau because it contains measurement operations.\n"
                    "To ignore measurement operations, pass the argument ignore_measurement=True.\n"
                    "The first measurement operation is: " +
                    op.str());
            }
            if (!ignore_reset && (op.gate->flags & GATE_IS_RESET)) {
                throw std::invalid_argument(
                    "The circuit has no well-defined tableau because it contains reset operations.\n"
                    "To ignore reset operations, pass the argument ignore_reset=True.\n"
                    "The first reset operation is: " +
                    op.str());
            }
        } else {
            // Operation should be an annotation like TICK or DETECTOR.
        }
    });

    return sim.inv_state.inverse();
}

std::vector<std::complex<float>> stim::circuit_to_output_state_vector(const Circuit &circuit, bool little_endian) {
    Tableau result(circuit.count_qubits());
    std::mt19937_64 unused_rng(0);
    TableauSimulator sim(unused_rng, circuit.count_qubits());

    circuit.for_each_operation([&](const Operation &op) {
        if (op.gate->flags & GATE_IS_UNITARY) {
            (sim.*op.gate->tableau_simulator_function)(op.target_data);
        } else if (op.gate->flags & (GATE_IS_NOISE | GATE_IS_RESET | GATE_PRODUCES_NOISY_RESULTS)) {
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

Circuit stim::tableau_to_circuit(const Tableau &tableau, const std::string &method) {
    if (method != "elimination") {
        std::stringstream ss;
        ss << "Unknown method: '" << method << "'. Known methods:\n";
        ss << "    - 'elimination'";
        throw std::invalid_argument(ss.str());
    }

    Tableau remaining = tableau.inverse();
    Circuit recorded_circuit;
    auto apply = [&](const std::string &name, uint32_t target) {
        remaining.inplace_scatter_append(GATE_DATA.at(name).tableau(), {target});
        recorded_circuit.append_op(name, {target});
    };
    auto apply2 = [&](const std::string &name, uint32_t target, uint32_t target2) {
        remaining.inplace_scatter_append(GATE_DATA.at(name).tableau(), {target, target2});
        recorded_circuit.append_op(name, {target, target2});
    };
    auto x_out = [&](size_t inp, size_t out) {
        const auto &p = remaining.xs[inp];
        return p.xs[out] + 2 * p.zs[out];
    };
    auto z_out = [&](size_t inp, size_t out) {
        const auto &p = remaining.zs[inp];
        return p.xs[out] + 2 * p.zs[out];
    };

    size_t n = remaining.num_qubits;
    for (size_t col = 0; col < n; col++) {
        // Find a cell with an anti-commuting pair of Paulis.
        size_t pivot_row;
        for (pivot_row = col; pivot_row < n; pivot_row++) {
            int px = x_out(col, pivot_row);
            int pz = z_out(col, pivot_row);
            if (px && pz && px != pz) {
                break;
            }
        }
        assert(pivot_row < n);  // Ensured by unitarity of the tableau.

        // Move the pivot to the diagonal.
        if (pivot_row != col) {
            apply2("CNOT", pivot_row, col);
            apply2("CNOT", col, pivot_row);
            apply2("CNOT", pivot_row, col);
        }

        // Transform the pivot to XZ.
        if (z_out(col, col) == 3) {
            apply("S", col);
        }
        if (z_out(col, col) != 2) {
            apply("H", col);
        }
        if (x_out(col, col) != 1) {
            apply("S", col);
        }

        // Use the pivot to remove all other terms in the X observable.
        for (size_t row = col + 1; row < n; row++) {
            if (x_out(col, row) == 3) {
                apply("S", row);
            }
        }
        for (size_t row = col + 1; row < n; row++) {
            if (x_out(col, row) == 2) {
                apply("H", row);
            }
        }
        for (size_t row = col + 1; row < n; row++) {
            if (x_out(col, row)) {
                apply2("CX", col, row);
            }
        }

        // Use the pivot to remove all other terms in the Z observable.
        for (size_t row = col + 1; row < n; row++) {
            if (z_out(col, row) == 3) {
                apply("S", row);
            }
        }
        for (size_t row = col + 1; row < n; row++) {
            if (z_out(col, row) == 1) {
                apply("H", row);
            }
        }
        for (size_t row = col + 1; row < n; row++) {
            if (z_out(col, row)) {
                apply2("CX", row, col);
            }
        }
    }

    // Fix pauli signs.
    simd_bits signs_copy = remaining.zs.signs;
    for (size_t col = 0; col < n; col++) {
        if (signs_copy[col]) {
            apply("H", col);
        }
    }
    for (size_t col = 0; col < n; col++) {
        if (signs_copy[col]) {
            apply("S", col);
            apply("S", col);
        }
    }
    for (size_t col = 0; col < n; col++) {
        if (signs_copy[col]) {
            apply("H", col);
        }
    }
    signs_copy = remaining.xs.signs;
    for (size_t col = 0; col < n; col++) {
        if (remaining.xs.signs[col]) {
            apply("S", col);
            apply("S", col);
        }
    }

    if (recorded_circuit.count_qubits() < n) {
        apply("H", n - 1);
        apply("H", n - 1);
    }
    return recorded_circuit;
}

size_t first_set_bit(size_t value, size_t min_result) {
    size_t t = min_result;
    value >>= min_result;
    assert(value);
    while (!(value & 1)) {
        value >>= 1;
        t += 1;
    }
    return t;
}

Tableau stim::unitary_to_tableau(const std::vector<std::vector<std::complex<float>>> &matrix, bool little_endian) {
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
    Circuit recorded_circuit = unitary_circuit_inverse(stabilizer_state_vector_to_circuit(first_col, true));

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

    auto apply = [&](const std::string &name, uint32_t target) {
        sim.apply(name, target);
        recorded_circuit.append_op(name, {target});
    };
    auto apply2 = [&](const std::string &name, uint32_t target, uint32_t target2) {
        sim.apply(name, target, target2);
        recorded_circuit.append_op(name, {target, target2});
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
                            apply2("CX", pivot, b);
                        }
                    }
                    if (pivot != q) {
                        apply2("SWAP", q, pivot);
                    }
                }

                // Undo phasing on this qubit.
                if (ratio.real() == -1) {
                    apply("Z", q);
                } else if (ratio.imag() == -1) {
                    apply("S", q);
                } else if (ratio.imag() == +1) {
                    apply("S_DAG", q);
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
                apply2("CZ", q1, q2);
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
            recorded_circuit.append_op("SWAP", {(uint32_t)q, (uint32_t)(num_qubits - q - 1)});
        }
    }
    recorded_circuit = unitary_circuit_inverse(recorded_circuit);
    if (!little_endian) {
        for (size_t q = 0; 2 * q + 1 < num_qubits; q++) {
            recorded_circuit.append_op("SWAP", {(uint32_t)q, (uint32_t)(num_qubits - q - 1)});
        }
    }

    return circuit_to_tableau(recorded_circuit, false, false, false);
}

Tableau stim::stabilizers_to_tableau(
    const std::vector<stim::PauliString> &stabilizers, bool allow_redundant, bool allow_underconstrained, bool invert) {
    size_t num_qubits = 0;
    for (const auto &e : stabilizers) {
        num_qubits = std::max(num_qubits, e.num_qubits);
    }

    Tableau inverted(num_qubits);

    PauliString cur(num_qubits);
    std::vector<size_t> targets;
    while (targets.size() < num_qubits) {
        targets.push_back(targets.size());
    }
    auto overwrite_cur_apply_recorded = [&](const PauliString &e) {
        PauliStringRef cur_ref = cur.ref();
        cur.xs.clear();
        cur.zs.clear();
        cur.xs.word_range_ref(0, e.xs.num_simd_words) = e.xs;
        cur.zs.word_range_ref(0, e.xs.num_simd_words) = e.zs;
        cur.sign = e.sign;
        inverted.apply_within(cur_ref, targets);
    };

    size_t used = 0;
    for (const auto &e : stabilizers) {
        overwrite_cur_apply_recorded(e);

        // Find a non-identity term in the Pauli string past the region used by other stabilizers.
        size_t pivot;
        for (pivot = used; pivot < num_qubits; pivot++) {
            if (cur.xs[pivot] || cur.zs[pivot]) {
                break;
            }
        }

        // Check for incompatible / redundant stabilizers.
        if (pivot == num_qubits) {
            if (cur.xs.not_zero()) {
                throw std::invalid_argument("Some of the given stabilizers anticommute.");
            }
            if (cur.sign) {
                throw std::invalid_argument("Some of the given stabilizers contradict each other.");
            }
            if (!allow_redundant && cur.zs.not_zero()) {
                throw std::invalid_argument(
                    "Didn't specify allow_redundant=True but one of the given stabilizers is a product of the others. "
                    "To allow redundant stabilizers, pass the argument allow_redundant=True.");
            }
            continue;
        }

        // Change pivot basis to the Z axis.
        if (cur.xs[pivot]) {
            inverted.inplace_scatter_append(GATE_DATA.at(cur.zs[pivot] ? "H_YZ" : "H_XZ").tableau(), {pivot});
        }
        // Cancel other terms in Pauli string.
        for (size_t q = 0; q < num_qubits; q++) {
            int p = cur.xs[q] + cur.zs[q] * 2;
            if (p && q != pivot) {
                inverted.inplace_scatter_append(
                    GATE_DATA.at(p == 1   ? "XCX"
                                 : p == 2 ? "XCZ"
                                          : "XCY")
                        .tableau(),
                    {pivot, q});
            }
        }

        // Move pivot to diagonal.
        if (pivot != used) {
            inverted.inplace_scatter_append(GATE_DATA.at("SWAP").tableau(), {pivot, used});
        }

        // Fix sign.
        overwrite_cur_apply_recorded(e);
        if (cur.sign) {
            inverted.inplace_scatter_append(GATE_DATA.at("X").tableau(), {used});
        }

        used++;
    }

    if (used < num_qubits) {
        if (!allow_underconstrained) {
            throw std::invalid_argument(
                "There weren't enough stabilizers to uniquely specify the state. "
                "To allow underspecifying the state, pass the argument allow_underconstrained=True.");
        }
    }

    if (invert) {
        return inverted;
    }
    return inverted.inverse();
}
