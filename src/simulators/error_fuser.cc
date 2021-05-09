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

#include "../circuit/circuit.h"

using namespace stim_internal;

void ErrorFuser::RX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        if (!zs[q].empty()) {
            throw std::invalid_argument("A detector or observable anti-commuted with a reset.");
        }
        xs[q].clear();
    }
}

void ErrorFuser::RY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        if (xs[q] != zs[q]) {
            throw std::invalid_argument("A detector or observable anti-commuted with a reset.");
        }
        xs[q].clear();
        zs[q].clear();
    }
}

void ErrorFuser::RZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        if (!xs[q].empty()) {
            throw std::invalid_argument("A detector or observable anti-commuted with a reset.");
        }
        zs[q].clear();
    }
}

void ErrorFuser::MX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<uint32_t> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        xs[q].xor_sorted_items(d);
        if (!zs[q].empty()) {
            throw std::invalid_argument("A detector or observable anti-commuted with a measurement.");
        }
    }
}

void ErrorFuser::MY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<uint32_t> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        xs[q].xor_sorted_items(d);
        zs[q].xor_sorted_items(d);
        if (xs[q] != zs[q]) {
            throw std::invalid_argument("A detector or observable anti-commuted with a measurement.");
        }
    }
}

void ErrorFuser::MZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;

        std::vector<uint32_t> &d = measurement_to_detectors[scheduled_measurement_time];
        std::sort(d.begin(), d.end());
        zs[q].xor_sorted_items(d);
        if (!xs[q].empty()) {
            throw std::invalid_argument("A detector or observable anti-commuted with a measurement.");
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

void ErrorFuser::feedback(uint32_t record_control, size_t target, bool x, bool z) {
    uint32_t time = scheduled_measurement_time + (record_control & ~TARGET_RECORD_BIT);
    std::vector<uint32_t> &dst = measurement_to_detectors[time];

    // Temporarily move map's vector data into a SparseXorVec for manipulation.
    std::sort(dst.begin(), dst.end());
    SparseXorVec<uint32_t> tmp(std::move(dst));

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
    uint32_t id = UINT32_MAX - num_found_detectors;
    num_found_detectors++;
    for (auto t : dat.targets) {
        auto delay = t & TARGET_VALUE_MASK;
        measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
    }
}

void ErrorFuser::OBSERVABLE_INCLUDE(const OperationData &dat) {
    uint32_t id = (int)dat.arg;
    num_found_observables = std::max(num_found_observables, id + 1);
    for (auto t : dat.targets) {
        auto delay = t & TARGET_VALUE_MASK;
        measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
    }
}

ErrorFuser::ErrorFuser(size_t num_qubits) : xs(num_qubits), zs(num_qubits) {
}

void ErrorFuser::run_circuit(const Circuit &circuit) {
    circuit.for_each_operation_reverse([&](const Operation &op) {
        (this->*op.gate->reverse_error_fuser_function)(op.target_data);
    });
}

void ErrorFuser::X_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        add_error(dat.arg, zs[q]);
    }
}

void ErrorFuser::Y_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        add_xored_error(dat.arg, xs[q], zs[q]);
    }
}

void ErrorFuser::Z_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        add_error(dat.arg, xs[q]);
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
    for (auto qp : dat.targets) {
        auto q = qp & TARGET_VALUE_MASK;
        if (qp & TARGET_PAULI_Z_BIT) {
            inplace_xor_tail(jag_flip_data, xs[q]);
        }
        if (qp & TARGET_PAULI_X_BIT) {
            inplace_xor_tail(jag_flip_data, zs[q]);
        }
    }
    add_error_in_sorted_jagged_tail(dat.arg);
}

void ErrorFuser::DEPOLARIZE1(const OperationData &dat) {
    if (dat.arg >= 3.0 / 4.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 3/4 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * sqrt(1 - (4 * dat.arg) / 3);
    for (auto q : dat.targets) {
        add_error(p, xs[q]);
        add_error(p, zs[q]);
        add_xored_error(p, xs[q], zs[q]);
    }
}

void ErrorFuser::DEPOLARIZE2(const OperationData &dat) {
    if (dat.arg >= 15.0 / 16.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 15/16 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * pow(1 - (16 * dat.arg) / 15, 0.125);
    for (size_t i = 0; i < dat.targets.size(); i += 2) {
        auto a = dat.targets[i];
        auto b = dat.targets[i + 1];

        auto &x1 = xs[a];
        auto &x2 = xs[b];
        auto &z1 = zs[a];
        auto &z2 = zs[b];
        auto y1 = x1 ^ z1;
        auto y2 = x2 ^ z2;

        // Isolated errors.
        add_error(p, x1);
        add_error(p, y1);
        add_error(p, z1);
        add_error(p, x2);
        add_error(p, y2);
        add_error(p, z2);

        // Paired errors.
        add_xored_error(p, x1, x2);
        add_xored_error(p, y1, x2);
        add_xored_error(p, z1, x2);
        add_xored_error(p, x1, y2);
        add_xored_error(p, y1, y2);
        add_xored_error(p, z1, y2);
        add_xored_error(p, x1, z2);
        add_xored_error(p, y1, z2);
        add_xored_error(p, z1, z2);
    }
}

void ErrorFuser::ELSE_CORRELATED_ERROR(const OperationData &dat) {
    throw std::out_of_range(
        "ELSE_CORRELATED_ERROR operations not supported when converting to a detector hyper graph.");
}

void ErrorFuser::convert_circuit_out(const Circuit &circuit, FILE *out) {
    ErrorFuser fuser(circuit.count_qubits());
    fuser.run_circuit(circuit);
    std::stringstream ss;

    uint32_t detector_id_root = UINT32_MAX - fuser.num_found_detectors + 1;
    for (const auto &kv : fuser.error_class_probabilities) {
        ss.str("");
        ss << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << kv.second;
        fprintf(out, "error(%s)", ss.str().data());
        for (auto e : kv.first) {
            if (e > (UINT32_MAX >> 2)) {
                fprintf(out, " D%lld", (long long)(e - detector_id_root));
            } else {
                fprintf(out, " L%lld", (long long)e);
            }
        }
        fprintf(out, "\n");
    }
}

void ErrorFuser::add_error(double probability, const SparseXorVec<uint32_t> &flipped) {
    jag_flip_data.append_tail(flipped.range());
    add_error_in_sorted_jagged_tail(probability);
}

void ErrorFuser::add_xored_error(
    double probability, const SparseXorVec<uint32_t> &flipped1, const SparseXorVec<uint32_t> &flipped2) {
    jag_flip_data.ensure_available(flipped1.size() + flipped2.size());
    jag_flip_data.tail.ptr_end =
        xor_merge_sort<uint32_t>(flipped1.range(), flipped2.range(), jag_flip_data.tail.ptr_end);
    add_error_in_sorted_jagged_tail(probability);
}

void ErrorFuser::add_error_in_sorted_jagged_tail(double probability) {
    auto flipped = jag_flip_data.tail;
    if (flipped.size()) {
        if (error_class_probabilities.find(flipped) != error_class_probabilities.end()) {
            auto &p = error_class_probabilities[flipped];
            p = p * (1 - probability) + (1 - p) * probability;
            jag_flip_data.discard_tail();
        } else {
            error_class_probabilities[flipped] = probability;
            jag_flip_data.commit_tail();
        }
    }
}
