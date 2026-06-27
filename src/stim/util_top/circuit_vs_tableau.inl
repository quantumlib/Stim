#include "stim/simulators/graph_simulator.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_top/circuit_vs_tableau.h"

namespace stim {

template <size_t W>
Tableau<W> circuit_to_tableau(
    const Circuit &circuit, bool ignore_noise, bool ignore_measurement, bool ignore_reset, bool inverse) {
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

    if (!inverse) {
        return sim.inv_state.inverse();
    }
    return sim.inv_state;
}

template <size_t W>
Circuit tableau_to_circuit(const Tableau<W> &tableau, std::string_view method) {
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
        result.safe_append(CircuitInstruction(GateType::MPP, {}, targets, ""));
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
            result.safe_append(CircuitInstruction(GateType::CX, {}, targets_x, ""));
        }
        if (!targets_y.empty()) {
            result.safe_append(CircuitInstruction(GateType::CY, {}, targets_y, ""));
        }
        if (!targets_z.empty()) {
            result.safe_append(CircuitInstruction(GateType::CZ, {}, targets_z, ""));
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
        recorded_circuit.safe_append(
            CircuitInstruction(gate_type, {}, std::vector<GateTarget>{GateTarget::qubit(target)}, ""));
    };
    auto apply2 = [&](GateType gate_type, uint32_t target, uint32_t target2) {
        remaining.inplace_scatter_append(GATE_DATA[gate_type].tableau<W>(), {target, target2});
        recorded_circuit.safe_append(CircuitInstruction(
            gate_type, {}, std::vector<GateTarget>{GateTarget::qubit(target), GateTarget::qubit(target2)}, ""));
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

}  // namespace stim
