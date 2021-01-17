#include "sim_tableau.h"
#include <queue>

SimTableau::SimTableau(size_t num_qubits) : inv_state(Tableau::identity(num_qubits)), rng((std::random_device {})()) {
}

bool SimTableau::is_deterministic(size_t target) const {
    size_t n = inv_state.num_qubits;
    auto p = inv_state.z_obs_ptr(target);
    return !any_non_zero((__m256i *)p._x, ceil256(n) >> 8);
}

std::vector<bool> SimTableau::measure_many(const std::vector<size_t> &targets, float bias) {
    // Force all measurements to become deterministic.
    collapse_many(targets, bias);

    // Report deterministic measurement results.
    std::vector<bool> results(targets.size(), false);
    for (size_t t = 0; t < targets.size(); t++) {
        results[t] = inv_state.z_sign(targets[t]);
    }
    return results;
}

void SimTableau::reset_many(const std::vector<size_t> &targets) {
    auto r = measure_many(targets);
    for (size_t k = 0; k < targets.size(); k++) {
        if (r[k]) {
            X(targets[k]);
        }
    }
}

void SimTableau::reset(size_t target) {
    if (measure(target)) {
        X(target);
    }
}

bool SimTableau::measure(size_t target, float bias) {
    return measure_many({target}, bias)[0];
}

void SimTableau::H(size_t q) {
    inv_state.prepend_H(q);
}

void SimTableau::H_XY(size_t q) {
    inv_state.prepend_H_XY(q);
}

void SimTableau::H_YZ(size_t q) {
    inv_state.prepend_H_YZ(q);
}

void SimTableau::SQRT_Z(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_Z_DAG(q);
}

void SimTableau::SQRT_Z_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_Z(q);
}

void SimTableau::SQRT_X(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_X_DAG(q);
}

void SimTableau::SQRT_X_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_X(q);
}

void SimTableau::SQRT_Y(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_Y_DAG(q);
}

void SimTableau::SQRT_Y_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_Y(q);
}

void SimTableau::CX(size_t c, size_t t) {
    inv_state.prepend_CX(c, t);
}

void SimTableau::CY(size_t c, size_t t) {
    inv_state.prepend_CY(c, t);
}

void SimTableau::CZ(size_t c, size_t t) {
    inv_state.prepend_CZ(c, t);
}

void SimTableau::SWAP(size_t q1, size_t q2) {
    inv_state.prepend_SWAP(q1, q2);
}

void SimTableau::ISWAP(size_t q1, size_t q2) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_ISWAP_DAG(q1, q2);
}

void SimTableau::ISWAP_DAG(size_t q1, size_t q2) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_ISWAP(q1, q2);
}

void SimTableau::XCX(size_t q1, size_t q2) {
    inv_state.prepend_XCX(q1, q2);
}
void SimTableau::XCY(size_t q1, size_t q2) {
    inv_state.prepend_XCY(q1, q2);
}
void SimTableau::XCZ(size_t q1, size_t q2) {
    inv_state.prepend_XCZ(q1, q2);
}
void SimTableau::YCX(size_t q1, size_t q2) {
    inv_state.prepend_YCX(q1, q2);
}
void SimTableau::YCY(size_t q1, size_t q2) {
    inv_state.prepend_YCY(q1, q2);
}
void SimTableau::YCZ(size_t q1, size_t q2) {
    inv_state.prepend_YCZ(q1, q2);
}

void SimTableau::X(size_t q) {
    inv_state.prepend_X(q);
}

void SimTableau::Y(size_t q) {
    inv_state.prepend_Y(q);
}

void SimTableau::Z(size_t q) {
    inv_state.prepend_Z(q);
}

void SimTableau::func_op(const std::string &name, const std::vector<size_t> &targets) {
    if (targets.size() == 1) {
        SINGLE_QUBIT_GATE_FUNCS.at(name)(*this, targets[0]);
    } else if (targets.size() == 2) {
        TWO_QUBIT_GATE_FUNCS.at(name)(*this, targets[0], targets[1]);
    } else {
        throw std::out_of_range("Unknown SimTableau op " + name);
    }
}

void SimTableau::tableau_op(const std::string &name, const std::vector<size_t> &targets) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend(GATE_TABLEAUS.at(GATE_INVERSE_NAMES.at(name)), targets);
}

std::vector<bool> SimTableau::simulate(const Circuit &circuit) {
    SimTableau sim(circuit.num_qubits);
    std::vector<bool> result;
    for (const auto &op : circuit.operations) {
        if (op.name == "M") {
            for (bool b : sim.measure_many(op.targets)) {
                result.push_back(b);
            }
        } else if (op.name == "R") {
            sim.reset_many(op.targets);
        } else if (op.targets.size() == 1) {
            SINGLE_QUBIT_GATE_FUNCS.at(op.name)(sim, op.targets[0]);
        } else if (op.targets.size() == 2) {
            TWO_QUBIT_GATE_FUNCS.at(op.name)(sim, op.targets[0], op.targets[1]);
        } else {
            throw std::runtime_error("Unsupported operation " + op.name);
        }
    }
    return result;
}

void SimTableau::ensure_large_enough_for_qubit(size_t q) {
    if (q < inv_state.num_qubits) {
        return;
    }
    inv_state.expand(ceil256(q + 1));
}

