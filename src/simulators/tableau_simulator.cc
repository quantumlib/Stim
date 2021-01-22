#include <queue>

#include "gate_data.h"
#include "tableau_simulator.h"

TableauSimulator::TableauSimulator(size_t num_qubits, std::mt19937_64 &rng) : inv_state(Tableau::identity(num_qubits)), rng(rng) {
}

bool TableauSimulator::is_deterministic(size_t target) const {
    return !inv_state.zs[target].x_ref.not_zero();
}

void TableauSimulator::measure(const OperationData &target_data, int8_t sign_bias) {
    // Ensure measurement observables are collapsed, picking random results.
    collapse(target_data, sign_bias);

    // Record measurement results.
    for (size_t k = 0; k < target_data.targets.size(); k++) {
        recorded_measurement_results.push(inv_state.zs.signs[target_data.targets[k]] ^ target_data.flags[k]);
    }
}

void TableauSimulator::reset(const OperationData &target_data, int8_t sign_bias) {
    // Collapse the qubits to be reset.
    collapse(target_data, sign_bias);

    // Force the collapsed qubits into the desired state.
    for (size_t k = 0; k < target_data.targets.size(); k++) {
        inv_state.zs.signs[target_data.targets[k]] = target_data.flags[k];
    }
}

void TableauSimulator::H_XZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_XZ(q);
    }
}

void TableauSimulator::H_XY(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_XY(q);
    }
}

void TableauSimulator::H_YZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_YZ(q);
    }
}

void TableauSimulator::SQRT_Z(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Z_DAG(q);
    }
}

void TableauSimulator::SQRT_Z_DAG(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Z(q);
    }
}

void TableauSimulator::SQRT_X(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_X_DAG(q);
    }
}

void TableauSimulator::SQRT_X_DAG(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_X(q);
    }
}

void TableauSimulator::SQRT_Y(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Y_DAG(q);
    }
}

void TableauSimulator::SQRT_Y_DAG(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Y(q);
    }
}

void TableauSimulator::CX(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_CX(c, t);
    }
}

void TableauSimulator::CY(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_CY(c, t);
    }
}

void TableauSimulator::CZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_CZ(c, t);
    }
}

void TableauSimulator::SWAP(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_SWAP(q1, q2);
    }
}

void TableauSimulator::ISWAP(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_ISWAP_DAG(q1, q2);
    }
}

void TableauSimulator::ISWAP_DAG(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_ISWAP(q1, q2);
    }
}

void TableauSimulator::XCX(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_XCX(q1, q2);
    }
}
void TableauSimulator::XCY(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_XCY(q1, q2);
    }
}
void TableauSimulator::XCZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_XCZ(q1, q2);
    }
}
void TableauSimulator::YCX(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_YCX(q1, q2);
    }
}
void TableauSimulator::YCY(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_YCY(q1, q2);
    }
}
void TableauSimulator::YCZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_YCZ(q1, q2);
    }
}

void TableauSimulator::X(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_X(q);
    }
}

void TableauSimulator::Y(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_Y(q);
    }
}

void TableauSimulator::Z(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_Z(q);
    }
}

void TableauSimulator::apply(const std::string &name, const OperationData &target_data) {
    try {
        SIM_TABLEAU_GATE_FUNC_DATA.at(name)(*this, target_data);
    } catch (const std::out_of_range &ex) {
        throw std::out_of_range("Gate isn't supported by TableauSimulator: " + name);
    }
}

void TableauSimulator::apply(const Tableau &tableau, const OperationData &target_data) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend(tableau.inverse(), target_data.targets);
}

std::vector<bool> TableauSimulator::sample_circuit(const Circuit &circuit, std::mt19937_64 &rng) {
    TableauSimulator sim(circuit.num_qubits, rng);
    for (const auto &op : circuit.operations) {
        sim.apply(op.name, op.target_data);
    }

    std::vector<bool> result;
    while (!sim.recorded_measurement_results.empty()) {
        result.push_back(sim.recorded_measurement_results.front());
        sim.recorded_measurement_results.pop();
    }
    return result;
}

