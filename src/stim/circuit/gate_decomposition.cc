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

#include "stim/stabilizers/pauli_string.h"

using namespace stim;

void stim::decompose_mpp_operation(
    const CircuitInstruction &mpp_op,
    size_t num_qubits,
    const std::function<void(const CircuitInstruction &inst)> &callback) {
    PauliString<64> current(num_qubits);
    simd_bits<64> merged(num_qubits);
    std::vector<GateTarget> h_xz;
    std::vector<GateTarget> h_yz;
    std::vector<GateTarget> cnot;
    std::vector<GateTarget> meas;

    auto flush = [&](){
        if (meas.empty()) {
            return;
        }
        if (!h_xz.empty()) {
            callback(CircuitInstruction{GateType::H, {}, h_xz});
        }
        if (!h_yz.empty()) {
            callback(CircuitInstruction{GateType::H_YZ, {}, h_yz});
        }
        if (!cnot.empty()) {
            callback(CircuitInstruction{GateType::CX, {}, cnot});
        }
        try {
            callback(CircuitInstruction{GateType::M, mpp_op.args, meas});
        } catch (const std::exception &ex) {
            // If the measurement was somehow illegal, unwind the conjugations to return to the original state.
            if (!cnot.empty()) {
                callback(CircuitInstruction{GateType::CX, {}, cnot});
            }
            if (!h_yz.empty()) {
                callback(CircuitInstruction{GateType::H_YZ, {}, h_yz});
            }
            if (!h_xz.empty()) {
                callback(CircuitInstruction{GateType::H, {}, h_xz});
            }
            throw;
        }
        if (!cnot.empty()) {
            callback(CircuitInstruction{GateType::CX, {}, cnot});
        }
        if (!h_yz.empty()) {
            callback(CircuitInstruction{GateType::H_YZ, {}, h_yz});
        }
        if (!h_xz.empty()) {
            callback(CircuitInstruction{GateType::H, {}, h_xz});
        }
        h_xz.clear();
        h_yz.clear();
        cnot.clear();
        meas.clear();
        merged.clear();
    };

    size_t start = 0;
    while (start < mpp_op.targets.size()) {
        size_t end = start + 1;
        while (end < mpp_op.targets.size() && mpp_op.targets[end].is_combiner()) {
            end += 2;
        }

        // Determine which qubits are being touched by the next group.
        current.xs.clear();
        current.zs.clear();
        current.sign = false;
        bool imag = 0;
        for (size_t i = start; i < end; i += 2) {
            current.safe_accumulate_pauli_term(mpp_op.targets[i], &imag);
        }
        if (imag) {
            throw std::invalid_argument(
                "Asked to measure an anti-Hermitian operator (e.g. X0*Z0 instead of Y0) in " + mpp_op.str());
        }

        // Products equal to +-I become MPAD instructions.
        if (current.ref().has_no_pauli_terms()) {
            flush();
            GateTarget t = GateTarget::qubit((uint32_t)current.sign);
            callback(CircuitInstruction{GateType::MPAD, mpp_op.args, &t});
            start = end;
            continue;
        }

        // If there's overlap with previous groups, the previous groups need to be flushed.
        if (current.xs.intersects(merged) || current.zs.intersects(merged)) {
            flush();
        }
        merged |= current.xs;
        merged |= current.zs;

        // Buffer operations to perform the desired measurement.
        size_t n64 = current.xs.num_u64_padded();
        bool first = true;
        for (size_t i = 0; i < n64; i++) {
            uint64_t x64 = current.xs.u64[i];
            uint64_t z64 = current.zs.u64[i];
            uint64_t u64 = x64 | z64;
            if (u64) {
                for (size_t j = 0; j < 64; j++) {
                    bool x = (x64 >> j) & 1;
                    bool z = (z64 >> j) & 1;
                    if (x | z) {
                        uint32_t q = (uint32_t)(i * 64 + j);
                        // Include single qubit gates transforming the Pauli into a Z.
                        if (x) {
                            if (z) {
                                h_yz.push_back({q});
                            } else {
                                h_xz.push_back({q});
                            }
                        }
                        // Include CNOT gates folding onto a single measured qubit.
                        if (first) {
                            meas.push_back(GateTarget::qubit(q, current.sign));
                            first = false;
                        } else {
                            cnot.push_back({q});
                            cnot.push_back({meas.back().qubit_value()});
                        }
                    }
                }
            }
            assert(!first);
        }

        start = end;
    }

    // Flush remaining groups.
    flush();
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
