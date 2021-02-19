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
#include <queue>
#include <sstream>

#include "../circuit/circuit.h"

void ErrorFuser::R(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        xs[q].vec.clear();
        zs[q].vec.clear();
    }
}

void ErrorFuser::M(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k] & TARGET_VALUE_MASK;
        scheduled_measurement_time++;
        auto view = jagged_detector_sets.inserted(measurement_to_detectors[scheduled_measurement_time]);
        std::sort(view.begin(), view.end());
        zs[q].inplace_xor_helper(view.begin(), view.size());
        jagged_detector_sets.vec.resize(view.offset);
        measurement_to_detectors.erase(scheduled_measurement_time);
    }
}

void ErrorFuser::MR(const OperationData &dat) {
    R(dat);
    M(dat);
}

void ErrorFuser::H_XZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        std::swap(xs[q].vec, zs[q].vec);
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
    auto &out_view = measurement_to_detectors[time];
    std::sort(out_view.begin(), out_view.end());
    VectorView<uint32_t> view{&out_view, 0, out_view.size()};
    if (x) {
        auto &x_vec = xs[target];
        vector_tail_view_xor_in_place(view, x_vec.begin(), x_vec.size());
    }
    if (z) {
        auto &z_vec = zs[target];
        vector_tail_view_xor_in_place(view, z_vec.begin(), z_vec.size());
    }
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
        std::swap(xs[a].vec, xs[b].vec);
        std::swap(zs[a].vec, zs[b].vec);
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
        std::swap(xs[a].vec, xs[b].vec);
        std::swap(zs[a].vec, zs[b].vec);
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
    if (prepend_observables) {
        for (auto t : dat.targets) {
            auto delay = t & TARGET_VALUE_MASK;
            measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
        }
    }
}

ErrorFuser::ErrorFuser(size_t num_qubits, bool prepend_observables)
    : prepend_observables(prepend_observables), xs(num_qubits), zs(num_qubits) {
}

void ErrorFuser::run_circuit(const Circuit &circuit) {
    for (size_t k = circuit.operations.size(); k-- > 0;) {
        const auto &op = circuit.operations[k];
        (this->*op.gate->hit_simulator_function)(op.target_data);
    }
    uint32_t detector_id_root = UINT32_MAX - num_found_detectors + 1;
    if (prepend_observables) {
        detector_id_root -= num_found_observables;
    }
    for (auto &t : jagged_detector_sets.vec) {
        if (t > (UINT32_MAX >> 2)) {
            t -= detector_id_root;
        }
        t |= TARGET_PAULI_X_BIT;
    }
}

void ErrorFuser::X_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        independent_error_1(dat.arg, zs[q]);
    }
}

void ErrorFuser::Y_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        independent_error_2(dat.arg, xs[q], zs[q]);
    }
}

void ErrorFuser::Z_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        independent_error_1(dat.arg, xs[q]);
    }
}

void ErrorFuser::CORRELATED_ERROR(const OperationData &dat) {
    VectorView<uint32_t> tail = jagged_detector_sets.tail_view(jagged_detector_sets.vec.size());
    for (auto qp : dat.targets) {
        auto q = qp & TARGET_VALUE_MASK;
        if (qp & TARGET_PAULI_Z_BIT) {
            vector_tail_view_xor_in_place(tail, xs[q].begin(), xs[q].size());
        }
        if (qp & TARGET_PAULI_X_BIT) {
            vector_tail_view_xor_in_place(tail, zs[q].begin(), zs[q].size());
        }
    }
    independent_error_placed_tail(dat.arg, tail);
}

