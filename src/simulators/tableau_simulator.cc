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
#include <set>

using namespace stim_internal;

TableauSimulator::TableauSimulator(size_t num_qubits, std::mt19937_64 &rng, int8_t sign_bias, MeasureRecord record)
    : inv_state(Tableau::identity(num_qubits)),
      rng(rng),
      sign_bias(sign_bias),
      measurement_record(record),
      last_correlated_error_occurred(false) {
}

bool TableauSimulator::is_deterministic_x(size_t target) const {
    return !inv_state.xs[target].xs.not_zero();
}

bool TableauSimulator::is_deterministic_y(size_t target) const {
    return inv_state.xs[target].xs == inv_state.zs[target].xs;
}

bool TableauSimulator::is_deterministic_z(size_t target) const {
    return !inv_state.zs[target].xs.not_zero();
}

void TableauSimulator::measure_x(const OperationData &target_data) {
    // Ensure measurement observables are collapsed.
    collapse_x(target_data.targets);

    // Record measurement results.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_VALUE_MASK;
        bool flipped = qf & TARGET_INVERTED_BIT;
        bool b = inv_state.xs.signs[q] ^ flipped;
        measurement_record.record_result(b);
    }
}

void TableauSimulator::measure_y(const OperationData &target_data) {
    // Ensure measurement observables are collapsed.
    collapse_y(target_data.targets);

    // Record measurement results.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_VALUE_MASK;
        bool flipped = qf & TARGET_INVERTED_BIT;
        bool b = inv_state.eval_y_obs(q).sign ^ flipped;
        measurement_record.record_result(b);
    }
}

void TableauSimulator::measure_z(const OperationData &target_data) {
    // Ensure measurement observables are collapsed.
    collapse_z(target_data.targets);

    // Record measurement results.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_VALUE_MASK;
        bool flipped = qf & TARGET_INVERTED_BIT;
        bool b = inv_state.zs.signs[q] ^ flipped;
        measurement_record.record_result(b);
    }
}

void TableauSimulator::measure_reset_x(const OperationData &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.

    // Ensure measurement observables are collapsed.
    collapse_x(target_data.targets);

    // Record measurement results while triggering resets.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_VALUE_MASK;
        bool flipped = qf & TARGET_INVERTED_BIT;
        bool b = inv_state.xs.signs[q] ^ flipped;
        measurement_record.record_result(b);
        inv_state.xs.signs[q] = false;
        inv_state.zs.signs[q] = false;
    }
}

void TableauSimulator::measure_reset_y(const OperationData &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.

    // Ensure measurement observables are collapsed.
    collapse_y(target_data.targets);

    // Record measurement results while triggering resets.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_VALUE_MASK;
        bool flipped = qf & TARGET_INVERTED_BIT;
        bool cur_sign = inv_state.eval_y_obs(q).sign;
        bool b = cur_sign ^ flipped;
        measurement_record.record_result(b);
        inv_state.zs.signs[q] ^= cur_sign;
    }
}

void TableauSimulator::measure_reset_z(const OperationData &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.

    // Ensure measurement observables are collapsed.
    collapse_z(target_data.targets);

    // Record measurement results while triggering resets.
    for (auto qf : target_data.targets) {
        auto q = qf & TARGET_VALUE_MASK;
        bool flipped = qf & TARGET_INVERTED_BIT;
        bool b = inv_state.zs.signs[q] ^ flipped;
        measurement_record.record_result(b);
        inv_state.xs.signs[q] = false;
        inv_state.zs.signs[q] = false;
    }
}

void TableauSimulator::reset_x(const OperationData &target_data) {
    // Collapse the qubits to be reset.
    collapse_x(target_data.targets);

    // Force the collapsed qubits into the ground state.
    for (auto q : target_data.targets) {
        inv_state.xs.signs[q] = false;
        inv_state.zs.signs[q] = false;
    }
}

void TableauSimulator::reset_y(const OperationData &target_data) {
    // Collapse the qubits to be reset.
    collapse_y(target_data.targets);

    // Force the collapsed qubits into the ground state.
    for (auto q : target_data.targets) {
        inv_state.xs.signs[q] = false;
        inv_state.zs.signs[q] = false;
        inv_state.zs.signs[q] ^= inv_state.eval_y_obs(q).sign;
    }
}

