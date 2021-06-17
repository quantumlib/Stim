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

#include "error_analyzer.h"

#include <algorithm>
#include <iomanip>
#include <queue>
#include <sstream>

using namespace stim_internal;

void ErrorAnalyzer::remove_gauge(ConstPointerRange<DemTarget> sorted) {
    if (sorted.empty()) {
        return;
    }
    const auto &max = sorted.back();
    // HACK: linear overhead due to not keeping an index of which detectors used where.
    for (auto &x : xs) {
        if (x.contains(max)) {
            x.xor_sorted_items(sorted);
        }
    }
    for (auto &z : zs) {
        if (z.contains(max)) {
            z.xor_sorted_items(sorted);
        }
    }
}

void ErrorAnalyzer::RX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        check_for_gauge(zs[q]);
        xs[q].clear();
        zs[q].clear();
    }
}

void ErrorAnalyzer::RY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        check_for_gauge(xs[q], zs[q]);
        xs[q].clear();
        zs[q].clear();
    }
}

void ErrorAnalyzer::RZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        check_for_gauge(xs[q]);
        xs[q].clear();
        zs[q].clear();
    }
}

void ErrorAnalyzer::check_for_gauge(
    SparseXorVec<DemTarget> &potential_gauge_summand_1, SparseXorVec<DemTarget> &potential_gauge_summand_2) {
    if (potential_gauge_summand_1 == potential_gauge_summand_2) {
        return;
    }
    potential_gauge_summand_1 ^= potential_gauge_summand_2;
    check_for_gauge(potential_gauge_summand_1);
}

void ErrorAnalyzer::check_for_gauge(const SparseXorVec<DemTarget> &potential_gauge) {
    if (potential_gauge.empty()) {
        return;
    }
    if (!allow_gauge_detectors) {
        throw std::invalid_argument("A detector or observable anti-commuted with a measurement or reset.");
    }
    for (const auto &t : potential_gauge) {
        if (t.is_observable_id()) {
            throw std::invalid_argument("An observable anti-commuted with a measurement or reset.");
        }
    }
    remove_gauge(add_error(0.5, potential_gauge.range()));
}

void ErrorAnalyzer::MX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<DemTarget> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        xs[q].xor_sorted_items(d);
        check_for_gauge(zs[q]);
    }
}

void ErrorAnalyzer::MY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<DemTarget> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        xs[q].xor_sorted_items(d);
        zs[q].xor_sorted_items(d);
        check_for_gauge(xs[q], zs[q]);
    }
}

void ErrorAnalyzer::MZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<DemTarget> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        zs[q].xor_sorted_items(d);
        check_for_gauge(xs[q]);
    }
}

void ErrorAnalyzer::MRX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{{}, {&q, &q + 1}};
        RX(d);
        MX(d);
    }
}

void ErrorAnalyzer::MRY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{{}, {&q, &q + 1}};
        RY(d);
        MY(d);
    }
}

void ErrorAnalyzer::MRZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{{}, {&q, &q + 1}};
        RZ(d);
        MZ(d);
    }
}

void ErrorAnalyzer::H_XZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        std::swap(xs[q], zs[q]);
    }
}

void ErrorAnalyzer::H_XY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        zs[q] ^= xs[q];
    }
}

void ErrorAnalyzer::H_YZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        xs[q] ^= zs[q];
    }
}

void ErrorAnalyzer::C_XYZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

void ErrorAnalyzer::C_ZYX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

void ErrorAnalyzer::XCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k];
        auto q2 = dat.targets[k + 1];
        xs[q1] ^= zs[q2];
        xs[q2] ^= zs[q1];
    }
}

void ErrorAnalyzer::XCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k];
        auto ty = dat.targets[k + 1];
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void ErrorAnalyzer::YCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k + 1];
        auto ty = dat.targets[k];
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void ErrorAnalyzer::ZCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        single_cy(c, t);
    }
}

void ErrorAnalyzer::YCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        single_cy(c, t);
    }
}

void ErrorAnalyzer::YCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        zs[a] ^= xs[b];
        zs[a] ^= zs[b];
        xs[a] ^= xs[b];
        xs[a] ^= zs[b];

        zs[b] ^= xs[a];
        zs[b] ^= zs[a];
        xs[b] ^= xs[a];
        xs[b] ^= zs[a];
    }
}

void ErrorAnalyzer::ZCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        single_cx(c, t);
    }
}