void TableauSimulator::ensure_large_enough_for_qubit(size_t max_q) {
    if (max_q < inv_state.num_qubits) {
        return;
    }
    inv_state.expand(max_q + 1);
}

void TableauSimulator::sample_stream(FILE *in, FILE *out, bool newline_after_ticks, std::mt19937_64 &rng) {
    CircuitReader reader(in);
    size_t max_qubit = 0;
    TableauSimulator sim(1, rng);
    while (reader.read_next_moment(newline_after_ticks)) {
        for (const auto &e : reader.operations) {
            for (size_t q : e.target_data.targets) {
                max_qubit = std::max(q, max_qubit);
            }
        }
        sim.ensure_large_enough_for_qubit(max_qubit);

        for (const auto &op : reader.operations) {
            if (op.name == "M") {
                sim.measure(op.target_data);
                while (!sim.recorded_measurement_results.empty()) {
                    putc_unlocked(sim.recorded_measurement_results.front() ? '1' : '0', out);
                    sim.recorded_measurement_results.pop();
                }
                if (newline_after_ticks) {
                    putc_unlocked('\n', out);
                    fflush(out);
                }
            } else {
                SIM_TABLEAU_GATE_FUNC_DATA.at(op.name)(sim, op.target_data);
            }
        }
    }
    if (!newline_after_ticks) {
        putc_unlocked('\n', out);
    }
}

VectorSimulator TableauSimulator::to_vector_sim() const {
    auto inv = inv_state.inverse();
    std::vector<PauliStringRef> stabilizers;
    for (size_t k = 0; k < inv.num_qubits; k++) {
        stabilizers.push_back(inv.zs[k]);
    }
    return VectorSimulator::from_stabilizers(stabilizers, rng);
}

void TableauSimulator::collapse(const OperationData &target_data, int8_t sign_bias) {
    // Find targets that need to be collapsed.
    std::vector<size_t> collapse_targets;
    collapse_targets.reserve(target_data.targets.size());
    for (size_t t : target_data.targets) {
        if (!is_deterministic(t)) {
            collapse_targets.push_back(t);
        }
    }

    // Only pay the cost of transposing if collapsing is needed.
    if (!collapse_targets.empty()) {
        TableauTransposedRaii temp_transposed(inv_state);
        for (auto target : collapse_targets) {
            collapse_qubit(target, temp_transposed, sign_bias);
        }
    }
}

void TableauSimulator::collapse_qubit(
        size_t target,
        TableauTransposedRaii &transposed_raii,
        int8_t sign_bias) {
    auto n = inv_state.num_qubits;

    // Search for any stabilizer generator that anti-commutes with the measurement observable.
    size_t pivot = 0;
    while (pivot < n && !transposed_raii.tableau.zs.xt[pivot][target]) {
        pivot++;
    }
    if (pivot == n) {
        // No anti-commuting stabilizer generator. Measurement is deterministic.
        return;
    }

    // Perform partial Gaussian elimination over the stabilizer generators that anti-commute with the measurement.
    // Do this by introducing no-effect-because-control-is-zero CNOTs at the beginning of time.
    for (size_t k = pivot + 1; k < n; k++) {
        if (transposed_raii.tableau.zs.xt[k][target]) {
            transposed_raii.append_CX(pivot, k);
        }
    }

    // Swap the now-isolated anti-commuting stabilizer generator for one that commutes with the measurement.
    if (transposed_raii.tableau.zs.zt[pivot][target]) {
        transposed_raii.append_H_YZ(pivot);
    } else {
        transposed_raii.append_H_XZ(pivot);
    }

    // Assign a measurement result.
    bool result_if_measured = sign_bias == 0 ? (rng() & 1) : sign_bias < 0;
    if (inv_state.zs.signs[target] != result_if_measured) {
        transposed_raii.append_X(pivot);
    }
}
