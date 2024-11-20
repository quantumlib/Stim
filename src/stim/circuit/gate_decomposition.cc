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

#include <span>

#include "stim/stabilizers/pauli_string.h"

using namespace stim;

struct ConjugateBySelfInverse {
    CircuitInstruction inst;
    const std::function<void(const CircuitInstruction &inst)> &do_instruction_callback;
    ConjugateBySelfInverse(
        CircuitInstruction inst, const std::function<void(const CircuitInstruction &inst)> &do_instruction_callback)
        : inst(inst), do_instruction_callback(do_instruction_callback) {
        if (!inst.targets.empty()) {
            do_instruction_callback(inst);
        }
    }
    ~ConjugateBySelfInverse() {
        if (!inst.targets.empty()) {
            do_instruction_callback(inst);
        }
    }
};

bool stim::accumulate_next_obs_terms_to_pauli_string_helper(
    CircuitInstruction instruction,
    size_t *start,
    PauliString<64> *obs,
    std::vector<GateTarget> *bits,
    bool allow_imaginary) {
    if (*start >= instruction.targets.size()) {
        return false;
    }

    if (bits != nullptr) {
        bits->clear();
    }
    obs->xs.clear();
    obs->zs.clear();
    obs->sign = false;
    bool imag = false;

    // Find end of current product.
    size_t end = *start + 1;
    while (end < instruction.targets.size() && instruction.targets[end].is_combiner()) {
        end += 2;
    }

    // Accumulate terms.
    for (size_t k = *start; k < end; k += 2) {
        GateTarget t = instruction.targets[k];

        if (t.is_pauli_target()) {
            obs->left_mul_pauli(t, &imag);
        } else if (t.is_classical_bit_target() && bits != nullptr) {
            bits->push_back(t);
        } else {
            throw std::invalid_argument("Found an unsupported target `" + t.str() + "` in " + instruction.str());
        }
    }

    if (imag && !allow_imaginary) {
        throw std::invalid_argument(
            "Acted on an anti-Hermitian operator (e.g. X0*Z0 instead of Y0) in " + instruction.str());
    }

    *start = end;
    return true;
}

void stim::decompose_mpp_operation(
    const CircuitInstruction &mpp_op,
    size_t num_qubits,
    const std::function<void(const CircuitInstruction &inst)> &do_instruction_callback) {
    PauliString<64> current(num_qubits);
    simd_bits<64> merged(num_qubits);
    std::vector<GateTarget> h_xz;
    std::vector<GateTarget> h_yz;
    std::vector<GateTarget> cnot;
    std::vector<GateTarget> meas;

    auto flush = [&]() {
        if (meas.empty()) {
            return;
        }
        {
            ConjugateBySelfInverse c1(CircuitInstruction(GateType::H, {}, h_xz, mpp_op.tag), do_instruction_callback);
            ConjugateBySelfInverse c2(
                CircuitInstruction(GateType::H_YZ, {}, h_yz, mpp_op.tag), do_instruction_callback);
            ConjugateBySelfInverse c3(CircuitInstruction(GateType::CX, {}, cnot, mpp_op.tag), do_instruction_callback);
            do_instruction_callback(CircuitInstruction(GateType::M, mpp_op.args, meas, mpp_op.tag));
        }
        h_xz.clear();
        h_yz.clear();
        cnot.clear();
        meas.clear();
        merged.clear();
    };

    size_t start = 0;
    while (accumulate_next_obs_terms_to_pauli_string_helper(mpp_op, &start, &current, nullptr)) {
        // Products equal to +-I become MPAD instructions.
        if (current.ref().has_no_pauli_terms()) {
            flush();
            GateTarget t = GateTarget::qubit((uint32_t)current.sign);
            do_instruction_callback(CircuitInstruction{GateType::MPAD, mpp_op.args, &t, mpp_op.tag});
            continue;
        }

        // If there's overlap with previous groups, the previous groups need to be flushed.
        if (current.xs.intersects(merged) || current.zs.intersects(merged)) {
            flush();
        }
        merged |= current.xs;
        merged |= current.zs;

        // Buffer operations to perform the desired measurement.
        bool first = true;
        current.ref().for_each_active_pauli([&](uint32_t q) {
            bool x = current.xs[q];
            bool z = current.zs[q];
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
        });
        assert(!first);
    }

    // Flush remaining groups.
    flush();
}

