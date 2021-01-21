#include <queue>

#include "gate_data.h"
#include "sim_tableau.h"

SimTableau::SimTableau(size_t num_qubits, std::mt19937_64 &rng) : inv_state(Tableau::identity(num_qubits)), rng(rng) {
}

bool SimTableau::is_deterministic(size_t target) const {
    return !inv_state.zs[target].x_ref.not_zero();
}

std::vector<bool> SimTableau::measure(const std::vector<size_t> &targets, float bias) {
    // Force all measurements to become deterministic.
    collapse_many(targets, bias);

    // Report deterministic measurement results.
    std::vector<bool> results(targets.size(), false);
    for (size_t t = 0; t < targets.size(); t++) {
        results[t] = inv_state.zs.signs[targets[t]];
    }
    return results;
}

void SimTableau::reset(const std::vector<size_t> &targets) {
    auto needs_reset = measure(targets);
    for (size_t k = 0; k < targets.size(); k++) {
        if (needs_reset[k]) {
            inv_state.prepend_X(targets[k]);
        }
    }
}

void SimTableau::H(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        inv_state.prepend_H_XZ(q);
    }
}

void SimTableau::H_XY(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        inv_state.prepend_H_XY(q);
    }
}

void SimTableau::H_YZ(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        inv_state.prepend_H_YZ(q);
    }
}

void SimTableau::SQRT_Z(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Z_DAG(q);
    }
}

void SimTableau::SQRT_Z_DAG(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Z(q);
    }
}

void SimTableau::SQRT_X(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_X_DAG(q);
    }
}

void SimTableau::SQRT_X_DAG(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_X(q);
    }
}

void SimTableau::SQRT_Y(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Y_DAG(q);
    }
}

void SimTableau::SQRT_Y_DAG(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Y(q);
    }
}

void SimTableau::CX(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_CX(c, t);
    }
}

void SimTableau::CY(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_CY(c, t);
    }
}

void SimTableau::CZ(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_CZ(c, t);
    }
}

void SimTableau::SWAP(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_SWAP(q1, q2);
    }
}

void SimTableau::ISWAP(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_ISWAP_DAG(q1, q2);
    }
}

void SimTableau::ISWAP_DAG(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_ISWAP(q1, q2);
    }
}

void SimTableau::XCX(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_XCX(q1, q2);
    }
}
void SimTableau::XCY(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_XCY(q1, q2);
    }
}
void SimTableau::XCZ(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_XCZ(q1, q2);
    }
}
void SimTableau::YCX(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_YCX(q1, q2);
    }
}
void SimTableau::YCY(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_YCY(q1, q2);
    }
}
void SimTableau::YCZ(const std::vector<size_t> &targets) {
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        inv_state.prepend_YCZ(q1, q2);
    }
}

void SimTableau::X(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        inv_state.prepend_X(q);
    }
}

void SimTableau::Y(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        inv_state.prepend_Y(q);
    }
}

void SimTableau::Z(const std::vector<size_t> &targets) {
    for (auto q : targets) {
        inv_state.prepend_Z(q);
    }
}

void SimTableau::broadcast_op(const std::string &name, const std::vector<size_t> &targets) {
    SIM_TABLEAU_GATE_FUNC_DATA.at(name)(*this, targets);
}

void SimTableau::tableau_op(const std::string &name, const std::vector<size_t> &targets) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend(GATE_TABLEAUS.at(GATE_INVERSE_NAMES.at(name)), targets);
}

std::vector<bool> SimTableau::simulate(const Circuit &circuit, std::mt19937_64 &rng) {
    SimTableau sim(circuit.num_qubits, rng);
    std::vector<bool> result;
    for (const auto &op : circuit.operations) {
        if (op.name == "M") {
            for (bool b : sim.measure(op.targets)) {
                result.push_back(b);
            }
        } else {
            SIM_TABLEAU_GATE_FUNC_DATA.at(op.name)(sim, op.targets);
        }
    }
    return result;
}

void SimTableau::ensure_large_enough_for_qubit(size_t q) {
    if (q < inv_state.num_qubits) {
        return;
    }
    inv_state.expand(q + 1);
}

