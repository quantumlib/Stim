#include "chp_sim.h"

ChpSim::ChpSim(size_t num_qubits) : inv_state(Tableau::identity(num_qubits)), rng((std::random_device {})()) {
}

bool ChpSim::is_deterministic(size_t target) const {
    size_t n = inv_state.num_qubits;
    auto p = inv_state.z_obs_ptr(target);
    return !any_non_zero((__m256i *)p._x, ceil256(n) >> 8, p.stride256);
}

size_t ChpSim::find_pivot(size_t target) {
    const auto &p = inv_state.z_obs_ptr(target);
    for (size_t q = 0; q < p.num_words256(); q++) {
        for (size_t k = 0; k < 4; k++) {
            uint64_t v = p._x[q * p.stride256 * 4 + k];
            if (v) {
                for (size_t i = 0; i < 64; i++) {
                    if ((v >> i) & 1) {
                        return q*256 + k*64 + i;
                    }
                }
            }
        }
    }
    return SIZE_MAX;
}

std::vector<bool> ChpSim::measure_many(const std::vector<size_t> &targets, float bias) {
    std::vector<size_t> pivots;
    pivots.reserve(targets.size());
    for (size_t k = 0; k < targets.size(); k++) {
        pivots.push_back(find_pivot(targets[k]));
    }

    std::vector<bool> results;
    results.reserve(targets.size());
    for (size_t k = 0; k < targets.size(); k++) {
        results.push_back(measure_given_pivot(targets[k], pivots[k], bias));
    }
    return results;
}

bool ChpSim::measure(size_t target, float bias) {
    return measure_given_pivot(target, find_pivot(target), bias);
}

bool ChpSim::measure_given_pivot(size_t target, size_t pivot, float bias) {
    size_t n = inv_state.num_qubits;
    if (pivot == SIZE_MAX) {
        // Deterministic result.
        return inv_state.z_sign(target);
    }

    // Cancel out other X / Y components.
    for (size_t q = pivot + 1; q < n; q++) {
        if (inv_state.z_obs_x_bit(target, q)) {
            inv_state.inplace_scatter_append_CX(pivot, q);
        }
    }

    // Collapse the state.
    if (inv_state.z_obs_z_bit(target, pivot)) {
        inv_state.inplace_scatter_append_H_YZ(pivot);
    } else {
        inv_state.inplace_scatter_append_H(pivot);
    }

    auto coin_flip = std::bernoulli_distribution(bias)(rng);
    if (inv_state.z_sign(target) != coin_flip) {
        inv_state.inplace_scatter_append_X(pivot);
    }

    return coin_flip;
}

void ChpSim::H(size_t q) {
    inv_state.inplace_scatter_prepend_H(q);
}

void ChpSim::H_XY(size_t q) {
    inv_state.inplace_scatter_prepend_H_XY(q);
}

void ChpSim::H_YZ(size_t q) {
    inv_state.inplace_scatter_prepend_H_YZ(q);
}

void ChpSim::SQRT_Z(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend_SQRT_Z_DAG(q);
}

void ChpSim::SQRT_Z_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend_SQRT_Z(q);
}

void ChpSim::SQRT_X(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend_SQRT_X_DAG(q);
}

void ChpSim::SQRT_X_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend_SQRT_X(q);
}

void ChpSim::SQRT_Y(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend_SQRT_Y_DAG(q);
}

void ChpSim::SQRT_Y_DAG(size_t q) {
    // Note: inverted because we're tracking the inverse tableau.
    inv_state.inplace_scatter_prepend_SQRT_Y(q);
}

void ChpSim::CX(size_t c, size_t t) {
    inv_state.inplace_scatter_prepend_CX(c, t);
}

void ChpSim::CY(size_t c, size_t t) {
    inv_state.inplace_scatter_prepend_CY(c, t);
}

void ChpSim::CZ(size_t c, size_t t) {
    inv_state.inplace_scatter_prepend_CZ(c, t);
}

void ChpSim::X(size_t q) {
    inv_state.inplace_scatter_prepend_X(q);
}

void ChpSim::Y(size_t q) {
    inv_state.inplace_scatter_prepend_Y(q);
}

void ChpSim::Z(size_t q) {
    inv_state.inplace_scatter_prepend_Z(q);
}

void ChpSim::op(const std::string &name, const std::vector<size_t> &targets) {
    inv_state.inplace_scatter_prepend(GATE_TABLEAUS.at(name), targets);
}
