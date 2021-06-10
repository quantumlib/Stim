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

#include "error_fuser.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <queue>
#include <sstream>

#include "../dem/detector_error_model.h"

using namespace stim_internal;

bool stim_internal::is_encoded_detector_id(uint64_t id) {
    return id < FIRST_OBSERVABLE_ID;
}

bool stim_internal::is_encoded_observable_id(uint64_t id) {
    return id >= FIRST_OBSERVABLE_ID && id != COMPOSITE_ERROR_SYGIL;
}

void ErrorFuser::remove_gauge(ConstPointerRange<uint64_t> sorted) {
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

void ErrorFuser::RX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        if (!zs[q].empty()) {
            if (validate_detectors) {
                throw std::invalid_argument("A detector or observable anti-commuted with a reset.");
            }
            remove_gauge(add_error(0.5, zs[q].range()));
        }
        xs[q].clear();
    }
}

void ErrorFuser::RY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        if (xs[q] != zs[q]) {
            if (validate_detectors) {
                throw std::invalid_argument("A detector or observable anti-commuted with a reset.");
            }
            remove_gauge(add_xored_error(0.5, xs[q].range(), zs[q].range()));
        }
        xs[q].clear();
        zs[q].clear();
    }
}

void ErrorFuser::RZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        if (!xs[q].empty()) {
            if (validate_detectors) {
                throw std::invalid_argument("A detector or observable anti-commuted with a reset.");
            }
            remove_gauge(add_error(0.5, xs[q].range()));
        }
        zs[q].clear();
    }
}

void ErrorFuser::MX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<uint64_t> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        xs[q].xor_sorted_items(d);
        if (!zs[q].empty()) {
            if (validate_detectors) {
                throw std::invalid_argument("A detector or observable anti-commuted with a measurement.");
            }
            remove_gauge(add_error(0.5, zs[q].range()));
        }
    }
}

void ErrorFuser::MY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<uint64_t> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        xs[q].xor_sorted_items(d);
        zs[q].xor_sorted_items(d);
        if (xs[q] != zs[q]) {
            if (validate_detectors) {
                throw std::invalid_argument("A detector or observable anti-commuted with a measurement.");
            }
            remove_gauge(add_xored_error(0.5, xs[q].range(), zs[q].range()));
        }
    }
}

void ErrorFuser::MZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<uint64_t> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        zs[q].xor_sorted_items(d);
        if (!xs[q].empty()) {
            if (validate_detectors) {
                throw std::invalid_argument("A detector or observable anti-commuted with a measurement.");
            }
            remove_gauge(add_error(0.5, xs[q].range()));
        }
    }
}

void ErrorFuser::MRX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{0, {&q, &q + 1}};
        RX(d);
        MX(d);
    }
}

void ErrorFuser::MRY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{0, {&q, &q + 1}};
        RY(d);
        MY(d);
    }
}

void ErrorFuser::MRZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{0, {&q, &q + 1}};
        RZ(d);
        MZ(d);
    }
}

void ErrorFuser::H_XZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        std::swap(xs[q], zs[q]);
    }
}

void ErrorFuser::H_XY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        zs[q] ^= xs[q];
    }
}

void ErrorFuser::H_YZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        xs[q] ^= zs[q];
    }
}

void ErrorFuser::C_XYZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

void ErrorFuser::C_ZYX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

void ErrorFuser::XCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k];
        auto q2 = dat.targets[k + 1];
        xs[q1] ^= zs[q2];
        xs[q2] ^= zs[q1];
    }
}

void ErrorFuser::XCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k];
        auto ty = dat.targets[k + 1];
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void ErrorFuser::YCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k + 1];
        auto ty = dat.targets[k];
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void ErrorFuser::ZCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        single_cy(c, t);
    }
}

void ErrorFuser::YCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        single_cy(c, t);
    }
}

void ErrorFuser::YCY(const OperationData &dat) {
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

void ErrorFuser::ZCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        single_cx(c, t);
    }
}

void ErrorFuser::SQRT_XX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        xs[a] ^= zs[a];
        xs[a] ^= zs[b];
        xs[b] ^= zs[a];
        xs[b] ^= zs[b];
    }
}

void ErrorFuser::SQRT_YY(const OperationData &dat) {
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

void ErrorFuser::SQRT_ZZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
    }
}