void ErrorFuser::DEPOLARIZE1(const OperationData &dat) {
    if (dat.arg >= 3.0 / 4.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 3/4 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * sqrt(1 - (4 * dat.arg) / 3);
    for (auto q : dat.targets) {
        independent_error_1(p, xs[q]);
        independent_error_1(p, zs[q]);
        independent_error_2(p, xs[q], zs[q]);
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
        independent_error_1(p, x1);
        independent_error_1(p, y1);
        independent_error_1(p, z1);
        independent_error_1(p, x2);
        independent_error_1(p, y2);
        independent_error_1(p, z2);

        // Paired errors.
        independent_error_2(p, x1, x2);
        independent_error_2(p, y1, x2);
        independent_error_2(p, z1, x2);
        independent_error_2(p, x1, y2);
        independent_error_2(p, y1, y2);
        independent_error_2(p, z1, y2);
        independent_error_2(p, x1, z2);
        independent_error_2(p, y1, z2);
        independent_error_2(p, z1, z2);
    }
}

void ErrorFuser::ELSE_CORRELATED_ERROR(const OperationData &dat) {
    throw std::out_of_range(
        "ELSE_CORRELATED_ERROR operations not supported when converting to a detector hyper graph.");
}

Circuit ErrorFuser::convert_circuit(const Circuit &circuit, bool prepend_observables) {
    ErrorFuser fuser(circuit.num_qubits, prepend_observables);
    fuser.run_circuit(circuit);

    Circuit result;
    const auto &e_gate = GATE_DATA.at("CORRELATED_ERROR");
    for (const auto &kv : fuser.error_class_probabilities) {
        result.append_operation(e_gate, kv.first.begin(), kv.first.size(), kv.second);
    }

    size_t num_ids = fuser.num_found_detectors;
    if (prepend_observables) {
        num_ids += fuser.num_found_observables;
    }
    auto view = result.jagged_target_data.view(result.jagged_target_data.vec.size(), num_ids);
    for (size_t k = 0; k < num_ids; k++) {
        result.jagged_target_data.vec.push_back(k);
    }
    result.operations.push_back({&GATE_DATA.at("M"), {0.0, view}});
    result.update_metadata_for_manually_appended_operation();

    return result;
}

void ErrorFuser::convert_circuit_out(const Circuit &circuit, FILE *out, bool prepend_observables) {
    ErrorFuser fuser(circuit.num_qubits, prepend_observables);
    fuser.run_circuit(circuit);

    for (const auto &kv : fuser.error_class_probabilities) {
        fprintf(out, "E(%f)", kv.second);
        for (auto e : kv.first) {
            fprintf(out, " X%lld", (long long)e);
        }
        fprintf(out, "\n");
    }
    fprintf(out, "M");
    size_t num_ids = fuser.num_found_detectors;
    if (prepend_observables) {
        num_ids += fuser.num_found_observables;
    }
    for (size_t k = 0; k < num_ids; k++) {
        fprintf(out, " %lld", (long long)k);
    }
    fprintf(out, "\n");
}

void ErrorFuser::independent_error_1(double probability, const SparseXorVec<uint32_t> &d1) {
    independent_error_1(probability, d1.begin(), d1.size());
}

void ErrorFuser::independent_error_1(double probability, const uint32_t *begin, size_t size) {
    independent_error_placed_tail(probability, jagged_detector_sets.inserted(begin, size));
}

void ErrorFuser::independent_error_placed_tail(double probability, VectorView<uint32_t> detector_set) {
    if (detector_set.size()) {
        if (error_class_probabilities.find(detector_set) != error_class_probabilities.end()) {
            auto &p = error_class_probabilities[detector_set];
            p = p * (1 - probability) + (1 - p) * probability;
            jagged_detector_sets.vec.resize(jagged_detector_sets.vec.size() - detector_set.size());
        } else {
            error_class_probabilities[detector_set] = probability;
        }
    }
}

void ErrorFuser::independent_error_2(
    double probability, const SparseXorVec<uint32_t> &d1, const SparseXorVec<uint32_t> &d2) {
    auto view = jagged_detector_sets.tail_view(jagged_detector_sets.vec.size());
    xor_into_vector_tail_view(view, d1.begin(), d1.size(), d2.begin(), d2.size());
    independent_error_placed_tail(probability, view);
}