void ErrorAnalyzer::SQRT_XX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        xs[a] ^= zs[a];
        xs[a] ^= zs[b];
        xs[b] ^= zs[a];
        xs[b] ^= zs[b];
    }
}

void ErrorAnalyzer::SQRT_YY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        zs[a] ^= xs[a];
        zs[b] ^= xs[b];
        xs[a] ^= zs[a];
        xs[a] ^= zs[b];
        xs[b] ^= zs[a];
        xs[b] ^= zs[b];
        zs[a] ^= xs[a];
        zs[b] ^= xs[b];
    }
}

void ErrorAnalyzer::SQRT_ZZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
    }
}

void ErrorAnalyzer::feedback(uint32_t record_control, size_t target, bool x, bool z) {
    uint64_t time = scheduled_measurement_time + (record_control & ~TARGET_RECORD_BIT);
    std::vector<DemTarget> &dst = measurement_to_detectors[time];

    // Temporarily move map's vector data into a SparseXorVec for manipulation.
    std::sort(dst.begin(), dst.end());
    SparseXorVec<DemTarget> tmp(std::move(dst));

    if (x) {
        tmp ^= xs[target];
    }
    if (z) {
        tmp ^= zs[target];
    }

    // Move data back into the map.
    dst = std::move(tmp.sorted_items);
}

void ErrorAnalyzer::single_cx(uint32_t c, uint32_t t) {
    if (!((c | t) & TARGET_RECORD_BIT)) {
        zs[c] ^= zs[t];
        xs[t] ^= xs[c];
    } else if (t & TARGET_RECORD_BIT) {
        throw std::out_of_range("Measurement record editing is not supported.");
    } else {
        feedback(c, t, false, true);
    }
}

void ErrorAnalyzer::single_cy(uint32_t c, uint32_t t) {
    if (!((c | t) & TARGET_RECORD_BIT)) {
        zs[c] ^= zs[t];
        zs[c] ^= xs[t];
        xs[t] ^= xs[c];
        zs[t] ^= xs[c];
    } else if (t & TARGET_RECORD_BIT) {
        throw std::out_of_range("Measurement record editing is not supported.");
    } else {
        feedback(c, t, true, true);
    }
}

void ErrorAnalyzer::single_cz(uint32_t c, uint32_t t) {
    if (!((c | t) & TARGET_RECORD_BIT)) {
        zs[c] ^= xs[t];
        zs[t] ^= xs[c];
    } else if (c & TARGET_RECORD_BIT) {
        feedback(c, t, true, false);
    } else if (t & TARGET_RECORD_BIT) {
        feedback(t, c, true, false);
    } else {
        // No effect.
    }
}

void ErrorAnalyzer::XCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        single_cx(c, t);
    }
}

void ErrorAnalyzer::ZCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k];
        auto q2 = dat.targets[k + 1];
        single_cz(q1, q2);
    }
}

void ErrorAnalyzer::I(const OperationData &dat) {
}

void ErrorAnalyzer::SWAP(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void ErrorAnalyzer::ISWAP(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void ErrorAnalyzer::DETECTOR(const OperationData &dat) {
    used_detectors++;
    auto id = DemTarget::relative_detector_id(total_detectors - used_detectors);
    for (auto t : dat.targets) {
        auto delay = t & TARGET_VALUE_MASK;
        measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
    }
    flushed_reversed_model.append_detector_instruction(dat.args, id);
}

void ErrorAnalyzer::OBSERVABLE_INCLUDE(const OperationData &dat) {
    auto id = DemTarget::observable_id((int32_t)dat.args[0]);
    for (auto t : dat.targets) {
        auto delay = t & TARGET_VALUE_MASK;
        measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
    }
    flushed_reversed_model.append_logical_observable_instruction(id);
}

ErrorAnalyzer::ErrorAnalyzer(
    uint64_t num_detectors, size_t num_qubits, bool decompose_errors, bool fold_loops, bool allow_gauge_detectors)
    : total_detectors(num_detectors),
      used_detectors(0),
      xs(num_qubits),
      zs(num_qubits),
      scheduled_measurement_time(0),
      decompose_errors(decompose_errors),
      accumulate_errors(true),
      fold_loops(fold_loops),
      allow_gauge_detectors(allow_gauge_detectors) {
}

void ErrorAnalyzer::run_circuit(const Circuit &circuit) {
    for (auto p = circuit.operations.crbegin(); p != circuit.operations.crend(); p++) {
        const auto &op = *p;
        assert(op.gate != nullptr);
        if (op.gate->id == gate_name_to_id("REPEAT")) {
            assert(op.target_data.targets.size() == 3);
            assert(op.target_data.targets[0] < circuit.blocks.size());
            uint64_t repeats = op_data_rep_count(op.target_data);
            const auto &block = circuit.blocks[op.target_data.targets[0]];
            run_loop(block, repeats);
        } else {
            (this->*op.gate->reverse_error_analyzer_function)(op.target_data);
        }
    }
}

void ErrorAnalyzer::post_check_initialization() {
    for (const auto &x : xs) {
        check_for_gauge(x);
    }
}

void ErrorAnalyzer::X_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_error(dat.args[0], zs[q].range());
    }
}