void ErrorFuser::feedback(uint32_t record_control, size_t target, bool x, bool z) {
    uint64_t time = scheduled_measurement_time + (record_control & ~TARGET_RECORD_BIT);
    std::vector<uint64_t> &dst = measurement_to_detectors[time];

    // Temporarily move map's vector data into a SparseXorVec for manipulation.
    std::sort(dst.begin(), dst.end());
    SparseXorVec<uint64_t> tmp(std::move(dst));

    if (x) {
        tmp ^= xs[target];
    }
    if (z) {
        tmp ^= zs[target];
    }

    // Move data back into the map.
    dst = std::move(tmp.sorted_items);
}

void ErrorFuser::single_cx(uint32_t c, uint32_t t) {
    if (!((c | t) & TARGET_RECORD_BIT)) {
        zs[c] ^= zs[t];
        xs[t] ^= xs[c];
    } else if (t & TARGET_RECORD_BIT) {
        throw std::out_of_range("Measurement record editing is not supported.");
    } else {
        feedback(c, t, false, true);
    }
}

void ErrorFuser::single_cy(uint32_t c, uint32_t t) {
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

void ErrorFuser::single_cz(uint32_t c, uint32_t t) {
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

void ErrorFuser::XCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        single_cx(c, t);
    }
}

void ErrorFuser::ZCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k];
        auto q2 = dat.targets[k + 1];
        single_cz(q1, q2);
    }
}

void ErrorFuser::I(const OperationData &dat) {
}

void ErrorFuser::SWAP(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void ErrorFuser::ISWAP(const OperationData &dat) {
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

void ErrorFuser::DETECTOR(const OperationData &dat) {
    uint64_t id = LAST_DETECTOR_ID - num_found_detectors;
    num_found_detectors++;
    for (auto t : dat.targets) {
        auto delay = t & TARGET_VALUE_MASK;
        measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
    }
}

void ErrorFuser::OBSERVABLE_INCLUDE(const OperationData &dat) {
    uint64_t id = FIRST_OBSERVABLE_ID + (int)dat.arg;
    num_found_observables = std::max(num_found_observables, id + 1);
    for (auto t : dat.targets) {
        auto delay = t & TARGET_VALUE_MASK;
        measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
    }
}

ErrorFuser::ErrorFuser(size_t num_qubits, bool find_reducible_errors, bool fold_loops, bool validate_detectors)
    : xs(num_qubits),
      zs(num_qubits),
      find_reducible_errors(find_reducible_errors),
      fold_loops(fold_loops),
      validate_detectors(validate_detectors) {
}

void ErrorFuser::run_circuit(const Circuit &circuit) {
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
            (this->*op.gate->reverse_error_fuser_function)(op.target_data);
        }
    }
}

void ErrorFuser::post_check_initialization() {
    for (const auto &x : xs) {
        if (!x.empty()) {
            if (validate_detectors) {
                throw std::invalid_argument("A detector or observable anti-commuted with an initialization.");
            }
            add_error(0.5, x.range());
        }
    }
}

void ErrorFuser::X_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_error(dat.arg, zs[q].range());
    }
}

void ErrorFuser::Y_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_xored_error(dat.arg, xs[q].range(), zs[q].range());
    }
}

void ErrorFuser::Z_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_error(dat.arg, xs[q].range());
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

void ErrorFuser::CORRELATED_ERROR(const OperationData &dat) {
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
    add_error_in_sorted_jagged_tail(dat.arg);
}