void SimTableau::simulate(FILE *in, FILE *out) {
    CircuitReader reader(in);
    size_t max_qubit = 0;
    SimTableau sim(1);
    while (reader.read_next_moment()) {
        for (const auto &e : reader.operations) {
            for (size_t q : e.targets) {
                max_qubit = std::max(q, max_qubit);
            }
        }
        sim.ensure_large_enough_for_qubit(max_qubit);

        for (const auto &op : reader.operations) {
            if (op.name == "M") {
                for (bool b : sim.measure_many(op.targets)) {
                    putc_unlocked(b ? '1' : '0', out);
                    putc_unlocked('\n', out);
                }
            } else if (op.name == "R") {
                sim.reset_many(op.targets);
            } else if (op.targets.size() == 1) {
                SINGLE_QUBIT_GATE_FUNCS.at(op.name)(sim, op.targets[0]);
            } else if (op.targets.size() == 2) {
                TWO_QUBIT_GATE_FUNCS.at(op.name)(sim, op.targets[0], op.targets[1]);
            } else {
                throw std::runtime_error("Unsupported operation " + op.name);
            }
        }
    }
}

SimVector SimTableau::to_vector_sim() const {
    auto inv = inv_state.inverse();
    std::vector<PauliStringPtr> stabilizers;
    for (size_t k = 0; k < inv.num_qubits; k++) {
        stabilizers.push_back(inv.z_obs_ptr(k));
    }
    return SimVector::from_stabilizers(stabilizers);
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
        TempTransposedTableauRaii temp_transposed(inv_state);
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
            out[k].sign = inv_state.z_sign(k);
        } else {
            remaining.push(k);
        }
    }
    if (!remaining.empty()) {
        TempTransposedTableauRaii temp_transposed(inv_state);
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
        TempTransposedTableauRaii &temp_transposed,
        SparsePauliString *destabilizer_out,
        float else_bias) {
    auto n = inv_state.num_qubits;

    // Find an anti-commuting part of the measurement observable's at the start of time.
    size_t pivot = 0;
    while (pivot < n && !temp_transposed.z_obs_x_bit(target, pivot)) {
        pivot++;
    }
    if (pivot == n) {
        // No anti-commuting part. Already collapsed.
        if (destabilizer_out != nullptr) {
            destabilizer_out->sign = temp_transposed.z_sign(target);
        }
        return;
    }

    // Introduce no-op CNOTs at the start of time to remove all anti-commuting parts except for one.
    for (size_t k = pivot + 1; k < n; k++) {
        auto x = temp_transposed.z_obs_x_bit(target, k);
        if (x) {
            temp_transposed.append_CX(pivot, k);
        }
    }

    // Collapse the anti-commuting part.
    if (temp_transposed.z_obs_z_bit(target, pivot)) {
        temp_transposed.append_H_YZ(pivot);
    } else {
        temp_transposed.append_H(pivot);
    }
    bool sign = temp_transposed.z_sign(target);
    if (destabilizer_out != nullptr) {
        auto t = temp_transposed.transposed_xz_ptr(pivot);
        *destabilizer_out = PauliStringPtr(
                n,
                BitPtr(&sign, 0),
                (uint64_t *) t.xz[1].z,
                (uint64_t *) t.xz[0].z).sparse();
    } else {
        auto coin_flip = std::bernoulli_distribution(else_bias)(rng);
        if (sign != coin_flip) {
            temp_transposed.append_X(pivot);
        }
    }
}

const std::unordered_map<std::string, std::function<void(SimTableau &, size_t)>> SINGLE_QUBIT_GATE_FUNCS{
        {"M",          [](SimTableau &sim, size_t q) { sim.measure(q); }},
        {"R",          &SimTableau::reset},
        {"I",          [](SimTableau &sim, size_t q) {}},
        // Pauli gates.
        {"X",          &SimTableau::X},
        {"Y",          &SimTableau::Y},
        {"Z",          &SimTableau::Z},
        // Axis exchange gates.
        {"H",          &SimTableau::H},
        {"H_XY",       &SimTableau::H_XY},
        {"H_XZ",       &SimTableau::H},
        {"H_YZ",       &SimTableau::H_YZ},
        // 90 degree rotation gates.
        {"SQRT_X",     &SimTableau::SQRT_X},
        {"SQRT_X_DAG", &SimTableau::SQRT_X_DAG},
        {"SQRT_Y",     &SimTableau::SQRT_Y},
        {"SQRT_Y_DAG", &SimTableau::SQRT_Y_DAG},
        {"SQRT_Z",     &SimTableau::SQRT_Z},
        {"SQRT_Z_DAG", &SimTableau::SQRT_Z_DAG},
        {"S",          &SimTableau::SQRT_Z},
        {"S_DAG",      &SimTableau::SQRT_Z_DAG},
};

const std::unordered_map<std::string, std::function<void(SimTableau &, size_t, size_t)>> TWO_QUBIT_GATE_FUNCS {
    {"SWAP", &SimTableau::SWAP},
    {"ISWAP", &SimTableau::ISWAP},
    {"ISWAP_DAG", &SimTableau::ISWAP_DAG},
    {"CNOT", &SimTableau::CX},
    {"CX", &SimTableau::CX},
    {"CY", &SimTableau::CY},
    {"CZ", &SimTableau::CZ},
    // Controlled interactions in other bases.
    {"XCX", &SimTableau::XCX},
    {"XCY", &SimTableau::XCY},
    {"XCZ", &SimTableau::XCZ},
    {"YCX", &SimTableau::YCX},
    {"YCY", &SimTableau::YCY},
    {"YCZ", &SimTableau::YCZ},
};
