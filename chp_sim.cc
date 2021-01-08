#include "chp_sim.h"

ChpSim::ChpSim(size_t num_qubits) : inv_state(Tableau::identity(num_qubits)), rng((std::random_device {})()) {
}

bool ChpSim::is_deterministic(size_t target) const {
    size_t n = inv_state.num_qubits;
    auto p = inv_state.z_obs_ptr(target);
    return !any_non_zero((__m256i *)p._x, ceil256(n) >> 8, p.stride256);
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
        BlockTransposedTableau temp_transposed(inv_state);
        for (size_t k = 0; k < targets.size(); k++) {
            if (!finished[k]) {
                results[k] = measure_while_block_transposed(temp_transposed, targets[k], bias);
            }
        }
    }

    return results;
}

bool ChpSim::measure(size_t target, float bias) {
    if (is_deterministic(target)) {
        return inv_state.z_sign(target);
    } else {
        BlockTransposedTableau temp_transposed(inv_state);
        return measure_while_block_transposed(temp_transposed, target, bias);
    }
}

bool ChpSim::measure_while_block_transposed(BlockTransposedTableau &block_transposed, size_t target, float bias) {
    size_t n = block_transposed.tableau.num_qubits;

    for (size_t q = n - 1; q > 0; q--) {
        size_t victim = q >> 1;
        if (block_transposed.z_obs_x_bit(target, q)) {
            if (block_transposed.z_obs_x_bit(target, victim)) {
                block_transposed.append_CX(victim, q);
            } else {
                block_transposed.append_SWAP(victim, q);
            }
        }
    }
    if (!block_transposed.z_obs_x_bit(target, 0)) {
        // Deterministic result.
        return block_transposed.z_sign(target);
    }

    // Collapse the state.
    if (block_transposed.z_obs_z_bit(target, 0)) {
        block_transposed.append_H_YZ(0);
    } else {
        block_transposed.append_H(0);
    }

    auto coin_flip = std::bernoulli_distribution(bias)(rng);
    if (block_transposed.z_sign(target) != coin_flip) {
        block_transposed.append_X(0);
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
    inv_state.inplace_scatter_prepend(GATE_TABLEAUS.at(name), targets);
}
