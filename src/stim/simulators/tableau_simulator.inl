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

#include <set>

#include "stim/circuit/gate_decomposition.h"
#include "stim/gates/gates.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/util_bot/probability_util.h"

namespace stim {

template <size_t W>
TableauSimulator<W>::TableauSimulator(std::mt19937_64 &&rng, size_t num_qubits, int8_t sign_bias, MeasureRecord record)
    : inv_state(Tableau<W>::identity(num_qubits)),
      rng(std::move(rng)),
      sign_bias(sign_bias),
      measurement_record(std::move(record)),
      last_correlated_error_occurred(false) {
}

template <size_t W>
TableauSimulator<W>::TableauSimulator(const TableauSimulator<W> &other, std::mt19937_64 &&rng)
    : inv_state(other.inv_state),
      rng(std::move(rng)),
      sign_bias(other.sign_bias),
      measurement_record(other.measurement_record),
      last_correlated_error_occurred(other.last_correlated_error_occurred) {
}

template <size_t W>
bool TableauSimulator<W>::is_deterministic_x(size_t target) const {
    return !inv_state.xs[target].xs.not_zero();
}

template <size_t W>
bool TableauSimulator<W>::is_deterministic_y(size_t target) const {
    return inv_state.xs[target].xs == inv_state.zs[target].xs;
}

template <size_t W>
bool TableauSimulator<W>::is_deterministic_z(size_t target) const {
    return !inv_state.zs[target].xs.not_zero();
}

template <size_t W>
void TableauSimulator<W>::do_MPP(const CircuitInstruction &target_data) {
    decompose_mpp_operation(target_data, inv_state.num_qubits, [&](const CircuitInstruction &inst) {
        do_gate(inst);
    });
}

template <size_t W>
void TableauSimulator<W>::do_SPP(const CircuitInstruction &target_data) {
    decompose_spp_or_spp_dag_operation(target_data, inv_state.num_qubits, false, [&](const CircuitInstruction &inst) {
        do_gate(inst);
    });
}

template <size_t W>
void TableauSimulator<W>::do_SPP_DAG(const CircuitInstruction &target_data) {
    decompose_spp_or_spp_dag_operation(target_data, inv_state.num_qubits, false, [&](const CircuitInstruction &inst) {
        do_gate(inst);
    });
}

template <size_t W>
void TableauSimulator<W>::postselect_helper(
    SpanRef<const GateTarget> targets,
    bool desired_result,
    GateType basis_change_gate,
    const char *false_name,
    const char *true_name) {
    std::set<GateTarget> unique_targets;
    unique_targets.insert(targets.begin(), targets.end());
    std::vector<GateTarget> unique_targets_vec;
    unique_targets_vec.insert(unique_targets_vec.end(), unique_targets.begin(), unique_targets.end());

    size_t finished = 0;
    do_gate({basis_change_gate, {}, unique_targets_vec, ""});
    {
        uint8_t old_bias = sign_bias;
        sign_bias = desired_result ? -1 : +1;
        TableauTransposedRaii<W> temp_transposed(inv_state);
        while (finished < targets.size()) {
            size_t q = (size_t)targets[finished].qubit_value();
            collapse_qubit_z(q, temp_transposed);
            if (inv_state.zs.signs[q] != desired_result) {
                break;
            }
            finished++;
        }
        sign_bias = old_bias;
    }
    do_gate({basis_change_gate, {}, unique_targets_vec, ""});

    if (finished < targets.size()) {
        std::stringstream msg;
        msg << "The requested postselection was impossible.\n";
        msg << "Desired state: |" << (desired_result ? true_name : false_name) << ">\n";
        msg << "Qubit " << targets[finished] << " is in the perpendicular state |"
            << (desired_result ? false_name : true_name) << ">\n";
        if (finished > 0) {
            msg << finished << " of the requested postselections were finished (";
            for (size_t k = 0; k < finished; k++) {
                msg << "qubit " << targets[k] << ", ";
            }
            msg << "[failed here])\n";
        }
        throw std::invalid_argument(msg.str());
    }
}

template <size_t W>
uint32_t TableauSimulator<W>::try_isolate_observable_to_qubit_z(PauliStringRef<W> observable, bool undo) {
    uint32_t pivot = UINT32_MAX;
    observable.for_each_active_pauli([&](size_t q) {
        uint8_t p = observable.xs[q] + observable.zs[q] * 2;
        if (pivot == UINT32_MAX) {
            pivot = q;
            if (!undo) {
                if (p == 1) {
                    inv_state.prepend_H_XZ(pivot);
                } else if (p == 3) {
                    inv_state.prepend_H_YZ(pivot);
                }
                if (observable.sign) {
                    inv_state.prepend_X(pivot);
                }
            }
        } else {
            if (p == 1) {
                inv_state.prepend_XCX(pivot, q);
            } else if (p == 2) {
                inv_state.prepend_XCZ(pivot, q);
            } else if (p == 3) {
                inv_state.prepend_XCY(pivot, q);
            }
        }
    });
    if (undo && pivot != UINT32_MAX) {
        uint8_t p = observable.xs[pivot] + observable.zs[pivot] * 2;
        if (observable.sign) {
            inv_state.prepend_X(pivot);
        }
        if (p == 1) {
            inv_state.prepend_H_XZ(pivot);
        } else if (p == 3) {
            inv_state.prepend_H_YZ(pivot);
        }
    }
    return pivot;
}

template <size_t W>
void TableauSimulator<W>::postselect_observable(PauliStringRef<W> observable, bool desired_result) {
    ensure_large_enough_for_qubits(observable.num_qubits);

    uint32_t pivot = try_isolate_observable_to_qubit_z(observable, false);
    int8_t expected;
    if (pivot != UINT32_MAX) {
        expected = peek_z(pivot);
    } else {
        expected = observable.sign ? -1 : +1;
    }
    if (desired_result) {
        expected *= -1;
    }

    if (expected != -1 && pivot != UINT32_MAX) {
        GateTarget t{pivot};
        postselect_z(&t, desired_result);
    }
    try_isolate_observable_to_qubit_z(observable, true);

    if (expected == -1) {
        std::stringstream msg;
        msg << "It's impossible to postselect into the ";
        msg << (desired_result ? "-1" : "+1");
        msg << " eigenstate of ";
        msg << observable;
        msg << " because the system is deterministically in the ";
        msg << (desired_result ? "+1" : "-1");
        msg << " eigenstate.";
        throw std::invalid_argument(msg.str());
    }
}

template <size_t W>
void TableauSimulator<W>::postselect_x(SpanRef<const GateTarget> targets, bool desired_result) {
    postselect_helper(targets, desired_result, GateType::H, "+", "-");
}

template <size_t W>
void TableauSimulator<W>::postselect_y(SpanRef<const GateTarget> targets, bool desired_result) {
    postselect_helper(targets, desired_result, GateType::H_YZ, "i", "-i");
}

template <size_t W>
void TableauSimulator<W>::postselect_z(SpanRef<const GateTarget> targets, bool desired_result) {
    postselect_helper(targets, desired_result, GateType::I, "0", "1");
}

template <size_t W>
void TableauSimulator<W>::do_MX(const CircuitInstruction &target_data) {
    // Ensure measurement observables are collapsed.
    collapse_x(target_data.targets);

    // Record measurement results.
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();
        bool flipped = t.is_inverted_result_target();
        bool b = inv_state.xs.signs[q] ^ flipped;
        measurement_record.record_result(b);
    }
    noisify_new_measurements(target_data);
}