void ErrorAnalyzer::Y_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_xored_error(dat.args[0], xs[q].range(), zs[q].range());
    }
}

void ErrorAnalyzer::Z_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_error(dat.args[0], xs[q].range());
    }
}

template <typename T>
inline void inplace_xor_tail(MonotonicBuffer<T> &dst, const SparseXorVec<T> &src) {
    ConstPointerRange<T> in1 = dst.tail;
    ConstPointerRange<T> in2 = src.range();
    xor_merge_sort_temp_buffer_callback(in1, in2, [&](ConstPointerRange<T> result) {
        dst.discard_tail();
        dst.append_tail(result);
    });
}

void ErrorAnalyzer::CORRELATED_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto qp : dat.targets) {
        auto q = qp & TARGET_VALUE_MASK;
        if (qp & TARGET_PAULI_Z_BIT) {
            inplace_xor_tail(mono_buf, xs[q]);
        }
        if (qp & TARGET_PAULI_X_BIT) {
            inplace_xor_tail(mono_buf, zs[q]);
        }
    }
    add_error_in_sorted_jagged_tail(dat.args[0]);
}

void ErrorAnalyzer::DEPOLARIZE1(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    if (dat.args[0] >= 3.0 / 4.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 3/4 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * sqrt(1 - (4 * dat.args[0]) / 3);
    for (auto q : dat.targets) {
        add_error_combinations<2>(
            p,
            {
                xs[q].range(),
                zs[q].range(),
            });
    }
}

void ErrorAnalyzer::DEPOLARIZE2(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    if (dat.args[0] >= 15.0 / 16.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 15/16 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * pow(1 - (16 * dat.args[0]) / 15, 0.125);
    for (size_t i = 0; i < dat.targets.size(); i += 2) {
        auto a = dat.targets[i];
        auto b = dat.targets[i + 1];
        add_error_combinations<4>(
            p,
            {
                xs[a].range(),
                zs[a].range(),
                xs[b].range(),
                zs[b].range(),
            });
    }
}

void ErrorAnalyzer::ELSE_CORRELATED_ERROR(const OperationData &dat) {
    throw std::out_of_range(
        "ELSE_CORRELATED_ERROR operations currently not supported in error analysis (cases may not be independent).");
}

void ErrorAnalyzer::PAULI_CHANNEL_1(const OperationData &dat) {
    throw std::out_of_range(
        "PAULI_CHANNEL_1 operations currently not supported in error analysis (cases may not be independent).");
}

void ErrorAnalyzer::PAULI_CHANNEL_2(const OperationData &dat) {
    throw std::out_of_range(
        "PAULI_CHANNEL_2 operations currently not supported in error analysis (cases may not be independent).");
}

DetectorErrorModel unreversed(const DetectorErrorModel &rev, uint64_t &base_detector_id, std::set<DemTarget> &seen) {
    DetectorErrorModel out;
    auto conv_append = [&](const DemInstruction &e) {
        auto stored_targets = out.target_buf.take_copy(e.target_data);
        auto stored_args = out.arg_buf.take_copy(e.arg_data);
        for (auto &t : stored_targets) {
            t.shift_if_detector_id(-(int64_t)base_detector_id);
        }
        out.instructions.push_back(DemInstruction{stored_args, stored_targets, e.type});
    };

    for (auto p = rev.instructions.crbegin(); p != rev.instructions.crend(); p++) {
        const auto &e = *p;
        switch (e.type) {
            case DEM_SHIFT_DETECTORS:
                base_detector_id += e.target_data[0].data;
                out.append_shift_detectors_instruction(e.arg_data, e.target_data[0].data);
                break;
            case DEM_ERROR:
                for (auto &t : e.target_data) {
                    seen.insert(t);
                }
                conv_append(e);
                break;
            case DEM_DETECTOR:
            case DEM_LOGICAL_OBSERVABLE:
                if (!e.arg_data.empty() || seen.find(e.target_data[0]) == seen.end()) {
                    conv_append(e);
                }
                break;
            case DEM_REPEAT_BLOCK: {
                uint64_t repetitions = e.target_data[0].data;
                if (repetitions) {
                    uint64_t old_base_detector_id = base_detector_id;
                    out.append_repeat_block(
                        e.target_data[0].data, unreversed(rev.blocks[e.target_data[1].data], base_detector_id, seen));
                    uint64_t loop_shift = base_detector_id - old_base_detector_id;
                    base_detector_id += loop_shift * (repetitions - 1);
                }
            } break;
            default:
                throw std::out_of_range("Unknown instruction type to unreversed.");
        }
    }
    return out;
}

DetectorErrorModel ErrorAnalyzer::circuit_to_detector_error_model(
    const Circuit &circuit, bool decompose_errors, bool fold_loops, bool allow_gauge_detectors) {
    ErrorAnalyzer analyzer(
        circuit.count_detectors(), circuit.count_qubits(), decompose_errors, fold_loops, allow_gauge_detectors);
    analyzer.run_circuit(circuit);
    analyzer.post_check_initialization();
    analyzer.flush();
    uint64_t t = 0;
    std::set<DemTarget> seen;
    return unreversed(analyzer.flushed_reversed_model, t, seen);
}

void ErrorAnalyzer::flush() {
    for (auto kv = error_class_probabilities.crbegin(); kv != error_class_probabilities.crend(); kv++) {
        if (kv->first.empty() || kv->second == 0) {
            continue;
        }
        flushed_reversed_model.append_error_instruction(kv->second, kv->first);
    }
    error_class_probabilities.clear();
}

ConstPointerRange<DemTarget> ErrorAnalyzer::add_xored_error(
    double probability, ConstPointerRange<DemTarget> flipped1, ConstPointerRange<DemTarget> flipped2) {
    mono_buf.ensure_available(flipped1.size() + flipped2.size());
    mono_buf.tail.ptr_end = xor_merge_sort(flipped1, flipped2, mono_buf.tail.ptr_end);
    return add_error_in_sorted_jagged_tail(probability);
}

ConstPointerRange<DemTarget> ErrorAnalyzer::mono_dedupe_store_tail() {
    auto v = error_class_probabilities.find(mono_buf.tail);
    if (v != error_class_probabilities.end()) {
        mono_buf.discard_tail();
        return v->first;
    }
    auto result = mono_buf.commit_tail();
    error_class_probabilities.insert({result, 0});
    return result;
}

ConstPointerRange<DemTarget> ErrorAnalyzer::mono_dedupe_store(ConstPointerRange<DemTarget> data) {
    auto v = error_class_probabilities.find(data);
    if (v != error_class_probabilities.end()) {
        return v->first;
    }
    mono_buf.append_tail(data);
    auto result = mono_buf.commit_tail();
    error_class_probabilities.insert({result, 0});
    return result;
}

ConstPointerRange<DemTarget> ErrorAnalyzer::add_error(double probability, ConstPointerRange<DemTarget> flipped) {
    auto key = mono_dedupe_store(flipped);
    auto &old_p = error_class_probabilities[key];
    old_p = old_p * (1 - probability) + (1 - old_p) * probability;
    return key;
}

ConstPointerRange<DemTarget> ErrorAnalyzer::add_error_in_sorted_jagged_tail(double probability) {
    auto key = mono_dedupe_store_tail();
    auto &old_p = error_class_probabilities[key];
    old_p = old_p * (1 - probability) + (1 - old_p) * probability;
    return key;
}

bool shifted_equals(int64_t shift, const SparseXorVec<DemTarget> &unshifted, const SparseXorVec<DemTarget> &expected) {
    if (unshifted.size() != expected.size()) {
        return false;
    }
    for (size_t k = 0; k < unshifted.size(); k++) {
        DemTarget a = unshifted.sorted_items[k];
        DemTarget e = expected.sorted_items[k];
        a.shift_if_detector_id(shift);
        if (a != e) {
            return false;
        }
    }
    return true;
}

void ErrorAnalyzer::run_loop(const Circuit &loop, uint64_t iterations) {
    if (!fold_loops) {
        // If loop folding is disabled, just manually run each iteration.
        for (size_t k = 0; k < iterations; k++) {
            run_circuit(loop);
        }
        return;
    }

    uint64_t num_loop_detectors = loop.count_detectors();
    uint64_t hare_iter = 0;
    uint64_t tortoise_iter = 0;
    ErrorAnalyzer hare(total_detectors - used_detectors, xs.size(), false, true, allow_gauge_detectors);
    hare.xs = xs;
    hare.zs = zs;
    hare.measurement_to_detectors = measurement_to_detectors;
    hare.scheduled_measurement_time = scheduled_measurement_time;
    hare.accumulate_errors = false;

    auto hare_is_colliding_with_tortoise = [&]() -> bool {
        // When comparing different loop iterations, shift detector ids to account for
        // detectors being introduced during each iteration.
        int64_t dt = -(int64_t)((hare_iter - tortoise_iter) * num_loop_detectors);
        for (size_t k = 0; k < hare.xs.size(); k++) {
            if (!shifted_equals(dt, xs[k], hare.xs[k])) {
                return false;
            }
            if (!shifted_equals(dt, zs[k], hare.zs[k])) {
                return false;
            }
        }
        return true;
    };

    // Perform tortoise-and-hare cycle finding.
    while (hare_iter < iterations) {
        hare.run_circuit(loop);
        hare_iter++;
        if (hare_is_colliding_with_tortoise()) {
            break;
        }

        if (hare_iter % 2 == 0) {
            run_circuit(loop);
            tortoise_iter++;
            if (hare_is_colliding_with_tortoise()) {
                break;
            }
        }
    }

    if (hare_iter < iterations) {
        // Don't bother folding a single iteration into a repeated block.
        uint64_t period = hare_iter - tortoise_iter;
        uint64_t period_iterations = (iterations - tortoise_iter) / period;
        if (period_iterations > 1) {
            // Stash error model build up so far.
            flush();
            DetectorErrorModel tmp = std::move(flushed_reversed_model);

            // Rewrite state to look like it would if loop had executed all but the last iteration.
            uint64_t shift_per_iteration = period * num_loop_detectors;
            int64_t detector_shift = (int64_t)((period_iterations - 1) * shift_per_iteration);
            shift_active_detector_ids(-detector_shift);
            used_detectors += detector_shift;
            tortoise_iter += period_iterations * period;

            // Compute the loop's error model.
            for (size_t k = 0; k < period; k++) {
                run_circuit(loop);
            }
            flush();
            DetectorErrorModel body = std::move(flushed_reversed_model);

            // The loop ends (well, starts because everything is reversed) by shifting the detector coordinates.
            uint64_t lower_level_shifts = body.total_detector_shift();
            DemTarget remaining_shift = {shift_per_iteration - lower_level_shifts};
            if (remaining_shift.data > 0) {
                if (body.instructions.empty() || body.instructions.front().type != DEM_SHIFT_DETECTORS) {
                    auto shift_targets = body.target_buf.take_copy({&remaining_shift, &remaining_shift + 1});
                    body.instructions.insert(
                        body.instructions.begin(), DemInstruction{{}, shift_targets, DEM_SHIFT_DETECTORS});
                } else {
                    remaining_shift.data += body.instructions[0].target_data[0].data;
                    auto shift_targets = body.target_buf.take_copy({&remaining_shift, &remaining_shift + 1});
                    body.instructions[0].target_data = shift_targets;
                }
            }

            // Append the loop to the growing error model and put the error model back in its proper place.
            tmp.append_repeat_block(period_iterations, std::move(body));
            flushed_reversed_model = std::move(tmp);
        }
    }

    // Perform remaining loop iterations leftover after jumping forward by multiples of the recurrence period.
    while (tortoise_iter < iterations) {
        run_circuit(loop);
        tortoise_iter++;
    }
}

void ErrorAnalyzer::shift_active_detector_ids(int64_t shift) {
    for (auto &e : measurement_to_detectors) {
        for (auto &v : e.second) {
            v.shift_if_detector_id(shift);
        }
    }
    for (auto &x : xs) {
        for (auto &v : x) {
            v.shift_if_detector_id(shift);
        }
    }
    for (auto &x : zs) {
        for (auto &v : x) {
            v.shift_if_detector_id(shift);
        }
    }
}

void ErrorAnalyzer::SHIFT_COORDS(const OperationData &dat) {
    flushed_reversed_model.append_shift_detectors_instruction(dat.args, 0);
}
