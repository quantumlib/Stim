/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stim/circuit/gate_decomposition.h"

using namespace stim;

void stim::decompose_mpp_operation(
    const CircuitInstruction &mpp_op,
    size_t num_qubits,
    const std::function<void(
        const CircuitInstruction &h_xz,
        const CircuitInstruction &h_yz,
        const CircuitInstruction &cnot,
        const CircuitInstruction &meas)> &callback) {
    simd_bits<64> used(num_qubits);
    simd_bits<64> inner_used(num_qubits);
    std::vector<GateTarget> h_xz;
    std::vector<GateTarget> h_yz;
    std::vector<GateTarget> cnot;
    std::vector<GateTarget> meas;

    size_t start = 0;
    while (start < mpp_op.targets.size()) {
        size_t end = start + 1;
        while (end < mpp_op.targets.size() && mpp_op.targets[end].is_combiner()) {
            end += 2;
        }

        // Determine which qubits are being touched by the next group.
        inner_used.clear();
        for (size_t i = start; i < end; i += 2) {
            auto t = mpp_op.targets[i];
            if (inner_used[t.qubit_value()]) {
                throw std::invalid_argument(
                    "A pauli product specified the same qubit twice.\n"
                    "The operation: " +
                    mpp_op.str());
            }
            inner_used[t.qubit_value()] = true;
        }

        // If there's overlap with previous groups, the previous groups have to be flushed first.
        if (inner_used.intersects(used)) {
            callback(
                CircuitInstruction{GateType::H, {}, h_xz},
                CircuitInstruction{GateType::H_YZ, {}, h_yz},
                CircuitInstruction{GateType::CX, {}, cnot},
                CircuitInstruction{GateType::M, mpp_op.args, meas});
            h_xz.clear();
            h_yz.clear();
            cnot.clear();
            meas.clear();
            used.clear();
        }
        used |= inner_used;

        // Append operations that are equivalent to the desired measurement.
        for (size_t i = start; i < end; i += 2) {
            auto t = mpp_op.targets[i];
            auto q = t.qubit_value();
            if (t.data & TARGET_PAULI_X_BIT) {
                if (t.data & TARGET_PAULI_Z_BIT) {
                    h_yz.push_back({q});
                } else {
                    h_xz.push_back({q});
                }
            }
            if (i == start) {
                meas.push_back({q});
            } else {
                cnot.push_back({q});
                cnot.push_back({meas.back().qubit_value()});
            }
            meas.back().data ^= t.data & TARGET_INVERTED_BIT;
        }

        start = end;
    }

    // Flush remaining groups.
    callback(
        CircuitInstruction{GateType::H, {}, h_xz},
        CircuitInstruction{GateType::H_YZ, {}, h_yz},
        CircuitInstruction{GateType::CX, {}, cnot},
        CircuitInstruction{GateType::M, mpp_op.args, meas});
}

void stim::decompose_pair_instruction_into_segments_with_single_use_controls(
    const CircuitInstruction &inst, size_t num_qubits, const std::function<void(CircuitInstruction)> &callback) {
    simd_bits<64> used_as_control(std::max(num_qubits, size_t{1}));
    size_t done = 0;
    size_t k = 0;
    while (done < inst.targets.size()) {
        bool flush = true;
        size_t q0 = 0;
        if (k < inst.targets.size()) {
            q0 = inst.targets[k].qubit_value();
            size_t q1 = inst.targets[k + 1].qubit_value();
            flush = used_as_control[q0] || used_as_control[q1];
        }
        if (flush) {
            callback(CircuitInstruction{
                inst.gate_type,
                inst.args,
                inst.targets.sub(done, k),
            });
            used_as_control.clear();
            done = k;
        }
        used_as_control[q0] = true;
        k += 2;
    }
}