template <size_t W>
void TableauSimulator<W>::do_MXX_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets, ""});

    // Ensure measurement observables are collapsed.
    collapse_x(inst.targets, 2);

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        GateTarget t1 = inst.targets[k];
        GateTarget t2 = inst.targets[k + 1];
        auto q = t1.qubit_value();
        bool flipped = t1.is_inverted_result_target() ^ t2.is_inverted_result_target();
        bool b = inv_state.xs.signs[q] ^ flipped;
        measurement_record.record_result(b);
    }
    noisify_new_measurements(inst.args, inst.targets.size() / 2);

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets, ""});
}

template <size_t W>
void TableauSimulator<W>::do_MYY_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets, ""});

    // Ensure measurement observables are collapsed.
    collapse_y(inst.targets, 2);

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        GateTarget t1 = inst.targets[k];
        GateTarget t2 = inst.targets[k + 1];
        auto q = t1.qubit_value();
        bool flipped = t1.is_inverted_result_target() ^ t2.is_inverted_result_target();
        bool b = inv_state.eval_y_obs(q).sign ^ flipped;
        measurement_record.record_result(b);
    }
    noisify_new_measurements(inst.args, inst.targets.size() / 2);

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets, ""});
}

template <size_t W>
void TableauSimulator<W>::do_MZZ_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    do_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets, ""});

    // Ensure measurement observables are collapsed.
    collapse_z(inst.targets, 2);

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        GateTarget t1 = inst.targets[k];
        GateTarget t2 = inst.targets[k + 1];
        auto q = t1.qubit_value();
        bool flipped = t1.is_inverted_result_target() ^ t2.is_inverted_result_target();
        bool b = inv_state.zs.signs[q] ^ flipped;
        measurement_record.record_result(b);
    }
    noisify_new_measurements(inst.args, inst.targets.size() / 2);

    // Untransform from single qubit measurements back to 2 qubit measurements.
    do_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets, ""});
}

template <size_t W>
void TableauSimulator<W>::do_MXX(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_disjoint_segments(inst, inv_state.num_qubits, [&](CircuitInstruction segment) {
        do_MXX_disjoint_controls_segment(segment);
    });
}

template <size_t W>
void TableauSimulator<W>::do_MYY(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_disjoint_segments(inst, inv_state.num_qubits, [&](CircuitInstruction segment) {
        do_MYY_disjoint_controls_segment(segment);
    });
}

template <size_t W>
void TableauSimulator<W>::do_MZZ(const CircuitInstruction &inst) {
    decompose_pair_instruction_into_disjoint_segments(inst, inv_state.num_qubits, [&](CircuitInstruction segment) {
        do_MZZ_disjoint_controls_segment(segment);
    });
}

template <size_t W>
void TableauSimulator<W>::do_MPAD(const CircuitInstruction &inst) {
    for (const auto &t : inst.targets) {
        measurement_record.record_result(t.qubit_value() != 0);
    }
    noisify_new_measurements(inst);
}

template <size_t W>
void TableauSimulator<W>::do_MY(const CircuitInstruction &target_data) {
    // Ensure measurement observables are collapsed.
    collapse_y(target_data.targets);

    // Record measurement results.
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();
        bool flipped = t.is_inverted_result_target();
        bool b = inv_state.eval_y_obs(q).sign ^ flipped;
        measurement_record.record_result(b);
    }
    noisify_new_measurements(target_data);
}

template <size_t W>
void TableauSimulator<W>::do_MZ(const CircuitInstruction &target_data) {
    // Ensure measurement observables are collapsed.
    collapse_z(target_data.targets);

    // Record measurement results.
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();
        bool flipped = t.is_inverted_result_target();
        bool b = inv_state.zs.signs[q] ^ flipped;
        measurement_record.record_result(b);
    }
    noisify_new_measurements(target_data);
}

