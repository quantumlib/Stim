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

#include "../circuit/gate_data.h"
#include "../probability_util.h"

TableauSimulator::TableauSimulator(size_t num_qubits, std::mt19937_64 &rng, int8_t sign_bias)
    : inv_state(Tableau::identity(num_qubits)),
      rng(rng),
      sign_bias(sign_bias),
      measurement_record(),
      last_correlated_error_occurred(false) {
}

bool TableauSimulator::is_deterministic(size_t target) const {
    return !inv_state.zs[target].xs.not_zero();
}

void TableauSimulator::measure(const OperationData &target_data) {
    // Ensure measurement observables are collapsed.
    collapse(target_data);

    // Record measurement results.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_VALUE_MASK;
        bool flipped = qf & TARGET_INVERTED_BIT;
        bool b = inv_state.zs.signs[q] ^ flipped;
        measurement_record.push_back(b);
    }
}

void TableauSimulator::measure_reset(const OperationData &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.

    // Ensure measurement observables are collapsed.
    collapse(target_data);

    // Record measurement results while triggering resets.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_VALUE_MASK;
        bool flipped = qf & TARGET_INVERTED_BIT;
        bool b = inv_state.zs.signs[q] ^ flipped;
        measurement_record.push_back(b);
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

bool TableauSimulator::read_measurement_record(uint32_t encoded_target) const {
    uint8_t b = encoded_target ^ TARGET_RECORD_BIT;
    if (b == 0 || b > measurement_record.size()) {
        throw std::out_of_range("Referred to a measurement record before the beginning of time.");
    }
    return measurement_record[measurement_record.size() - b];
}

void TableauSimulator::single_cx(uint32_t c, uint32_t t) {
    if (!((c | t) & TARGET_RECORD_BIT)) {
        inv_state.prepend_ZCX(c, t);
    } else if (t & TARGET_RECORD_BIT) {
        throw std::out_of_range("Measurement record editing is not supported.");
    } else {
        if (read_measurement_record(c)) {
            inv_state.prepend_X(t);
        }
    }
}

void TableauSimulator::single_cy(uint32_t c, uint32_t t) {
    if (!((c | t) & TARGET_RECORD_BIT)) {
        inv_state.prepend_ZCY(c, t);
    } else if (t & TARGET_RECORD_BIT) {
        throw std::out_of_range("Measurement record editing is not supported.");
    } else {
        if (read_measurement_record(c)) {
            inv_state.prepend_Y(t);
        }
    }
}

void TableauSimulator::ZCX(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k], targets[k + 1]);
    }
}

void TableauSimulator::ZCY(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k], targets[k + 1]);
    }
}

void TableauSimulator::ZCZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k];
        auto q2 = targets[k + 1];
        if (!((q1 | q2) & TARGET_RECORD_BIT)) {
            inv_state.prepend_ZCZ(q1, q2);
            continue;
        } else if ((q1 & q2) & TARGET_RECORD_BIT) {
            // No-op.
        } else if (q1 & TARGET_RECORD_BIT) {
            if (read_measurement_record(q1)) {
                inv_state.prepend_Z(q2);
            }
        } else if (q2 & TARGET_RECORD_BIT) {
            if (read_measurement_record(q2)) {
                inv_state.prepend_Z(q1);
            }
        }
    }
}

void TableauSimulator::SWAP(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k];
        auto t = targets[k + 1];
        inv_state.prepend_SWAP(c, t);
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
        single_cx(targets[k + 1], targets[k]);
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
        single_cy(targets[k + 1], targets[k]);
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
        auto q = qxz & TARGET_VALUE_MASK;
        if (qxz & TARGET_PAULI_X_BIT) {
            inv_state.prepend_X(q);
        }
        if (qxz & TARGET_PAULI_Z_BIT) {
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
    TableauSimulator sim(circuit.count_qubits(), rng, sign_bias);
    circuit.for_each_operation([&](const Operation &op) {
        (sim.*op.gate->tableau_simulator_function)(op.target_data);
    });

    simd_bits result(sim.measurement_record.size());
    for (size_t k = 0; k < sim.measurement_record.size(); k++) {
        result[k] = sim.measurement_record[k];
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
    TableauSimulator sim(1, rng);
    size_t reported = 0;
    while (true) {
        Circuit unprocessed;
        unprocessed.append_from_file(in, newline_after_measurements);
        if (unprocessed.operations.empty()) {
            break;
        }
        sim.ensure_large_enough_for_qubits(unprocessed.count_qubits());

        unprocessed.for_each_operation([&](const Operation &op) {
            (sim.*op.gate->tableau_simulator_function)(op.target_data);
            while (reported < sim.measurement_record.size()) {
                putc('0' + sim.measurement_record[reported++], out);
            }
            if (newline_after_measurements && (op.gate->flags & GATE_PRODUCES_RESULTS)) {
                putc('\n', out);
                fflush(out);
            }
        });

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
        t &= TARGET_VALUE_MASK;
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

Circuit aliased_noiseless_subset(const Circuit &circuit) {
    // HACK: result has pointers into `circuit`!
    Circuit result;
    for (const auto &op : circuit.operations) {
        if (!(op.gate->flags & GATE_IS_NOISE)) {
            result.operations.push_back(op);
        }
    }
    for (const auto &block : circuit.blocks) {
        result.blocks.push_back(aliased_noiseless_subset(block));
    }
    return result;
}

simd_bits TableauSimulator::reference_sample_circuit(const Circuit &circuit) {
    std::mt19937_64 irrelevant_rng(0);
    return TableauSimulator::sample_circuit(aliased_noiseless_subset(circuit), irrelevant_rng, +1);
}
