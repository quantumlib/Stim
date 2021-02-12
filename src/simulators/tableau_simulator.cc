// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tableau_simulator.h"

#include <queue>

#include "../circuit/gate_data.h"
#include "../probability_util.h"

TableauSimulator::TableauSimulator(size_t num_qubits, std::mt19937_64 &rng, int8_t sign_bias)
    : inv_state(Tableau::identity(num_qubits)), rng(rng), sign_bias(sign_bias), recorded_measurement_results(), last_correlated_error_occurred(false) {
}

bool TableauSimulator::is_deterministic(size_t target) const {
    return !inv_state.zs[target].xs.not_zero();
}

void TableauSimulator::measure(const OperationData &target_data) {
    // Ensure measurement observables are collapsed.
    collapse(target_data);

    // Record measurement results.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_QUBIT_MASK;
        bool flipped = qf & TARGET_INVERTED_MASK;
        recorded_measurement_results.push(inv_state.zs.signs[q] ^ flipped);
    }
}

void TableauSimulator::measure_reset(const OperationData &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.

    // Ensure measurement observables are collapsed.
    collapse(target_data);

    // Record measurement results while triggering resets.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_QUBIT_MASK;
        bool flipped = qf & TARGET_INVERTED_MASK;
        recorded_measurement_results.push(inv_state.zs.signs[q] ^ flipped);
        inv_state.zs.signs[q] = false;
    }
}

void TableauSimulator::reset(const OperationData &target_data) {
    // Collapse the qubits to be reset.
    collapse(target_data);

    // Force the collapsed qubits into the ground state.
    for (auto q : target_data.targets) {
        inv_state.zs.signs[q] = false;
    }
}

void TableauSimulator::I(const OperationData &target_data) {
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

void TableauSimulator::ZCX(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_ZCX(c, t);
    }
}

void TableauSimulator::ZCY(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_ZCY(c, t);
    }
}

void TableauSimulator::ZCZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_ZCZ(c, t);
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

void TableauSimulator::DEPOLARIZE1(const OperationData &target_data) {
    RareErrorIterator::for_samples(target_data.arg, target_data.targets, rng, [&](size_t q) {
        auto p = 1 + (rng() % 3);
        inv_state.xs.signs[q] ^= p & 1;
        inv_state.zs.signs[q] ^= p & 2;
    });
}

void TableauSimulator::DEPOLARIZE2(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    auto n = targets.size() >> 1;
    RareErrorIterator::for_samples(target_data.arg, n, rng, [&](size_t s) {
        auto p = 1 + (rng() % 15);
        auto q1 = targets[s << 1];
        auto q2 = targets[1 | (s << 1)];
        inv_state.xs.signs[q1] ^= p & 1;
        inv_state.zs.signs[q1] ^= p & 2;
        inv_state.xs.signs[q2] ^= p & 4;
        inv_state.zs.signs[q2] ^= p & 8;
    });
}

void TableauSimulator::X_ERROR(const OperationData &target_data) {
    RareErrorIterator::for_samples(target_data.arg, target_data.targets, rng, [&](size_t q) {
        inv_state.xs.signs[q] ^= true;
    });
}

void TableauSimulator::Y_ERROR(const OperationData &target_data) {
    RareErrorIterator::for_samples(target_data.arg, target_data.targets, rng, [&](size_t q) {
        inv_state.xs.signs[q] ^= true;
        inv_state.zs.signs[q] ^= true;
    });
}

void TableauSimulator::Z_ERROR(const OperationData &target_data) {
    RareErrorIterator::for_samples(target_data.arg, target_data.targets, rng, [&](size_t q) {
        inv_state.zs.signs[q] ^= true;
    });
}

void TableauSimulator::CORRELATED_ERROR(const OperationData &target_data) {
    last_correlated_error_occurred = false;
    ELSE_CORRELATED_ERROR(target_data);
}

void TableauSimulator::ELSE_CORRELATED_ERROR(const OperationData &target_data) {
    if (last_correlated_error_occurred) {
        return;
    }
    last_correlated_error_occurred = std::bernoulli_distribution(target_data.arg)(rng);
    if (!last_correlated_error_occurred) {
        return;
    }
    for (auto qxz : target_data.targets) {
        auto q = qxz & TARGET_QUBIT_MASK;
        auto x = qxz & TARGET_PAULI_X_MASK;
        auto z = qxz & TARGET_PAULI_Z_MASK;
        if (x) {
            inv_state.prepend_X(q);
        }
        if (z) {
            inv_state.prepend_Z(q);
        }
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

simd_bits TableauSimulator::sample_circuit(const Circuit &circuit, std::mt19937_64 &rng, int8_t sign_bias) {
    TableauSimulator sim(circuit.num_qubits, rng, sign_bias);
    for (const auto &op : circuit.operations) {
        (sim.*op.gate->tableau_simulator_function)(op.target_data);
    }

    assert(sim.recorded_measurement_results.size() == circuit.num_measurements);
    simd_bits result(circuit.num_measurements);
    for (size_t k = 0; k < circuit.num_measurements; k++) {
        result[k] = sim.recorded_measurement_results.front();
        sim.recorded_measurement_results.pop();
    }
    return result;
}

void TableauSimulator::ensure_large_enough_for_qubits(size_t num_qubits) {
    if (num_qubits <= inv_state.num_qubits) {
        return;
    }
    inv_state.expand(num_qubits);
}

void TableauSimulator::sample_stream(FILE *in, FILE *out, bool newline_after_measurements, std::mt19937_64 &rng) {
    Circuit unprocessed;
    TableauSimulator sim(1, rng);
    while (unprocessed.append_from_file(in, newline_after_measurements)) {
        sim.ensure_large_enough_for_qubits(unprocessed.num_qubits);

        for (const auto &op : unprocessed.operations) {
            (sim.*op.gate->tableau_simulator_function)(op.target_data);
            while (!sim.recorded_measurement_results.empty()) {
                putc('0' + sim.recorded_measurement_results.front(), out);
                sim.recorded_measurement_results.pop();
            }
            if (newline_after_measurements && (op.gate->flags & GATE_PRODUCES_RESULTS)) {
                putc('\n', out);
                fflush(out);
            }
        }

        unprocessed.clear();
    }
    if (!newline_after_measurements) {
        putc('\n', out);
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

void TableauSimulator::collapse(const OperationData &target_data) {
    // Find targets that need to be collapsed.
    std::vector<size_t> collapse_targets;
    collapse_targets.reserve(target_data.targets.size());
    for (auto t : target_data.targets) {
        t &= TARGET_QUBIT_MASK;
        if (!is_deterministic(t)) {
            collapse_targets.push_back(t);
        }
    }

    // Only pay the cost of transposing if collapsing is needed.
    if (!collapse_targets.empty()) {
        TableauTransposedRaii temp_transposed(inv_state);
        for (auto target : collapse_targets) {
            collapse_qubit(target, temp_transposed);
        }
    }
}

void TableauSimulator::collapse_qubit(size_t target, TableauTransposedRaii &transposed_raii) {
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
            transposed_raii.append_ZCX(pivot, k);
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

simd_bits TableauSimulator::reference_sample_circuit(const Circuit &circuit) {
    Circuit filtered;
    std::vector<Operation> deterministic_operations{};
    for (const auto &op : circuit.operations) {
        if (!(op.gate->flags & GATE_IS_NOISE)) {
            filtered.append_operation(op);
        }
    }

    std::mt19937_64 irrelevant_rng(0);
    return TableauSimulator::sample_circuit(filtered, irrelevant_rng, +1);
}
