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

#ifndef _STIM_GATE_DECOMPOSITION_H
#define _STIM_GATE_DECOMPOSITION_H

#include "stim/circuit/gate_data.h"
#include "stim/circuit/gate_target.h"
#include "stim/circuit/circuit_instruction.h"
#include "stim/mem/simd_bits.h"


namespace stim {

template <size_t W>
void decompose_mpp_operation(
    const CircuitInstruction &mpp_op,
    size_t num_qubits,
    const std::function<void(
        const CircuitInstruction &h_xz,
        const CircuitInstruction &h_yz,
        const CircuitInstruction &cnot,
        const CircuitInstruction &meas)> &callback) {
    simd_bits<W> used(num_qubits);
    simd_bits<W> inner_used(num_qubits);
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

}  // namespace stim

#endif