void ErrorFuser::DEPOLARIZE1(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    if (dat.arg >= 3.0 / 4.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 3/4 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * sqrt(1 - (4 * dat.arg) / 3);
    for (auto q : dat.targets) {
        add_error_combinations<2>(
            p,
            {
                xs[q].range(),
                zs[q].range(),
            });
    }
}

void ErrorFuser::DEPOLARIZE2(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    if (dat.arg >= 15.0 / 16.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 15/16 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * pow(1 - (16 * dat.arg) / 15, 0.125);
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

void ErrorFuser::ELSE_CORRELATED_ERROR(const OperationData &dat) {
    throw std::out_of_range(
        "ELSE_CORRELATED_ERROR operations not supported when converting to a detector hyper graph.");
}

DetectorErrorModel ErrorFuser::circuit_to_detector_error_model(
    const Circuit &circuit, bool find_reducible_errors, bool fold_loops, bool validate_detectors) {
    ErrorFuser fuser(circuit.count_qubits(), find_reducible_errors, fold_loops, validate_detectors);
    fuser.run_circuit(circuit);
    fuser.post_check_initialization();
    fuser.flush();
    return fuser.flushed_to_detector_error_model();
}

DetectorErrorModel ErrorFuser::flushed_to_detector_error_model() const {
    uint64_t time_offset = 0;
    DetectorErrorModel model;
    for (auto e = flushed.crbegin(); e != flushed.crend(); e++) {
        e->append_to_detector_error_model(model, num_found_detectors, time_offset, true);
    }
    return model;
}

void ErrorFuser::flush() {
    for (auto kv = error_class_probabilities.crbegin(); kv != error_class_probabilities.crend(); kv++) {
        if (kv->first.empty() || kv->second == 0) {
            continue;
        }
        flushed.push_back(FusedError{kv->second, kv->first, 0, nullptr});
    }
    error_class_probabilities.clear();
}

ConstPointerRange<uint64_t> ErrorFuser::add_xored_error(
    double probability, ConstPointerRange<uint64_t> flipped1, ConstPointerRange<uint64_t> flipped2) {
    mono_buf.ensure_available(flipped1.size() + flipped2.size());
    mono_buf.tail.ptr_end = xor_merge_sort<uint64_t>(flipped1, flipped2, mono_buf.tail.ptr_end);
    return add_error_in_sorted_jagged_tail(probability);
}

ConstPointerRange<uint64_t> ErrorFuser::mono_dedupe_store_tail() {
    auto v = error_class_probabilities.find(mono_buf.tail);
    if (v != error_class_probabilities.end()) {
        mono_buf.discard_tail();
        return v->first;
    }
    auto result = mono_buf.commit_tail();
    error_class_probabilities.insert({result, 0});
    return result;
}

ConstPointerRange<uint64_t> ErrorFuser::mono_dedupe_store(ConstPointerRange<uint64_t> data) {
    auto v = error_class_probabilities.find(data);
    if (v != error_class_probabilities.end()) {
        return v->first;
    }
    mono_buf.append_tail(data);
    auto result = mono_buf.commit_tail();
    error_class_probabilities.insert({result, 0});
    return result;
}

ConstPointerRange<uint64_t> ErrorFuser::add_error(double probability, ConstPointerRange<uint64_t> flipped) {
    auto key = mono_dedupe_store(flipped);
    auto &old_p = error_class_probabilities[key];
    old_p = old_p * (1 - probability) + (1 - old_p) * probability;
    return key;
}

ConstPointerRange<uint64_t> ErrorFuser::add_error_in_sorted_jagged_tail(double probability) {
    auto key = mono_dedupe_store_tail();
    auto &old_p = error_class_probabilities[key];
    old_p = old_p * (1 - probability) + (1 - old_p) * probability;
    return key;
}

bool shifted_equals(int64_t shift, const SparseXorVec<uint64_t> &unshifted, const SparseXorVec<uint64_t> &expected) {
    if (unshifted.size() != expected.size()) {
        return false;
    }
    for (size_t k = 0; k < unshifted.size(); k++) {
        auto a = unshifted.sorted_items[k];
        auto e = expected.sorted_items[k];
        if (is_encoded_detector_id(a)) {
            a += shift;
        }
        if (a != e) {
            return false;
        }
    }
    return true;
}

void ErrorFuser::run_loop(const Circuit &loop, uint64_t iterations) {
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
    ErrorFuser hare(xs.size(), false, true, validate_detectors);
    hare.xs = xs;
    hare.zs = zs;
    hare.num_found_detectors = num_found_detectors;
    hare.num_found_observables = num_found_observables;
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
        uint64_t period = hare_iter - tortoise_iter;
        uint64_t period_iterations = (iterations - tortoise_iter) / period;
        // Don't bother folding a single iteration into a repeated block.
        if (period_iterations > 1) {
            // Put loop body at end of flushed errors list.
            flush();
            size_t loop_content_start = flushed.size();
            for (size_t k = 0; k < period; k++) {
                run_circuit(loop);
            }
            flush();

            // Move loop body errors from end of flushed vector into a block.
            std::unique_ptr<FusedErrorRepeatBlock> block(new FusedErrorRepeatBlock());
            block->repetitions = period_iterations;
            block->total_ticks_per_iteration_including_sub_loops = num_loop_detectors * period;
            block->errors.reserve(flushed.size() - loop_content_start);
            for (size_t k = loop_content_start; k < flushed.size(); k++) {
                block->errors.push_back(std::move(flushed[k]));
            }
            flushed.erase(flushed.begin() + loop_content_start, flushed.end());

            // Rewrite state to look like it would if loop had actually executed all iterations.
            int64_t skipped_detectors = num_loop_detectors * (period_iterations - 1) * period;
            block->skip(skipped_detectors);
            flushed.push_back(FusedError{0, {}, 0, std::move(block)});
            num_found_detectors += skipped_detectors;
            shift_active_detector_ids(-skipped_detectors);
            tortoise_iter += period_iterations * period;
        }
    }

    // Perform remaining loop iterations leftover after jumping forward by multiples of the recurrence period.
    while (tortoise_iter < iterations) {
        run_circuit(loop);
        tortoise_iter++;
    }
}

void ErrorFuser::shift_active_detector_ids(int64_t shift) {
    for (auto &e : measurement_to_detectors) {
        for (auto &v : e.second) {
            if (is_encoded_detector_id(v)) {
                v += shift;
            }
        }
    }
    for (auto &x : xs) {
        for (auto &v : x) {
            if (is_encoded_detector_id(v)) {
                v += shift;
            }
        }
    }
    for (auto &x : zs) {
        for (auto &v : x) {
            if (is_encoded_detector_id(v)) {
                v += shift;
            }
        }
    }
}

void FusedErrorRepeatBlock::append_to_detector_error_model(
    DetectorErrorModel &out, uint64_t num_found_detectors, uint64_t &tick_count) const {
    DetectorErrorModel body;
    for (auto e = errors.crbegin(); e != errors.crend(); e++) {
        e->append_to_detector_error_model(body, num_found_detectors, tick_count, false);
    }
    size_t outer_ticks = outer_ticks_per_iteration();
    if (outer_ticks) {
        tick_count += outer_ticks;
        body.append_tick(outer_ticks);
    }
    tick_count += total_ticks_per_iteration_including_sub_loops * (repetitions - 1);

    out.append_repeat_block(repetitions, std::move(body));
}

void FusedErrorRepeatBlock::skip(uint64_t skipped) {
    for (auto &e : errors) {
        e.skip(skipped);
    }
}
uint64_t FusedErrorRepeatBlock::outer_ticks_per_iteration() const {
    uint64_t result = total_ticks_per_iteration_including_sub_loops;
    for (const auto &e : errors) {
        if (e.block) {
            result -= e.block->total_ticks_per_iteration_including_sub_loops * e.block->repetitions;
        }
    }
    return result;
}

void FusedError::skip(uint64_t skipped) {
    if (block) {
        block->skip(skipped);
    } else {
        local_time_shift += skipped;
    }
}

void FusedError::append_to_detector_error_model(
    DetectorErrorModel &out, uint64_t num_found_detectors, uint64_t &tick_count, bool top_level) const {
    if (block) {
        block->append_to_detector_error_model(out, num_found_detectors, tick_count);
        return;
    }

    std::vector<DemRelativeSymptom> symptoms;
    bool is_reducible = false;
    for (size_t k = 0; k < flipped.size(); k++) {
        auto e = flipped[k];
        if (e == COMPOSITE_ERROR_SYGIL) {
            is_reducible = true;
            if (k != 0 && k != flipped.size() - 1) {
                symptoms.push_back(DemRelativeSymptom::separator());
            }
        } else if (is_encoded_detector_id(e)) {
            auto abs_id = e + num_found_detectors - LAST_DETECTOR_ID - 1;
            auto rel_id = abs_id - tick_count - local_time_shift;
            symptoms.push_back(DemRelativeSymptom::detector_id(
                DemRelValue::unspecified(),
                DemRelValue::unspecified(),
                top_level ? DemRelValue::absolute(abs_id) : DemRelValue::relative(rel_id)));
        } else {
            symptoms.push_back(DemRelativeSymptom::observable_id(e - FIRST_OBSERVABLE_ID));
        }
    }
    if (is_reducible) {
        out.append_reducible_error(probability, symptoms);
    } else {
        out.append_error(probability, symptoms);
    }
}