void TableauSimulator::reset_z(const OperationData &target_data) {
    // Collapse the qubits to be reset.
    collapse_z(target_data.targets);

    // Force the collapsed qubits into the ground state.
    for (auto q : target_data.targets) {
        inv_state.xs.signs[q] = false;
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

void TableauSimulator::C_XYZ(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_ZYX(q);
    }
}

void TableauSimulator::C_ZYX(const OperationData &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_XYZ(q);
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

PauliString TableauSimulator::peek_bloch(uint32_t target) const {
    PauliStringRef x = inv_state.xs[target];
    PauliStringRef z = inv_state.zs[target];

    PauliString result(1);
    if (!x.xs.not_zero()) {
        result.sign = x.sign;
        result.xs[0] = true;
    } else if (!z.xs.not_zero()) {
        result.sign = z.sign;
        result.zs[0] = true;
    } else if (x.xs == z.xs) {
        PauliString y = inv_state.eval_y_obs(target);
        result.sign = y.sign;
        result.xs[0] = true;
        result.zs[0] = true;
    }

    return result;
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
    assert(encoded_target & TARGET_RECORD_BIT);
    return measurement_record.lookback(encoded_target ^ TARGET_RECORD_BIT);
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
        inv_state.zs.signs[q] ^= true;
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
        inv_state.xs.signs[q] ^= true;
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

    const std::vector<bool> &v = sim.measurement_record.storage;
    simd_bits result(v.size());
    for (size_t k = 0; k < v.size(); k++) {
        result[k] = v[k];
    }
    return result;
}

void TableauSimulator::ensure_large_enough_for_qubits(size_t num_qubits) {
    if (num_qubits <= inv_state.num_qubits) {
        return;
    }
    inv_state.expand(num_qubits);
}

void TableauSimulator::sample_stream(FILE *in, FILE *out, SampleFormat format, bool interactive, std::mt19937_64 &rng) {
    TableauSimulator sim(1, rng);
    auto writer = MeasureRecordWriter::make(out, format);
    Circuit unprocessed;
    while (true) {
        unprocessed.clear();
        unprocessed.append_from_file(in, true);
        if (unprocessed.operations.empty()) {
            break;
        }
        sim.ensure_large_enough_for_qubits(unprocessed.count_qubits());

        unprocessed.for_each_operation([&](const Operation &op) {
            (sim.*op.gate->tableau_simulator_function)(op.target_data);
            sim.measurement_record.write_unwritten_results_to(*writer);
            if (interactive && (op.gate->flags & GATE_PRODUCES_RESULTS)) {
                putc('\n', out);
                fflush(out);
            }
        });
    }
    writer->write_end();
}

VectorSimulator TableauSimulator::to_vector_sim() const {
    auto inv = inv_state.inverse();
    std::vector<PauliStringRef> stabilizers;
    for (size_t k = 0; k < inv.num_qubits; k++) {
        stabilizers.push_back(inv.zs[k]);
    }
    return VectorSimulator::from_stabilizers(stabilizers, rng);
}

void TableauSimulator::collapse_x(ConstPointerRange<uint32_t> targets) {
    // Find targets that need to be collapsed.
    std::set<uint32_t> unique_collapse_targets;
    for (uint32_t t : targets) {
        t &= TARGET_VALUE_MASK;
        if (!is_deterministic_x(t)) {
            unique_collapse_targets.insert(t);
        }
    }

    // Only pay the cost of transposing if collapsing is needed.
    if (!unique_collapse_targets.empty()) {
        std::vector<uint32_t> collapse_targets(unique_collapse_targets.begin(), unique_collapse_targets.end());
        H_XZ({0, collapse_targets});
        {
            TableauTransposedRaii temp_transposed(inv_state);
            for (auto q : collapse_targets) {
                collapse_qubit_z(q, temp_transposed);
            }
        }
        H_XZ({0, collapse_targets});
    }
}

void TableauSimulator::collapse_y(ConstPointerRange<uint32_t> targets) {
    // Find targets that need to be collapsed.
    std::set<uint32_t> unique_collapse_targets;
    for (uint32_t t : targets) {
        t &= TARGET_VALUE_MASK;
        if (!is_deterministic_y(t)) {
            unique_collapse_targets.insert(t);
        }
    }

    // Only pay the cost of transposing if collapsing is needed.
    if (!unique_collapse_targets.empty()) {
        std::vector<uint32_t> collapse_targets(unique_collapse_targets.begin(), unique_collapse_targets.end());
        H_YZ({0, collapse_targets});
        {
            TableauTransposedRaii temp_transposed(inv_state);
            for (auto q : collapse_targets) {
                collapse_qubit_z(q, temp_transposed);
            }
        }
        H_YZ({0, collapse_targets});
    }
}

void TableauSimulator::collapse_z(ConstPointerRange<uint32_t> targets) {
    // Find targets that need to be collapsed.
    std::vector<size_t> collapse_targets;
    collapse_targets.reserve(targets.size());
    for (uint32_t t : targets) {
        t &= TARGET_VALUE_MASK;
        if (!is_deterministic_z(t)) {
            collapse_targets.push_back(t);
        }
    }

    // Only pay the cost of transposing if collapsing is needed.
    if (!collapse_targets.empty()) {
        TableauTransposedRaii temp_transposed(inv_state);
        for (auto target : collapse_targets) {
            collapse_qubit_z(target, temp_transposed);
        }
    }
}

size_t TableauSimulator::collapse_qubit_z(size_t target, TableauTransposedRaii &transposed_raii) {
    auto n = inv_state.num_qubits;

    // Search for any stabilizer generator that anti-commutes with the measurement observable.
    size_t pivot = 0;
    while (pivot < n && !transposed_raii.tableau.zs.xt[pivot][target]) {
        pivot++;
    }
    if (pivot == n) {
        // No anti-commuting stabilizer generator. Measurement is deterministic.
        return SIZE_MAX;
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

    return pivot;
}

void TableauSimulator::collapse_isolate_qubit_z(size_t target, TableauTransposedRaii &transposed_raii) {
    // Force T(Z_target) to be a product of Z operations.
    collapse_qubit_z(target, transposed_raii);

    // Ensure T(Z_target) is a product of Z operations containing Z_target.
    auto n = inv_state.num_qubits;
    for (size_t q = 0; true; q++) {
        assert(q < n);
        if (transposed_raii.tableau.zs.zt[q][target]) {
            if (q != target) {
                transposed_raii.append_SWAP(q, target);
            }
            break;
        }
    }

    // Ensure T(Z_target) = +-Z_target.
    for (size_t q = 0; q < n; q++) {
        if (q != target && transposed_raii.tableau.zs.zt[q][target]) {
            // Cancel Z term on non-target q.
            transposed_raii.append_ZCX(q, target);
        }
    }

    // Note T(X_target) now contains X_target or Y_target because it has to anti-commute with T(Z_target) = Z_target.
    // Ensure T(X_target) contains X_target instead of Y_target.
    if (transposed_raii.tableau.xs.zt[target][target]) {
        transposed_raii.append_S(target);
    }

    // Ensure T(X_target) = +-X_target.
    for (size_t q = 0; q < n; q++) {
        if (q != target) {
            int p = transposed_raii.tableau.xs.xt[q][target] + 2*transposed_raii.tableau.xs.zt[q][target];
            if (p == 1) {
                transposed_raii.append_ZCX(target, q);
            } else if (p == 2) {
                transposed_raii.append_ZCZ(target, q);
            } else if (p == 3) {
                transposed_raii.append_ZCY(target, q);
            }
        }
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

void TableauSimulator::paulis(const PauliString &paulis) {
    auto nw = paulis.xs.num_simd_words;
    inv_state.zs.signs.word_range_ref(0, nw) ^= paulis.xs;
    inv_state.xs.signs.word_range_ref(0, nw) ^= paulis.zs;
}

void TableauSimulator::set_num_qubits(size_t new_num_qubits) {
    if (new_num_qubits >= inv_state.num_qubits) {
        ensure_large_enough_for_qubits(new_num_qubits);
        return;
    }

    // Collapse qubits past the new size and ensure the internal state totally decouples them.
    {
        TableauTransposedRaii temp_transposed(inv_state);
        for (size_t q = new_num_qubits; q < inv_state.num_qubits; q++) {
            collapse_isolate_qubit_z(q, temp_transposed);
        }
    }

    Tableau old_state = std::move(inv_state);
    inv_state = Tableau(new_num_qubits);
    inv_state.xs.signs.truncated_overwrite_from(old_state.xs.signs, new_num_qubits);
    inv_state.zs.signs.truncated_overwrite_from(old_state.zs.signs, new_num_qubits);
    for (size_t q = 0; q < new_num_qubits; q++) {
        inv_state.xs[q].xs.truncated_overwrite_from(old_state.xs[q].xs, new_num_qubits);
        inv_state.xs[q].zs.truncated_overwrite_from(old_state.xs[q].zs, new_num_qubits);
        inv_state.zs[q].xs.truncated_overwrite_from(old_state.zs[q].xs, new_num_qubits);
        inv_state.zs[q].zs.truncated_overwrite_from(old_state.zs[q].zs, new_num_qubits);
    }
}

std::pair<bool, PauliString> TableauSimulator::measure_kickback_z(uint32_t target) {
    bool flipped = target & TARGET_INVERTED_BIT;
    uint32_t q = target & TARGET_VALUE_MASK;
    PauliString kickback(0);
    bool has_kickback = !is_deterministic_z(q); // Note: do this before transposing the state!

    {
        TableauTransposedRaii temp_transposed(inv_state);
        if (has_kickback) {
            size_t pivot = collapse_qubit_z(q, temp_transposed);
            kickback = temp_transposed.unsigned_x_input(pivot);
        }
        bool result = inv_state.zs.signs[q] ^ flipped;
        measurement_record.storage.push_back(result);

        // Prevent later measure_kickback calls from unnecessarily targeting this qubit with a Z gate.
        collapse_isolate_qubit_z(target, temp_transposed);

        return {result, kickback};
    }
}

std::pair<bool, PauliString> TableauSimulator::measure_kickback_y(uint32_t target) {
    H_YZ({0.0, {&target, &target + 1}});
    auto result = measure_kickback_z(target);
    H_YZ({0.0, {&target, &target + 1}});
    if (result.second.num_qubits) {
        // Also conjugate the kickback by H_YZ.
        result.second.xs[target] ^= result.second.zs[target];
    }
    return result;
}

std::pair<bool, PauliString> TableauSimulator::measure_kickback_x(uint32_t target) {
    H_XZ({0.0, {&target, &target + 1}});
    auto result = measure_kickback_z(target);
    H_XZ({0.0, {&target, &target + 1}});
    if (result.second.num_qubits) {
        // Also conjugate the kickback by H_XZ.
        result.second.xs[target].swap_with(result.second.zs[target]);
    }
    return result;
}

std::vector<PauliString> TableauSimulator::canonical_stabilizers() const {
    Tableau t = inv_state.inverse();
    size_t n = t.num_qubits;
    std::vector<PauliString> stabilizers;
    for (size_t k = 0; k < n; k++) {
        stabilizers.push_back(t.zs[k]);
    }

    size_t min_pivot = 0;
    for (size_t q = 0; q < n; q++) {
        for (size_t b = 0; b < 2; b++) {
            size_t pivot = min_pivot;
            while (pivot < n && !(b ? stabilizers[pivot].zs : stabilizers[pivot].xs)[q]) {
                pivot++;
            }
            if (pivot == n) {
                continue;
            }
            for (size_t s = 0; s < n; s++) {
                if (s != pivot && (b ? stabilizers[s].zs : stabilizers[s].xs)[q]) {
                    stabilizers[s].ref() *= stabilizers[pivot];
                }
            }
            if (min_pivot != pivot) {
                std::swap(stabilizers[min_pivot], stabilizers[pivot]);
            }
            min_pivot += 1;
        }
    }
    return stabilizers;
}
