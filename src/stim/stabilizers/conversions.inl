#include "stim/probability_util.h"
#include "stim/simulators/graph_simulator.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/conversions.h"

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
Tableau<W> circuit_to_tableau(const Circuit &circuit, bool ignore_noise, bool ignore_measurement, bool ignore_reset) {
    Tableau<W> result(circuit.count_qubits());
    TableauSimulator<W> sim(std::mt19937_64(0), circuit.count_qubits());

    circuit.for_each_operation([&](const CircuitInstruction &op) {
        const auto &flags = GATE_DATA[op.gate_type].flags;
        if (!ignore_measurement && (flags & GATE_PRODUCES_RESULTS)) {
            throw std::invalid_argument(
                "The circuit has no well-defined tableau because it contains measurement operations.\n"
                "To ignore measurement operations, pass the argument ignore_measurement=True.\n"
                "The first measurement operation is: " +
                op.str());
        }
        if (!ignore_reset && (flags & GATE_IS_RESET)) {
            throw std::invalid_argument(
                "The circuit has no well-defined tableau because it contains reset operations.\n"
                "To ignore reset operations, pass the argument ignore_reset=True.\n"
                "The first reset operation is: " +
                op.str());
        }
        if (!ignore_noise && (flags & GATE_IS_NOISY)) {
            for (const auto &f : op.args) {
                if (f > 0) {
                    throw std::invalid_argument(
                        "The circuit has no well-defined tableau because it contains noisy operations.\n"
                        "To ignore noisy operations, pass the argument ignore_noise=True.\n"
                        "The first noisy operation is: " +
                        op.str());
                }
            }
        }
        if (flags & GATE_IS_UNITARY) {
            sim.do_gate(op);
        }
    });

    return sim.inv_state.inverse();
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

template <size_t W>
Circuit tableau_to_circuit(const Tableau<W> &tableau, const std::string &method) {
    if (method == "elimination") {
        return tableau_to_circuit_elimination_method(tableau);
    } else if (method == "graph_state") {
        return tableau_to_circuit_graph_method(tableau);
    } else if (method == "mpp_state") {
        return tableau_to_circuit_mpp_method(tableau, false);
    } else if (method == "mpp_state_unsigned") {
        return tableau_to_circuit_mpp_method(tableau, true);
    } else {
        std::stringstream ss;
        ss << "Unknown method: '" << method << "'. Known methods:\n";
        ss << "    - 'elimination'\n";
        ss << "    - 'graph_state'\n";
        ss << "    - 'mpp_state'\n";
        ss << "    - 'mpp_state_unsigned'\n";
        throw std::invalid_argument(ss.str());
    }
}

template <size_t W>
Circuit tableau_to_circuit_graph_method(const Tableau<W> &tableau) {
    GraphSimulator sim(tableau.num_qubits);
    sim.do_circuit(tableau_to_circuit_elimination_method(tableau));
    return sim.to_circuit(true);
}

template <size_t W>
Circuit tableau_to_circuit_mpp_method(const Tableau<W> &tableau, bool skip_sign) {
    Circuit result;
    std::vector<GateTarget> targets;
    size_t n = tableau.num_qubits;

    // Measure each stabilizer with MPP.
    for (size_t k = 0; k < n; k++) {
        const auto &stabilizer = tableau.zs[k];
        bool need_sign = stabilizer.sign;
        for (size_t q = 0; q < n; q++) {
            bool x = stabilizer.xs[q];
            bool z = stabilizer.zs[q];
            if (x || z) {
                targets.push_back(GateTarget::pauli_xz(q, x, z, need_sign));
                targets.push_back(GateTarget::combiner());
                need_sign = false;
            }
        }
        assert(!targets.empty());
        targets.pop_back();
        result.safe_append(GateType::MPP, targets, {});
        targets.clear();
    }

    if (!skip_sign) {
        // Correct each stabilizer's sign with feedback.
        std::vector<GateTarget> targets_x;
        std::vector<GateTarget> targets_y;
        std::vector<GateTarget> targets_z;
        std::array<std::vector<GateTarget> *, 4> targets_ptrs = {nullptr, &targets_x, &targets_z, &targets_y};
        for (size_t k = 0; k < n; k++) {
            const auto &destabilizer = tableau.xs[k];
            for (size_t q = 0; q < n; q++) {
                bool x = destabilizer.xs[q];
                bool z = destabilizer.zs[q];
                auto *out = targets_ptrs[x + z * 2];
                if (out != nullptr) {
                    out->push_back(GateTarget::rec(-(int32_t)(n - k)));
                    out->push_back(GateTarget::qubit(q));
                }
            }
        }
        if (!targets_x.empty()) {
            result.safe_append(GateType::CX, targets_x, {});
        }
        if (!targets_y.empty()) {
            result.safe_append(GateType::CY, targets_y, {});
        }
        if (!targets_z.empty()) {
            result.safe_append(GateType::CZ, targets_z, {});
        }
    }

    return result;
}

template <size_t W>
Circuit tableau_to_circuit_elimination_method(const Tableau<W> &tableau) {
    Tableau<W> remaining = tableau.inverse();
    Circuit recorded_circuit;
    auto apply = [&](GateType gate_type, uint32_t target) {
        remaining.inplace_scatter_append(GATE_DATA[gate_type].tableau<W>(), {target});
        recorded_circuit.safe_append(gate_type, std::vector<GateTarget>{GateTarget::qubit(target)}, {});
    };
    auto apply2 = [&](GateType gate_type, uint32_t target, uint32_t target2) {
        remaining.inplace_scatter_append(GATE_DATA[gate_type].tableau<W>(), {target, target2});
        recorded_circuit.safe_append(
            gate_type, std::vector<GateTarget>{GateTarget::qubit(target), GateTarget::qubit(target2)}, {});
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
            apply2(GateType::CX, pivot_row, col);
            apply2(GateType::CX, col, pivot_row);
            apply2(GateType::CX, pivot_row, col);
        }

        // Transform the pivot to XZ.
        if (z_out(col, col) == 3) {
            apply(GateType::S, col);
        }
        if (z_out(col, col) != 2) {
            apply(GateType::H, col);
        }
        if (x_out(col, col) != 1) {
            apply(GateType::S, col);
        }

        // Use the pivot to remove all other terms in the X observable.
        for (size_t row = col + 1; row < n; row++) {
            if (x_out(col, row) == 3) {
                apply(GateType::S, row);
            }
        }
        for (size_t row = col + 1; row < n; row++) {
            if (x_out(col, row) == 2) {
                apply(GateType::H, row);
            }
        }
        for (size_t row = col + 1; row < n; row++) {
            if (x_out(col, row)) {
                apply2(GateType::CX, col, row);
            }
        }

        // Use the pivot to remove all other terms in the Z observable.
        for (size_t row = col + 1; row < n; row++) {
            if (z_out(col, row) == 3) {
                apply(GateType::S, row);
            }
        }
        for (size_t row = col + 1; row < n; row++) {
            if (z_out(col, row) == 1) {
                apply(GateType::H, row);
            }
        }
        for (size_t row = col + 1; row < n; row++) {
            if (z_out(col, row)) {
                apply2(GateType::CX, row, col);
            }
        }
    }

    // Fix pauli signs.
    simd_bits<W> signs_copy = remaining.zs.signs;
    for (size_t col = 0; col < n; col++) {
        if (signs_copy[col]) {
            apply(GateType::H, col);
        }
    }
    for (size_t col = 0; col < n; col++) {
        if (signs_copy[col]) {
            apply(GateType::S, col);
            apply(GateType::S, col);
        }
    }
    for (size_t col = 0; col < n; col++) {
        if (signs_copy[col]) {
            apply(GateType::H, col);
        }
    }
    for (size_t col = 0; col < n; col++) {
        if (remaining.xs.signs[col]) {
            apply(GateType::S, col);
            apply(GateType::S, col);
        }
    }

    if (recorded_circuit.count_qubits() < n) {
        apply(GateType::H, n - 1);
        apply(GateType::H, n - 1);
    }
    return recorded_circuit;
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

    for (size_t k1 = 0; k1 < stabilizers.size(); k1++) {
        for (size_t k2 = 0; k2 < stabilizers.size(); k2++) {
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
    Tableau<W> inverted(num_qubits);

    PauliString<W> cur(num_qubits);
    std::vector<size_t> targets;
    while (targets.size() < num_qubits) {
        targets.push_back(targets.size());
    }
    auto overwrite_cur_apply_recorded = [&](const PauliString<W> &e) {
        PauliStringRef<W> cur_ref = cur.ref();
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
            std::string name = cur.zs[pivot] ? "H_YZ" : "H_XZ";
            inverted.inplace_scatter_append(GATE_DATA.at(name).tableau<W>(), {pivot});
        }
        // Cancel other terms in Pauli string.
        for (size_t q = 0; q < num_qubits; q++) {
            int p = cur.xs[q] + cur.zs[q] * 2;
            if (p && q != pivot) {
                inverted.inplace_scatter_append(
                    GATE_DATA.at(p == 1   ? "XCX"
                                 : p == 2 ? "XCZ"
                                          : "XCY")
                        .tableau<W>(),
                    {pivot, q});
            }
        }

        // Move pivot to diagonal.
        if (pivot != used) {
            inverted.inplace_scatter_append(GATE_DATA.at("SWAP").tableau<W>(), {pivot, used});
        }

        // Fix sign.
        overwrite_cur_apply_recorded(e);
        if (cur.sign) {
            inverted.inplace_scatter_append(GATE_DATA.at("X").tableau<W>(), {used});
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

}  // namespace stim