static void decompose_spp_or_spp_dag_operation_helper(
    PauliStringRef<64> observable,
    std::span<const GateTarget> classical_bits,
    bool invert_sign,
    const std::function<void(const CircuitInstruction &inst)> &do_instruction_callback,
    std::vector<GateTarget> *h_xz_buf,
    std::vector<GateTarget> *h_yz_buf,
    std::vector<GateTarget> *cnot_buf,
    std::string_view tag) {
    h_xz_buf->clear();
    h_yz_buf->clear();
    cnot_buf->clear();

    // Assemble quantum terms from the observable.
    uint64_t focus_qubit = UINT64_MAX;
    observable.for_each_active_pauli([&](uint32_t q) {
        bool x = observable.xs[q];
        bool z = observable.zs[q];
        // Include single qubit gates transforming the Pauli into a Z.
        if (x) {
            if (z) {
                h_yz_buf->push_back({q});
            } else {
                h_xz_buf->push_back({q});
            }
        }
        // Include CNOT gates folding onto a single measured qubit.
        if (focus_qubit == UINT64_MAX) {
            focus_qubit = q;
        } else {
            cnot_buf->push_back({q});
            cnot_buf->push_back({(uint32_t)focus_qubit});
        }
    });

    // Products need a quantum part to have an observable effect.
    if (focus_qubit == UINT64_MAX) {
        return;
    }

    for (const auto &t : classical_bits) {
        cnot_buf->push_back({t});
        cnot_buf->push_back({(uint32_t)focus_qubit});
    }

    GateTarget t = GateTarget::qubit(focus_qubit);
    bool sign = invert_sign ^ observable.sign;
    GateType g = sign ? GateType::S_DAG : GateType::S;
    {
        ConjugateBySelfInverse c1(CircuitInstruction(GateType::H, {}, *h_xz_buf, tag), do_instruction_callback);
        ConjugateBySelfInverse c2(CircuitInstruction(GateType::H_YZ, {}, *h_yz_buf, tag), do_instruction_callback);
        ConjugateBySelfInverse c3(CircuitInstruction(GateType::CX, {}, *cnot_buf, tag), do_instruction_callback);
        do_instruction_callback(CircuitInstruction(g, {}, &t, tag));
    }
}

void stim::decompose_spp_or_spp_dag_operation(
    const CircuitInstruction &spp_op,
    size_t num_qubits,
    bool invert_sign,
    const std::function<void(const CircuitInstruction &inst)> &do_instruction_callback) {
    PauliString<64> obs(num_qubits);
    std::vector<GateTarget> h_xz_buf;
    std::vector<GateTarget> h_yz_buf;
    std::vector<GateTarget> cnot_buf;
    std::vector<GateTarget> bits;

    if (spp_op.gate_type == GateType::SPP) {
        // No sign inversion needed.
    } else if (spp_op.gate_type == GateType::SPP_DAG) {
        invert_sign ^= true;
    } else {
        throw std::invalid_argument("Not an SPP or SPP_DAG instruction: " + spp_op.str());
    }

    size_t start = 0;
    while (accumulate_next_obs_terms_to_pauli_string_helper(spp_op, &start, &obs, &bits)) {
        decompose_spp_or_spp_dag_operation_helper(
            obs, bits, invert_sign, do_instruction_callback, &h_xz_buf, &h_yz_buf, &cnot_buf, spp_op.tag);
    }
}

void stim::decompose_pair_instruction_into_disjoint_segments(
    const CircuitInstruction &inst, size_t num_qubits, const std::function<void(CircuitInstruction)> &callback) {
    simd_bits<64> used_as_control(num_qubits);
    size_t num_flushed = 0;
    size_t cur_index = 0;
    auto flush = [&]() {
        callback(
            CircuitInstruction{
                inst.gate_type,
                inst.args,
                inst.targets.sub(num_flushed, cur_index),
                inst.tag,
            });
        used_as_control.clear();
        num_flushed = cur_index;
    };
    while (cur_index < inst.targets.size()) {
        size_t q0 = inst.targets[cur_index].qubit_value();
        size_t q1 = inst.targets[cur_index + 1].qubit_value();
        if (used_as_control[q0] || used_as_control[q1]) {
            flush();
        }
        used_as_control[q0] = true;
        used_as_control[q1] = true;
        cur_index += 2;
    }
    if (num_flushed < inst.targets.size()) {
        flush();
    }
}

void stim::for_each_disjoint_target_segment_in_instruction_reversed(
    const CircuitInstruction &inst,
    simd_bits_range_ref<64> workspace,
    const std::function<void(CircuitInstruction)> &callback) {
    workspace.clear();
    size_t cur_end = inst.targets.size();
    size_t cur_start = inst.targets.size();
    auto flush = [&]() {
        callback(CircuitInstruction(inst.gate_type, inst.args, inst.targets.sub(cur_start, cur_end), inst.tag));
        workspace.clear();
        cur_end = cur_start;
    };
    while (cur_start > 0) {
        auto t = inst.targets[cur_start - 1];
        if (t.has_qubit_value()) {
            if (workspace[t.qubit_value()]) {
                flush();
            }
            workspace[t.qubit_value()] = true;
        }
        cur_start--;
    }
    if (cur_end > 0) {
        flush();
    }
}

void stim::for_each_combined_targets_group(
    const CircuitInstruction &inst, const std::function<void(CircuitInstruction)> &callback) {
    if (inst.targets.empty()) {
        return;
    }
    size_t start = 0;
    size_t next_start = 1;
    while (true) {
        if (next_start >= inst.targets.size() || !inst.targets[next_start].is_combiner()) {
            callback(CircuitInstruction(inst.gate_type, inst.args, inst.targets.sub(start, next_start), inst.tag));
            start = next_start;
            next_start = start + 1;
            if (next_start > inst.targets.size()) {
                return;
            }
        } else {
            next_start += 2;
        }
    }
}