template <size_t W>
bool TableauSimulator<W>::measure_pauli_string(const PauliStringRef<W> pauli_string, double flip_probability) {
    if (!(0 <= flip_probability && flip_probability <= 1)) {
        throw std::invalid_argument("Need 0 <= flip_probability <= 1");
    }
    ensure_large_enough_for_qubits(pauli_string.num_qubits);

    std::vector<GateTarget> targets;
    targets.reserve(pauli_string.num_qubits * 2);
    for (size_t k = 0; k < pauli_string.num_qubits; k++) {
        bool x = pauli_string.xs[k];
        bool z = pauli_string.zs[k];
        if (x || z) {
            GateTarget target{(uint32_t)k};
            if (x) {
                target.data |= TARGET_PAULI_X_BIT;
            }
            if (z) {
                target.data |= TARGET_PAULI_Z_BIT;
            }
            targets.push_back(target);
            targets.push_back(GateTarget::combiner());
        }
    }
    double p = flip_probability;
    if (pauli_string.sign) {
        p = 1 - p;
    }
    if (targets.empty()) {
        measurement_record.record_result(std::bernoulli_distribution(p)(rng));
    } else {
        targets.pop_back();
        do_MPP(CircuitInstruction{GateType::MPP, &p, targets, ""});
    }
    return measurement_record.lookback(1);
}

template <size_t W>
void TableauSimulator<W>::do_MRX(const CircuitInstruction &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.

    // Ensure measurement observables are collapsed.
    collapse_x(target_data.targets);

    // Record measurement results while triggering resets.
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();
        bool flipped = t.is_inverted_result_target();
        bool b = inv_state.xs.signs[q] ^ flipped;
        measurement_record.record_result(b);
        inv_state.xs.signs[q] = false;
        inv_state.zs.signs[q] = false;
    }
    noisify_new_measurements(target_data);
}

template <size_t W>
void TableauSimulator<W>::do_MRY(const CircuitInstruction &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.

    // Ensure measurement observables are collapsed.
    collapse_y(target_data.targets);

    // Record measurement results while triggering resets.
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();
        bool flipped = t.is_inverted_result_target();
        bool cur_sign = inv_state.eval_y_obs(q).sign;
        bool b = cur_sign ^ flipped;
        measurement_record.record_result(b);
        inv_state.zs.signs[q] ^= cur_sign;
    }
    noisify_new_measurements(target_data);
}

template <size_t W>
void TableauSimulator<W>::do_MRZ(const CircuitInstruction &target_data) {
    // Note: Caution when implementing this. Can't group the resets. because the same qubit target may appear twice.

    // Ensure measurement observables are collapsed.
    collapse_z(target_data.targets);

    // Record measurement results while triggering resets.
    for (auto t : target_data.targets) {
        auto q = t.qubit_value();
        bool flipped = t.is_inverted_result_target();
        bool b = inv_state.zs.signs[q] ^ flipped;
        measurement_record.record_result(b);
        inv_state.xs.signs[q] = false;
        inv_state.zs.signs[q] = false;
    }
    noisify_new_measurements(target_data);
}

template <size_t W>
void TableauSimulator<W>::noisify_new_measurements(SpanRef<const double> args, size_t num_targets) {
    if (args.empty()) {
        return;
    }
    size_t last = measurement_record.storage.size() - 1;
    RareErrorIterator::for_samples(args[0], num_targets, rng, [&](size_t k) {
        measurement_record.storage[last - k] = !measurement_record.storage[last - k];
    });
}

template <size_t W>
void TableauSimulator<W>::noisify_new_measurements(const CircuitInstruction &inst) {
    noisify_new_measurements(inst.args, inst.targets.size());
}

template <size_t W>
void TableauSimulator<W>::do_RX(const CircuitInstruction &target_data) {
    // Collapse the qubits to be reset.
    collapse_x(target_data.targets);

    // Force the collapsed qubits into the ground state.
    for (auto q : target_data.targets) {
        inv_state.xs.signs[q.data] = false;
        inv_state.zs.signs[q.data] = false;
    }
}

template <size_t W>
void TableauSimulator<W>::do_RY(const CircuitInstruction &target_data) {
    // Collapse the qubits to be reset.
    collapse_y(target_data.targets);

    // Force the collapsed qubits into the ground state.
    for (auto q : target_data.targets) {
        inv_state.xs.signs[q.data] = false;
        inv_state.zs.signs[q.data] = false;
        inv_state.zs.signs[q.data] ^= inv_state.eval_y_obs(q.data).sign;
    }
}

template <size_t W>
void TableauSimulator<W>::do_RZ(const CircuitInstruction &target_data) {
    // Collapse the qubits to be reset.
    collapse_z(target_data.targets);

    // Force the collapsed qubits into the ground state.
    for (auto q : target_data.targets) {
        inv_state.xs.signs[q.data] = false;
        inv_state.zs.signs[q.data] = false;
    }
}

template <size_t W>
void TableauSimulator<W>::do_I(const CircuitInstruction &target_data) {
}