void SimTableau::simulate(FILE *in, FILE *out, bool newline_after_measurement, std::mt19937_64 &rng) {
    CircuitReader reader(in);
    size_t max_qubit = 0;
    SimTableau sim(1, rng);
    while (reader.read_next_moment(newline_after_measurement)) {
        for (const auto &e : reader.operations) {
            for (size_t q : e.targets) {
                max_qubit = std::max(q, max_qubit);
            }
        }
        sim.ensure_large_enough_for_qubit(max_qubit);

        for (const auto &op : reader.operations) {
            if (op.name == "M") {
                for (bool b : sim.measure(op.targets)) {
                    putc_unlocked(b ? '1' : '0', out);
                }
                if (newline_after_measurement) {
                    putc_unlocked('\n', out);
                    fflush(out);
                }
            } else {
                SIM_TABLEAU_GATE_FUNC_DATA.at(op.name)(sim, op.targets);
            }
        }
    }
    if (!newline_after_measurement) {
        putc_unlocked('\n', out);
    }
}

SimVector SimTableau::to_vector_sim() const {
    auto inv = inv_state.inverse();
    std::vector<PauliStringRef> stabilizers;
    for (size_t k = 0; k < inv.num_qubits; k++) {
        stabilizers.push_back(inv.zs[k]);
    }
    return SimVector::from_stabilizers(stabilizers, rng);
}

void SimTableau::collapse_many(const std::vector<size_t> &targets, float bias) {
    std::vector<size_t> collapse_targets;
    collapse_targets.reserve(targets.size());
    for (auto target : targets) {
        if (!is_deterministic(target)) {
            collapse_targets.push_back(target);
        }
    }
    if (!collapse_targets.empty()) {
        TableauTransposedRaii temp_transposed(inv_state);
        for (auto target : collapse_targets) {
            collapse_while_transposed(target, temp_transposed, nullptr, bias);
        }
    }
}

std::vector<SparsePauliString> SimTableau::inspected_collapse(
        const std::vector<size_t> &targets) {
    std::vector<SparsePauliString> out(targets.size());

    std::queue<size_t> remaining;
    for (size_t k = 0; k < targets.size(); k++) {
        if (is_deterministic(targets[k])) {
            out[k].sign = inv_state.zs.signs[targets[k]];
        } else {
            remaining.push(k);
        }
    }
    if (!remaining.empty()) {
        TableauTransposedRaii temp_transposed(inv_state);
        do {
            auto k = remaining.front();
            remaining.pop();
            collapse_while_transposed(targets[k], temp_transposed, &out[k], -1);
        } while (!remaining.empty());
    }

    return out;
}

void SimTableau::collapse_while_transposed(
        size_t target,
        TableauTransposedRaii &temp_transposed,
        SparsePauliString *destabilizer_out,
        float else_bias) {
    auto n = inv_state.num_qubits;

    // Find an anti-commuting part of the measurement observable's at the start of time.
    size_t pivot = 0;
    while (pivot < n && !temp_transposed.tableau.zs.xt[pivot][target]) {
        pivot++;
    }
    if (pivot == n) {
        // No anti-commuting part. Already collapsed.
        if (destabilizer_out != nullptr) {
            destabilizer_out->sign = temp_transposed.tableau.zs.signs[target];
        }
        return;
    }

    // Introduce no-op CNOTs at the start of time to remove all anti-commuting parts except for one.
    for (size_t k = pivot + 1; k < n; k++) {
        auto x = temp_transposed.tableau.zs.xt[k][target];
        if (x) {
            temp_transposed.append_CX(pivot, k);
        }
    }

    // Collapse the anti-commuting part.
    if (temp_transposed.tableau.zs.zt[pivot][target]) {
        temp_transposed.append_H_YZ(pivot);
    } else {
        temp_transposed.append_H(pivot);
    }
    bool sign = temp_transposed.tableau.zs.signs[target];
    if (destabilizer_out != nullptr) {
        *destabilizer_out = PauliStringRef(
                n,
                bit_ref(&sign, 0),
                temp_transposed.tableau.zs[pivot].z_ref,
                temp_transposed.tableau.xs[pivot].z_ref
        ).sparse();
    } else {
        bool sign = temp_transposed.tableau.zs.signs[target];
        auto coin_flip = std::bernoulli_distribution(else_bias)(rng);
        if (sign != coin_flip) {
            temp_transposed.append_X(pivot);
        }
    }
}
