#include "chp_sim.h"

ChpSim::ChpSim(size_t num_qubits) : inv_state(Tableau::identity(num_qubits)), rng((std::random_device {})()) {
}

bool ChpSim::is_deterministic(size_t target) const {
    size_t n = inv_state.num_qubits;
    auto p = inv_state.z_obs_ptr(target);
    return !any_non_zero((__m256i *)p._x, ceil256(n) >> 8);
}

std::vector<bool> ChpSim::measure_many(const std::vector<size_t> &targets, float bias) {
    std::vector<bool> finished(targets.size(), false);
    std::vector<bool> results(targets.size(), false);

    // Note deterministic measurements.
    bool any_random = false;
    for (size_t k = 0; k < targets.size(); k++) {
        if (is_deterministic(targets[k])) {
            finished[k] = true;
            results[k] = inv_state.z_sign(targets[k]);
        } else {
            any_random = true;
        }
    }

    // Handle remaining random measurements.
    if (any_random) {
        TempBlockTransposedTableauRaii temp_transposed(inv_state);
        for (size_t k = 0; k < targets.size(); k++) {
            if (!finished[k]) {
                results[k] = measure_while_block_transposed(temp_transposed, targets[k], bias);
            }
        }
    }

    return results;
}

void ChpSim::reset_many(const std::vector<size_t> &targets) {
    auto r = measure_many(targets);
    for (size_t k = 0; k < targets.size(); k++) {
        if (r[k]) {
            X(targets[k]);
        }
    }
}

void ChpSim::reset(size_t target) {
    if (measure(target)) {
        X(target);
    }
}

bool ChpSim::measure(size_t target, float bias) {
    if (is_deterministic(target)) {
        return inv_state.z_sign(target);
    } else {
        TempBlockTransposedTableauRaii temp_transposed(inv_state);
        return measure_while_block_transposed(temp_transposed, target, bias);
    }
}

bool ChpSim::measure_while_block_transposed(TempBlockTransposedTableauRaii &block_transposed, size_t target, float bias) {
    size_t n = block_transposed.tableau.num_qubits;
    size_t pivot = 0;
    while (pivot < n && !block_transposed.z_obs_x_bit(target, pivot)) {
        pivot++;
    }
    if (pivot == n) {
        // Deterministic result.
        return block_transposed.z_sign(target);
    }

    // Isolate to a single qubit.
    for (size_t victim = pivot + 1; victim < n; victim++) {
        bool x = block_transposed.z_obs_x_bit(target, victim);
        if (x) {
            block_transposed.append_CX(pivot, victim);
        }
    }

    // Collapse the state.
    if (block_transposed.z_obs_z_bit(target, pivot)) {
        block_transposed.append_H_YZ(pivot);
    } else {
        block_transposed.append_H(pivot);
    }

    auto coin_flip = std::bernoulli_distribution(bias)(rng);
    if (block_transposed.z_sign(target) != coin_flip) {
        block_transposed.append_X(pivot);
    }

    return coin_flip;
}

void ChpSim::H(size_t q) {
    inv_state.prepend_H(q);
}

void ChpSim::H_XY(size_t q) {
    inv_state.prepend_H_XY(q);
}

void ChpSim::H_YZ(size_t q) {
    inv_state.prepend_H_YZ(q);
}

void ChpSim::SQRT_Z(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_Z_DAG(q);
}

void ChpSim::SQRT_Z_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_Z(q);
}

void ChpSim::SQRT_X(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_X_DAG(q);
}

void ChpSim::SQRT_X_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_X(q);
}

void ChpSim::SQRT_Y(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_Y_DAG(q);
}

void ChpSim::SQRT_Y_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.prepend_SQRT_Y(q);
}

void ChpSim::CX(size_t c, size_t t) {
    inv_state.prepend_CX(c, t);
}

void ChpSim::CY(size_t c, size_t t) {
    inv_state.prepend_CY(c, t);
}

void ChpSim::CZ(size_t c, size_t t) {
    inv_state.prepend_CZ(c, t);
}

void ChpSim::SWAP(size_t q1, size_t q2) {
    inv_state.prepend_SWAP(q1, q2);
}

void ChpSim::X(size_t q) {
    inv_state.prepend_X(q);
}

void ChpSim::Y(size_t q) {
    inv_state.prepend_Y(q);
}

void ChpSim::Z(size_t q) {
    inv_state.prepend_Z(q);
}

void ChpSim::op(const std::string &name, const std::vector<size_t> &targets) {
    inv_state.inplace_scatter_prepend(GATE_TABLEAUS.at(GATE_INVERSE_NAMES.at(name)), targets);
}

std::vector<bool> ChpSim::simulate(const Circuit &circuit) {
    ChpSim sim(circuit.num_qubits);
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

void ChpSim::ensure_large_enough_for_qubit(size_t q) {
    if (q < inv_state.num_qubits) {
        return;
    }
    inv_state.expand(ceil256(q + 1));
}

void ChpSim::simulate(FILE *in, FILE *out) {
    CircuitReader reader(in);
    size_t max_qubit = 0;
    ChpSim sim(1);
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

const std::unordered_map<std::string, std::function<void(ChpSim &, size_t)>> SINGLE_QUBIT_GATE_FUNCS{
        {"M",          [](ChpSim &sim, size_t q) { sim.measure(q); }},
        {"R",          &ChpSim::reset},
        {"I",          [](ChpSim &sim, size_t q) {}},
        // Pauli gates.
        {"X",          &ChpSim::X},
        {"Y",          &ChpSim::Y},
        {"Z",          &ChpSim::Z},
        // Axis exchange gates.
        {"H",          &ChpSim::H},
        {"H_XY",       &ChpSim::H_XY},
        {"H_XZ",       &ChpSim::H},
        {"H_YZ",       &ChpSim::H_YZ},
        // 90 degree rotation gates.
        {"SQRT_X",     &ChpSim::SQRT_X},
        {"SQRT_X_DAG", &ChpSim::SQRT_X_DAG},
        {"SQRT_Y",     &ChpSim::SQRT_Y},
        {"SQRT_Y_DAG", &ChpSim::SQRT_Y_DAG},
        {"SQRT_Z",     &ChpSim::SQRT_Z},
        {"SQRT_Z_DAG", &ChpSim::SQRT_Z_DAG},
        {"S",          &ChpSim::SQRT_Z},
        {"S_DAG",      &ChpSim::SQRT_Z_DAG},
};

const std::unordered_map<std::string, std::function<void(ChpSim &, size_t, size_t)>> TWO_QUBIT_GATE_FUNCS {
    {"SWAP", &ChpSim::SWAP},
    {"CNOT", &ChpSim::CX},
    {"CX", &ChpSim::CX},
    {"CY", &ChpSim::CY},
    {"CZ", &ChpSim::CZ},
    // Controlled interactions in other bases.
    {"XCX", [](ChpSim &sim, size_t q1, size_t q2) { sim.op("XCX", {q1, q2}); }},
    {"XCY", [](ChpSim &sim, size_t q1, size_t q2) { sim.op("XCY", {q1, q2}); }},
    {"XCZ", [](ChpSim &sim, size_t q1, size_t q2) { sim.op("XCZ", {q1, q2}); }},
    {"YCX", [](ChpSim &sim, size_t q1, size_t q2) { sim.op("YCX", {q1, q2}); }},
    {"YCY", [](ChpSim &sim, size_t q1, size_t q2) { sim.op("YCY", {q1, q2}); }},
    {"YCZ", [](ChpSim &sim, size_t q1, size_t q2) { sim.op("YCZ", {q1, q2}); }},
};