template <size_t W>
void TableauSimulator<W>::do_H_XZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_XZ(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_H_XY(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_XY(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_H_YZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_YZ(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_H_NXY(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_NXY(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_H_NXZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_NXZ(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_H_NYZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_H_NYZ(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_C_XYZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_ZYX(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_C_NXYZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_ZYNX(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_C_XNYZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_ZNYX(q.data);
    }
}
template <size_t W>
void TableauSimulator<W>::do_C_XYNZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_NZYX(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_C_ZYX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_XYZ(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_C_NZYX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_XYNZ(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_C_ZNYX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_XNYZ(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_C_ZYNX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_C_NXYZ(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_Z(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Z_DAG(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_Z_DAG(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Z(q.data);
    }
}

template <size_t W>
PauliString<W> TableauSimulator<W>::peek_bloch(uint32_t target) const {
    PauliStringRef<W> x = inv_state.xs[target];
    PauliStringRef<W> z = inv_state.zs[target];

    PauliString<W> result(1);
    if (!x.xs.not_zero()) {
        result.sign = x.sign;
        result.xs[0] = true;
    } else if (!z.xs.not_zero()) {
        result.sign = z.sign;
        result.zs[0] = true;
    } else if (x.xs == z.xs) {
        PauliString<W> y = inv_state.eval_y_obs(target);
        result.sign = y.sign;
        result.xs[0] = true;
        result.zs[0] = true;
    }

    return result;
}

template <size_t W>
int8_t TableauSimulator<W>::peek_x(uint32_t target) const {
    PauliStringRef<W> x = inv_state.xs[target];
    return x.xs.not_zero() ? 0 : x.sign ? -1 : +1;
}

template <size_t W>
int8_t TableauSimulator<W>::peek_y(uint32_t target) const {
    PauliString<W> y = inv_state.eval_y_obs(target);
    return y.xs.not_zero() ? 0 : y.sign ? -1 : +1;
}

template <size_t W>
int8_t TableauSimulator<W>::peek_z(uint32_t target) const {
    PauliStringRef<W> z = inv_state.zs[target];
    return z.xs.not_zero() ? 0 : z.sign ? -1 : +1;
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_X(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_X_DAG(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_X_DAG(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_X(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_Y(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Y_DAG(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_Y_DAG(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_Y(q.data);
    }
}

template <size_t W>
bool TableauSimulator<W>::read_measurement_record(uint32_t encoded_target) const {
    if (encoded_target & TARGET_SWEEP_BIT) {
        // Shot-to-shot variation currently not supported by the tableau simulator. Use frame simulator.
        return false;
    }
    assert(encoded_target & TARGET_RECORD_BIT);
    return measurement_record.lookback(encoded_target ^ TARGET_RECORD_BIT);
}

template <size_t W>
void TableauSimulator<W>::single_cx(uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        inv_state.prepend_ZCX(c, t);
    } else if (t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) {
        throw std::invalid_argument("Measurement record editing is not supported.");
    } else {
        if (read_measurement_record(c)) {
            inv_state.prepend_X(t);
        }
    }
}

template <size_t W>
void TableauSimulator<W>::single_cy(uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        inv_state.prepend_ZCY(c, t);
    } else if (t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) {
        throw std::invalid_argument("Measurement record editing is not supported.");
    } else {
        if (read_measurement_record(c)) {
            inv_state.prepend_Y(t);
        }
    }
}

template <size_t W>
void TableauSimulator<W>::do_ZCX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k].data, targets[k + 1].data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_ZCY(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k].data, targets[k + 1].data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_ZCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        q1 &= ~TARGET_INVERTED_BIT;
        q2 &= ~TARGET_INVERTED_BIT;
        if (!((q1 | q2) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
            inv_state.prepend_ZCZ(q1, q2);
            continue;
        } else if (!(q2 & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
            if (read_measurement_record(q1)) {
                inv_state.prepend_Z(q2);
            }
        } else if (!(q1 & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
            if (read_measurement_record(q2)) {
                inv_state.prepend_Z(q1);
            }
        } else {
            // Both targets are bits. No effect.
        }
    }
}

template <size_t W>
void TableauSimulator<W>::do_SWAP(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto c = targets[k].data;
        auto t = targets[k + 1].data;
        inv_state.prepend_SWAP(c, t);
    }
}

template <size_t W>
void TableauSimulator<W>::do_CXSWAP(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        inv_state.prepend_ZCX(q2, q1);
        inv_state.prepend_ZCX(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_CZSWAP(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        inv_state.prepend_ZCZ(q1, q2);
        inv_state.prepend_SWAP(q2, q1);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SWAPCX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        inv_state.prepend_ZCX(q1, q2);
        inv_state.prepend_ZCX(q2, q1);
    }
}

template <size_t W>
void TableauSimulator<W>::do_ISWAP(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_ISWAP_DAG(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_ISWAP_DAG(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_ISWAP(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_XCX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        inv_state.prepend_XCX(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_ZZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_ZZ_DAG(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_ZZ_DAG(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_ZZ(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_YY(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_YY_DAG(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_YY_DAG(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_YY(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_XX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_XX_DAG(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_SQRT_XX_DAG(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        // Note: inverted because we're tracking the inverse tableau.
        inv_state.prepend_SQRT_XX(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_XCY(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        inv_state.prepend_XCY(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_XCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cx(targets[k + 1].data, targets[k].data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_YCX(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        inv_state.prepend_YCX(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_YCY(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        auto q1 = targets[k].data;
        auto q2 = targets[k + 1].data;
        inv_state.prepend_YCY(q1, q2);
    }
}

template <size_t W>
void TableauSimulator<W>::do_YCZ(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    for (size_t k = 0; k < targets.size(); k += 2) {
        single_cy(targets[k + 1].data, targets[k].data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_DEPOLARIZE1(const CircuitInstruction &target_data) {
    RareErrorIterator::for_samples(target_data.args[0], target_data.targets, rng, [&](GateTarget q) {
        auto p = 1 + (rng() % 3);
        inv_state.xs.signs[q.data] ^= p & 1;
        inv_state.zs.signs[q.data] ^= p & 2;
    });
}

template <size_t W>
void TableauSimulator<W>::do_DEPOLARIZE2(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    assert(!(targets.size() & 1));
    auto n = targets.size() >> 1;
    RareErrorIterator::for_samples(target_data.args[0], n, rng, [&](size_t s) {
        auto p = 1 + (rng() % 15);
        auto q1 = targets[s << 1].data;
        auto q2 = targets[1 | (s << 1)].data;
        inv_state.xs.signs[q1] ^= p & 1;
        inv_state.zs.signs[q1] ^= p & 2;
        inv_state.xs.signs[q2] ^= p & 4;
        inv_state.zs.signs[q2] ^= p & 8;
    });
}

template <size_t W>
void TableauSimulator<W>::do_HERALDED_ERASE(const CircuitInstruction &inst) {
    auto nt = inst.targets.size();
    size_t offset = measurement_record.storage.size();
    measurement_record.storage.insert(measurement_record.storage.end(), nt, false);

    uint64_t rng_buf = 0;
    size_t buf_size = 0;
    RareErrorIterator::for_samples(inst.args[0], nt, rng, [&](size_t target) {
        auto qubit = inst.targets[target].qubit_value();
        if (buf_size == 0) {
            rng_buf = rng();
            buf_size = 64;
        }
        inv_state.xs.signs[qubit] ^= (bool)(rng_buf & 1);
        inv_state.zs.signs[qubit] ^= (bool)(rng_buf & 2);
        measurement_record.storage[offset + target] = true;
        rng_buf >>= 2;
        buf_size -= 2;
    });
}

template <size_t W>
void TableauSimulator<W>::do_HERALDED_PAULI_CHANNEL_1(const CircuitInstruction &inst) {
    auto nt = inst.targets.size();
    size_t offset = measurement_record.storage.size();
    measurement_record.storage.insert(measurement_record.storage.end(), nt, false);

    double hi = inst.args[0];
    double hx = inst.args[1];
    double hy = inst.args[2];
    double hz = inst.args[3];
    double ht = std::min(1.0, hi + hx + hy + hz);
    std::array<double, 3> conditionals{hx, hy, hz};
    if (ht != 0) {
        conditionals[0] /= ht;
        conditionals[1] /= ht;
        conditionals[2] /= ht;
    }
    RareErrorIterator::for_samples(ht, nt, rng, [&](size_t target) {
        measurement_record.storage[offset + target] = true;
        do_PAULI_CHANNEL_1(CircuitInstruction{GateType::PAULI_CHANNEL_1, conditionals, &inst.targets[target], ""});
    });
}

template <size_t W>
void TableauSimulator<W>::do_X_ERROR(const CircuitInstruction &target_data) {
    RareErrorIterator::for_samples(target_data.args[0], target_data.targets, rng, [&](GateTarget q) {
        inv_state.zs.signs[q.data] ^= true;
    });
}

template <size_t W>
void TableauSimulator<W>::do_Y_ERROR(const CircuitInstruction &target_data) {
    RareErrorIterator::for_samples(target_data.args[0], target_data.targets, rng, [&](GateTarget q) {
        inv_state.xs.signs[q.data] ^= true;
        inv_state.zs.signs[q.data] ^= true;
    });
}

template <size_t W>
void TableauSimulator<W>::do_Z_ERROR(const CircuitInstruction &target_data) {
    RareErrorIterator::for_samples(target_data.args[0], target_data.targets, rng, [&](GateTarget q) {
        inv_state.xs.signs[q.data] ^= true;
    });
}

template <size_t W>
void TableauSimulator<W>::do_PAULI_CHANNEL_1(const CircuitInstruction &target_data) {
    bool tmp = last_correlated_error_occurred;
    perform_pauli_errors_via_correlated_errors<1>(
        target_data,
        [&]() {
            last_correlated_error_occurred = false;
        },
        [&](const CircuitInstruction &d) {
            do_ELSE_CORRELATED_ERROR(d);
        });
    last_correlated_error_occurred = tmp;
}

template <size_t W>
void TableauSimulator<W>::do_PAULI_CHANNEL_2(const CircuitInstruction &target_data) {
    bool tmp = last_correlated_error_occurred;
    perform_pauli_errors_via_correlated_errors<2>(
        target_data,
        [&]() {
            last_correlated_error_occurred = false;
        },
        [&](const CircuitInstruction &d) {
            do_ELSE_CORRELATED_ERROR(d);
        });
    last_correlated_error_occurred = tmp;
}

template <size_t W>
void TableauSimulator<W>::do_CORRELATED_ERROR(const CircuitInstruction &target_data) {
    last_correlated_error_occurred = false;
    do_ELSE_CORRELATED_ERROR(target_data);
}

template <size_t W>
void TableauSimulator<W>::do_ELSE_CORRELATED_ERROR(const CircuitInstruction &target_data) {
    if (last_correlated_error_occurred) {
        return;
    }
    last_correlated_error_occurred = std::bernoulli_distribution(target_data.args[0])(rng);
    if (!last_correlated_error_occurred) {
        return;
    }
    for (auto qxz : target_data.targets) {
        auto q = qxz.qubit_value();
        if (qxz.data & TARGET_PAULI_X_BIT) {
            inv_state.prepend_X(q);
        }
        if (qxz.data & TARGET_PAULI_Z_BIT) {
            inv_state.prepend_Z(q);
        }
    }
}

template <size_t W>
void TableauSimulator<W>::do_X(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_X(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_Y(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_Y(q.data);
    }
}

template <size_t W>
void TableauSimulator<W>::do_Z(const CircuitInstruction &target_data) {
    const auto &targets = target_data.targets;
    for (auto q : targets) {
        inv_state.prepend_Z(q.data);
    }
}

template <size_t W>
simd_bits<W> TableauSimulator<W>::sample_circuit(const Circuit &circuit, std::mt19937_64 &rng, int8_t sign_bias) {
    TableauSimulator<W> sim(std::move(rng), circuit.count_qubits(), sign_bias);
    sim.safe_do_circuit(circuit);

    const std::vector<bool> &v = sim.measurement_record.storage;
    simd_bits<W> result(v.size());
    for (size_t k = 0; k < v.size(); k++) {
        result[k] ^= v[k];
    }
    rng = std::move(sim.rng);
    return result;
}

template <size_t W>
void TableauSimulator<W>::ensure_large_enough_for_qubits(size_t num_qubits) {
    if (num_qubits <= inv_state.num_qubits) {
        return;
    }
    inv_state.expand(num_qubits, 1.1);
}

template <size_t W>
void TableauSimulator<W>::sample_stream(
    FILE *in, FILE *out, SampleFormat format, bool interactive, std::mt19937_64 &rng) {
    TableauSimulator<W> sim(std::move(rng), 1);
    auto writer = MeasureRecordWriter::make(out, format);
    Circuit unprocessed;
    while (true) {
        unprocessed.clear();
        if (interactive) {
            try {
                unprocessed.append_from_file(in, true);
            } catch (const std::exception &ex) {
                std::cerr << "\033[31m" << ex.what() << "\033[0m\n";
                continue;
            }
        } else {
            unprocessed.append_from_file(in, true);
        }
        if (unprocessed.operations.empty()) {
            break;
        }
        sim.ensure_large_enough_for_qubits(unprocessed.count_qubits());

        unprocessed.for_each_operation([&](const CircuitInstruction &op) {
            sim.do_gate(op);
            sim.measurement_record.write_unwritten_results_to(*writer);
            if (interactive && op.count_measurement_results()) {
                putc('\n', out);
                fflush(out);
            }
        });
    }
    rng = std::move(sim.rng);
    writer->write_end();
}

template <size_t W>
VectorSimulator TableauSimulator<W>::to_vector_sim() const {
    auto inv = inv_state.inverse();
    std::vector<PauliStringRef<W>> stabilizers;
    for (size_t k = 0; k < inv.num_qubits; k++) {
        stabilizers.push_back(inv.zs[k]);
    }
    return VectorSimulator::from_stabilizers<W>(stabilizers);
}

template <size_t W>
void TableauSimulator<W>::apply_tableau(const Tableau<W> &tableau, const std::vector<size_t> &targets) {
    inv_state.inplace_scatter_prepend(tableau.inverse(), targets);
}

template <size_t W>
std::vector<std::complex<float>> TableauSimulator<W>::to_state_vector(bool little_endian) const {
    auto sim = to_vector_sim();
    if (!little_endian && inv_state.num_qubits > 0) {
        for (size_t q = 0; q < inv_state.num_qubits - q - 1; q++) {
            sim.apply(GateType::SWAP, q, inv_state.num_qubits - q - 1);
        }
    }
    return sim.state;
}

template <size_t W>
void TableauSimulator<W>::collapse_x(SpanRef<const GateTarget> targets, size_t stride) {
    // Find targets that need to be collapsed.
    std::set<GateTarget> unique_collapse_targets;
    for (size_t k = 0; k < targets.size(); k += stride) {
        GateTarget t = targets[k];
        t.data &= TARGET_VALUE_MASK;
        if (!is_deterministic_x(t.data)) {
            unique_collapse_targets.insert(t);
        }
    }

    // Only pay the cost of transposing if collapsing is needed.
    if (!unique_collapse_targets.empty()) {
        std::vector<GateTarget> collapse_targets(unique_collapse_targets.begin(), unique_collapse_targets.end());
        do_H_XZ({GateType::H, {}, collapse_targets, ""});
        {
            TableauTransposedRaii<W> temp_transposed(inv_state);
            for (auto q : collapse_targets) {
                collapse_qubit_z(q.data, temp_transposed);
            }
        }
        do_H_XZ({GateType::H, {}, collapse_targets, ""});
    }
}

template <size_t W>
void TableauSimulator<W>::collapse_y(SpanRef<const GateTarget> targets, size_t stride) {
    // Find targets that need to be collapsed.
    std::set<GateTarget> unique_collapse_targets;
    for (size_t k = 0; k < targets.size(); k += stride) {
        GateTarget t = targets[k];
        t.data &= TARGET_VALUE_MASK;
        if (!is_deterministic_y(t.data)) {
            unique_collapse_targets.insert(t);
        }
    }

    // Only pay the cost of transposing if collapsing is needed.
    if (!unique_collapse_targets.empty()) {
        std::vector<GateTarget> collapse_targets(unique_collapse_targets.begin(), unique_collapse_targets.end());
        do_H_YZ({GateType::H_YZ, {}, collapse_targets, ""});
        {
            TableauTransposedRaii<W> temp_transposed(inv_state);
            for (auto q : collapse_targets) {
                collapse_qubit_z(q.data, temp_transposed);
            }
        }
        do_H_YZ({GateType::H_YZ, {}, collapse_targets, ""});
    }
}

template <size_t W>
void TableauSimulator<W>::collapse_z(SpanRef<const GateTarget> targets, size_t stride) {
    // Find targets that need to be collapsed.
    std::vector<GateTarget> collapse_targets;
    collapse_targets.reserve(targets.size());
    for (size_t k = 0; k < targets.size(); k += stride) {
        GateTarget t = targets[k];
        t.data &= TARGET_VALUE_MASK;
        if (!is_deterministic_z(t.data)) {
            collapse_targets.push_back(t);
        }
    }

    // Only pay the cost of transposing if collapsing is needed.
    if (!collapse_targets.empty()) {
        TableauTransposedRaii<W> temp_transposed(inv_state);
        for (auto target : collapse_targets) {
            collapse_qubit_z(target.data, temp_transposed);
        }
    }
}

template <size_t W>
size_t TableauSimulator<W>::collapse_qubit_z(size_t target, TableauTransposedRaii<W> &transposed_raii) {
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

template <size_t W>
void TableauSimulator<W>::collapse_isolate_qubit_z(size_t target, TableauTransposedRaii<W> &transposed_raii) {
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
            int p = transposed_raii.tableau.xs.xt[q][target] + 2 * transposed_raii.tableau.xs.zt[q][target];
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

template <size_t W>
void TableauSimulator<W>::safe_do_circuit(const Circuit &circuit, uint64_t reps) {
    ensure_large_enough_for_qubits(circuit.count_qubits());
    for (uint64_t k = 0; k < reps; k++) {
        circuit.for_each_operation([&](const CircuitInstruction &op) {
            do_gate(op);
        });
    }
}

template <size_t W>
simd_bits<W> TableauSimulator<W>::reference_sample_circuit(const Circuit &circuit) {
    std::mt19937_64 irrelevant_rng(0);
    return TableauSimulator<W>::sample_circuit(circuit.aliased_noiseless_circuit(), irrelevant_rng, +1);
}

template <size_t W>
void TableauSimulator<W>::paulis(const PauliString<W> &paulis) {
    auto nw = paulis.xs.num_simd_words;
    inv_state.zs.signs.word_range_ref(0, nw) ^= paulis.xs;
    inv_state.xs.signs.word_range_ref(0, nw) ^= paulis.zs;
}

template <size_t W>
void TableauSimulator<W>::do_operation_ensure_size(const CircuitInstruction &operation) {
    uint64_t n = 0;
    for (const auto &t : operation.targets) {
        if (t.has_qubit_value()) {
            n = std::max(n, (uint64_t)t.qubit_value() + 1);
        }
    }
    ensure_large_enough_for_qubits(n);
    do_gate(operation);
}

template <size_t W>
void TableauSimulator<W>::set_num_qubits(size_t new_num_qubits) {
    if (new_num_qubits >= inv_state.num_qubits) {
        ensure_large_enough_for_qubits(new_num_qubits);
        return;
    }

    // Collapse qubits past the new size and ensure the internal state totally decouples them.
    {
        TableauTransposedRaii<W> temp_transposed(inv_state);
        for (size_t q = new_num_qubits; q < inv_state.num_qubits; q++) {
            collapse_isolate_qubit_z(q, temp_transposed);
        }
    }

    Tableau<W> old_state = std::move(inv_state);
    inv_state = Tableau<W>(new_num_qubits);
    inv_state.xs.signs.truncated_overwrite_from(old_state.xs.signs, new_num_qubits);
    inv_state.zs.signs.truncated_overwrite_from(old_state.zs.signs, new_num_qubits);
    for (size_t q = 0; q < new_num_qubits; q++) {
        inv_state.xs[q].xs.truncated_overwrite_from(old_state.xs[q].xs, new_num_qubits);
        inv_state.xs[q].zs.truncated_overwrite_from(old_state.xs[q].zs, new_num_qubits);
        inv_state.zs[q].xs.truncated_overwrite_from(old_state.zs[q].xs, new_num_qubits);
        inv_state.zs[q].zs.truncated_overwrite_from(old_state.zs[q].zs, new_num_qubits);
    }
}

template <size_t W>
std::pair<bool, PauliString<W>> TableauSimulator<W>::measure_kickback_z(GateTarget target) {
    bool flipped = target.is_inverted_result_target();
    uint32_t q = target.qubit_value();
    PauliString<W> kickback(0);
    bool has_kickback = !is_deterministic_z(q);  // Note: do this before transposing the state!

    {
        TableauTransposedRaii<W> temp_transposed(inv_state);
        if (has_kickback) {
            size_t pivot = collapse_qubit_z(q, temp_transposed);
            kickback = temp_transposed.unsigned_x_input(pivot);
        }
        bool result = inv_state.zs.signs[q] ^ flipped;
        measurement_record.storage.push_back(result);

        // Prevent later measure_kickback calls from unnecessarily targeting this qubit with a Z gate.
        collapse_isolate_qubit_z(q, temp_transposed);

        return {result, kickback};
    }
}

template <size_t W>
std::pair<bool, PauliString<W>> TableauSimulator<W>::measure_kickback_y(GateTarget target) {
    do_H_YZ({GateType::H, {}, &target, ""});
    auto result = measure_kickback_z(target);
    do_H_YZ({GateType::H, {}, &target, ""});
    if (result.second.num_qubits) {
        // Also conjugate the kickback by H_YZ.
        result.second.xs[target.qubit_value()] ^= result.second.zs[target.qubit_value()];
    }
    return result;
}

template <size_t W>
std::pair<bool, PauliString<W>> TableauSimulator<W>::measure_kickback_x(GateTarget target) {
    do_H_XZ({GateType::H, {}, &target, ""});
    auto result = measure_kickback_z(target);
    do_H_XZ({GateType::H, {}, &target, ""});
    if (result.second.num_qubits) {
        // Also conjugate the kickback by H_XZ.
        result.second.xs[target.qubit_value()].swap_with(result.second.zs[target.qubit_value()]);
    }
    return result;
}

template <size_t W>
std::vector<PauliString<W>> TableauSimulator<W>::canonical_stabilizers() const {
    return inv_state.inverse().stabilizers(true);
}

template <size_t W>
int8_t TableauSimulator<W>::peek_observable_expectation(const PauliString<W> &observable) const {
    TableauSimulator<W> state = *this;

    // Kick the observable onto an ancilla qubit's Z observable.
    auto n = (uint32_t)std::max(state.inv_state.num_qubits, observable.num_qubits);
    state.ensure_large_enough_for_qubits(n + 1);
    GateTarget anc{n};
    if (observable.sign) {
        state.do_X({GateType::X, {}, &anc, ""});
    }
    observable.ref().for_each_active_pauli([&](size_t q) {
        int p = observable.xs[q] + (observable.zs[q] << 1);
        std::array<GateTarget, 2> targets{GateTarget{(uint32_t)q}, anc};
        GateType c2_type = GateType::CX;
        if (p == 1) {
            c2_type = GateType::XCX;
        } else if (p == 3) {
            c2_type = GateType::YCX;
        }
        state.do_gate({c2_type, {}, targets, ""});
    });

    // Use simulator features to determines if the measurement is deterministic.
    if (!state.is_deterministic_z(anc.data)) {
        return 0;
    }
    state.do_MZ({GateType::M, {}, &anc, ""});
    return state.measurement_record.storage.back() ? -1 : +1;
}

template <size_t W>
void TableauSimulator<W>::do_gate(const CircuitInstruction &inst) {
    switch (inst.gate_type) {
        case GateType::DETECTOR:
            do_I(inst);
            break;
        case GateType::OBSERVABLE_INCLUDE:
            do_I(inst);
            break;
        case GateType::TICK:
            do_I(inst);
            break;
        case GateType::QUBIT_COORDS:
            do_I(inst);
            break;
        case GateType::SHIFT_COORDS:
            do_I(inst);
            break;
        case GateType::REPEAT:
            do_I(inst);
            break;
        case GateType::MX:
            do_MX(inst);
            break;
        case GateType::MY:
            do_MY(inst);
            break;
        case GateType::M:
            do_MZ(inst);
            break;
        case GateType::MRX:
            do_MRX(inst);
            break;
        case GateType::MRY:
            do_MRY(inst);
            break;
        case GateType::MR:
            do_MRZ(inst);
            break;
        case GateType::RX:
            do_RX(inst);
            break;
        case GateType::RY:
            do_RY(inst);
            break;
        case GateType::R:
            do_RZ(inst);
            break;
        case GateType::MPP:
            do_MPP(inst);
            break;
        case GateType::SPP:
            do_SPP(inst);
            break;
        case GateType::SPP_DAG:
            do_SPP_DAG(inst);
            break;
        case GateType::MXX:
            do_MXX(inst);
            break;
        case GateType::MYY:
            do_MYY(inst);
            break;
        case GateType::MZZ:
            do_MZZ(inst);
            break;
        case GateType::MPAD:
            do_MPAD(inst);
            break;
        case GateType::XCX:
            do_XCX(inst);
            break;
        case GateType::XCY:
            do_XCY(inst);
            break;
        case GateType::XCZ:
            do_XCZ(inst);
            break;
        case GateType::YCX:
            do_YCX(inst);
            break;
        case GateType::YCY:
            do_YCY(inst);
            break;
        case GateType::YCZ:
            do_YCZ(inst);
            break;
        case GateType::CX:
            do_ZCX(inst);
            break;
        case GateType::CY:
            do_ZCY(inst);
            break;
        case GateType::CZ:
            do_ZCZ(inst);
            break;
        case GateType::H:
            do_H_XZ(inst);
            break;
        case GateType::H_XY:
            do_H_XY(inst);
            break;
        case GateType::H_YZ:
            do_H_YZ(inst);
            break;
        case GateType::H_NXY:
            do_H_NXY(inst);
            break;
        case GateType::H_NXZ:
            do_H_NXZ(inst);
            break;
        case GateType::H_NYZ:
            do_H_NYZ(inst);
            break;
        case GateType::DEPOLARIZE1:
            do_DEPOLARIZE1(inst);
            break;
        case GateType::DEPOLARIZE2:
            do_DEPOLARIZE2(inst);
            break;
        case GateType::X_ERROR:
            do_X_ERROR(inst);
            break;
        case GateType::Y_ERROR:
            do_Y_ERROR(inst);
            break;
        case GateType::Z_ERROR:
            do_Z_ERROR(inst);
            break;
        case GateType::PAULI_CHANNEL_1:
            do_PAULI_CHANNEL_1(inst);
            break;
        case GateType::PAULI_CHANNEL_2:
            do_PAULI_CHANNEL_2(inst);
            break;
        case GateType::E:
            do_CORRELATED_ERROR(inst);
            break;
        case GateType::ELSE_CORRELATED_ERROR:
            do_ELSE_CORRELATED_ERROR(inst);
            break;
        case GateType::I:
        case GateType::II:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
            do_I(inst);
            break;
        case GateType::X:
            do_X(inst);
            break;
        case GateType::Y:
            do_Y(inst);
            break;
        case GateType::Z:
            do_Z(inst);
            break;
        case GateType::C_XYZ:
            do_C_XYZ(inst);
            break;
        case GateType::C_NXYZ:
            do_C_NXYZ(inst);
            break;
        case GateType::C_XNYZ:
            do_C_XNYZ(inst);
            break;
        case GateType::C_XYNZ:
            do_C_XYNZ(inst);
            break;
        case GateType::C_ZYX:
            do_C_ZYX(inst);
            break;
        case GateType::C_NZYX:
            do_C_NZYX(inst);
            break;
        case GateType::C_ZNYX:
            do_C_ZNYX(inst);
            break;
        case GateType::C_ZYNX:
            do_C_ZYNX(inst);
            break;
        case GateType::SQRT_X:
            do_SQRT_X(inst);
            break;
        case GateType::SQRT_X_DAG:
            do_SQRT_X_DAG(inst);
            break;
        case GateType::SQRT_Y:
            do_SQRT_Y(inst);
            break;
        case GateType::SQRT_Y_DAG:
            do_SQRT_Y_DAG(inst);
            break;
        case GateType::S:
            do_SQRT_Z(inst);
            break;
        case GateType::S_DAG:
            do_SQRT_Z_DAG(inst);
            break;
        case GateType::SQRT_XX:
            do_SQRT_XX(inst);
            break;
        case GateType::SQRT_XX_DAG:
            do_SQRT_XX_DAG(inst);
            break;
        case GateType::SQRT_YY:
            do_SQRT_YY(inst);
            break;
        case GateType::SQRT_YY_DAG:
            do_SQRT_YY_DAG(inst);
            break;
        case GateType::SQRT_ZZ:
            do_SQRT_ZZ(inst);
            break;
        case GateType::SQRT_ZZ_DAG:
            do_SQRT_ZZ_DAG(inst);
            break;
        case GateType::SWAP:
            do_SWAP(inst);
            break;
        case GateType::ISWAP:
            do_ISWAP(inst);
            break;
        case GateType::ISWAP_DAG:
            do_ISWAP_DAG(inst);
            break;
        case GateType::CXSWAP:
            do_CXSWAP(inst);
            break;
        case GateType::CZSWAP:
            do_CZSWAP(inst);
            break;
        case GateType::SWAPCX:
            do_SWAPCX(inst);
            break;
        case GateType::HERALDED_ERASE:
            do_HERALDED_ERASE(inst);
            break;
        case GateType::HERALDED_PAULI_CHANNEL_1:
            do_HERALDED_PAULI_CHANNEL_1(inst);
            break;
        default:
            throw std::invalid_argument(
                "Not implemented by TableauSimulator::do_gate: " + std::string(GATE_DATA[inst.gate_type].name));
    }
}

}  // namespace stim
