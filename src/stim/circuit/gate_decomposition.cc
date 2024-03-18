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

template <bool use_x, bool use_z, typename CALLBACK>
static void for_each_active_qubit_in(PauliStringRef<64> obs, CALLBACK callback) {
    size_t n = obs.xs.num_u64_padded();
    for (size_t w = 0; w < n; w++) {
        uint64_t v = 0;
        if (use_x) {
            v |= obs.xs.u64[w];
        }
        if (use_z) {
            v |= obs.zs.u64[w];
        }
        while (v) {
            size_t j = std::countr_zero(v);
            v &= ~(uint64_t{1} << j);
            bool b = false;
            uint32_t q = (uint32_t)(w * 64 + j);
            if (use_x) {
                b |= obs.xs[q];
            }
            if (use_z) {
                b |= obs.zs[q];
            }
            if (b) {
                callback(w * 64 + j);
            }
        }
    }
}

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
            ConjugateBySelfInverse c1(CircuitInstruction{GateType::H, {}, h_xz}, do_instruction_callback);
            ConjugateBySelfInverse c2(CircuitInstruction{GateType::H_YZ, {}, h_yz}, do_instruction_callback);
            ConjugateBySelfInverse c3(CircuitInstruction{GateType::CX, {}, cnot}, do_instruction_callback);
            do_instruction_callback(CircuitInstruction{GateType::M, mpp_op.args, meas});
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
            do_instruction_callback(CircuitInstruction{GateType::MPAD, mpp_op.args, &t});
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
        for_each_active_qubit_in<true, true>(current, [&](uint32_t q) {
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
    std::vector<GateTarget> *cnot_buf) {
    h_xz_buf->clear();
    h_yz_buf->clear();
    cnot_buf->clear();

    // Assemble quantum terms from the observable.
    uint64_t focus_qubit = UINT64_MAX;
    for_each_active_qubit_in<true, true>(observable, [&](uint32_t q) {
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
        ConjugateBySelfInverse c1(CircuitInstruction{GateType::H, {}, *h_xz_buf}, do_instruction_callback);
        ConjugateBySelfInverse c2(CircuitInstruction{GateType::H_YZ, {}, *h_yz_buf}, do_instruction_callback);
        ConjugateBySelfInverse c3(CircuitInstruction{GateType::CX, {}, *cnot_buf}, do_instruction_callback);
        do_instruction_callback(CircuitInstruction{g, {}, &t});
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
            obs, bits, invert_sign, do_instruction_callback, &h_xz_buf, &h_yz_buf, &cnot_buf);
    }
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

struct Simplifier {
    size_t num_qubits;
    std::function<void(const CircuitInstruction &inst)> yield;
    simd_bits<64> used;
    std::vector<GateTarget> qs1_buf;
    std::vector<GateTarget> qs2_buf;
    std::vector<GateTarget> qs_buf;

    Simplifier(size_t num_qubits, std::function<void(const CircuitInstruction &inst)> init_yield)
        : num_qubits(num_qubits), yield(init_yield), used(num_qubits) {
    }

    void do_xcz(SpanRef<const GateTarget> targets) {
        if (targets.empty()) {
            return;
        }

        qs_buf.clear();
        for (size_t k = 0; k < targets.size(); k += 2) {
            qs_buf.push_back(targets[k + 1]);
            qs_buf.push_back(targets[k]);
        }
        yield(CircuitInstruction{GateType::CX, {}, qs_buf});
    }

    void simplify_potentially_overlapping_1q_instruction(const CircuitInstruction &inst) {
        used.clear();

        size_t start = 0;
        for (size_t k = 0; k < inst.targets.size(); k++) {
            auto t = inst.targets[k];
            if (t.has_qubit_value() && used[t.qubit_value()]) {
                CircuitInstruction disjoint = CircuitInstruction{inst.gate_type, inst.args, inst.targets.sub(start, k)};
                simplify_disjoint_1q_instruction(disjoint);
                used.clear();
                start = k;
            }
            if (t.has_qubit_value()) {
                used[t.qubit_value()] = true;
            }
        }
        simplify_disjoint_1q_instruction(
            CircuitInstruction{inst.gate_type, inst.args, inst.targets.sub(start, inst.targets.size())});
    }

    void simplify_potentially_overlapping_2q_instruction(const CircuitInstruction &inst) {
        used.clear();

        size_t start = 0;
        for (size_t k = 0; k < inst.targets.size(); k += 2) {
            auto a = inst.targets[k];
            auto b = inst.targets[k + 1];
            if ((a.has_qubit_value() && used[a.qubit_value()]) || (b.has_qubit_value() && used[b.qubit_value()])) {
                CircuitInstruction disjoint = CircuitInstruction{inst.gate_type, inst.args, inst.targets.sub(start, k)};
                simplify_disjoint_2q_instruction(disjoint);
                used.clear();
                start = k;
            }
            if (a.has_qubit_value()) {
                used[a.qubit_value()] = true;
            }
            if (b.has_qubit_value()) {
                used[b.qubit_value()] = true;
            }
        }
        simplify_disjoint_2q_instruction(
            CircuitInstruction{inst.gate_type, inst.args, inst.targets.sub(start, inst.targets.size())});
    }

    void simplify_disjoint_1q_instruction(const CircuitInstruction &inst) {
        const auto &ts = inst.targets;

        switch (inst.gate_type) {
            case GateType::I:
                // Do nothing.
                break;
            case GateType::X:
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                break;
            case GateType::Y:
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::Z:
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::C_XYZ:
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                break;
            case GateType::C_ZYX:
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::H:
                yield({GateType::H, {}, ts});
                break;
            case GateType::H_XY:
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::H_YZ:
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::S:
                yield({GateType::S, {}, ts});
                break;
            case GateType::SQRT_X:
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                break;
            case GateType::SQRT_X_DAG:
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                break;
            case GateType::SQRT_Y:
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                break;
            case GateType::SQRT_Y_DAG:
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::S_DAG:
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                break;

            case GateType::MX:
                yield({GateType::H, {}, ts});
                yield({GateType::M, {}, ts});
                yield({GateType::H, {}, ts});
                break;
            case GateType::MY:
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                yield({GateType::M, {}, ts});
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::M:
                yield({GateType::M, {}, ts});
                break;
            case GateType::MRX:
                yield({GateType::H, {}, ts});
                yield({GateType::M, {}, ts});
                yield({GateType::R, {}, ts});
                yield({GateType::H, {}, ts});
                break;
            case GateType::MRY:
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::S, {}, ts});
                yield({GateType::H, {}, ts});
                yield({GateType::M, {}, ts});
                yield({GateType::R, {}, ts});
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::MR:
                yield({GateType::M, {}, ts});
                yield({GateType::R, {}, ts});
                break;
            case GateType::RX:
                yield({GateType::R, {}, ts});
                yield({GateType::H, {}, ts});
                break;
            case GateType::RY:
                yield({GateType::R, {}, ts});
                yield({GateType::H, {}, ts});
                yield({GateType::S, {}, ts});
                break;
            case GateType::R:
                yield({GateType::R, {}, ts});
                break;

            default:
                throw std::invalid_argument("Unhandled in Simplifier::simplify_disjoint_1q_instruction: " + inst.str());
        }
    }

    void simplify_disjoint_2q_instruction(const CircuitInstruction &inst) {
        const auto &ts = inst.targets;
        qs_buf.clear();
        qs1_buf.clear();
        qs2_buf.clear();
        for (size_t k = 0; k < inst.targets.size(); k += 2) {
            auto a = inst.targets[k];
            auto b = inst.targets[k + 1];
            if (a.has_qubit_value()) {
                auto t = GateTarget::qubit(a.qubit_value());
                qs1_buf.push_back(t);
                qs_buf.push_back(t);
            }
            if (b.has_qubit_value()) {
                auto t = GateTarget::qubit(b.qubit_value());
                qs2_buf.push_back(t);
                qs_buf.push_back(t);
            }
        }

        switch (inst.gate_type) {
            case GateType::CX:
                yield({GateType::CX, {}, ts});
                break;
            case GateType::XCZ:
                do_xcz(ts);
                break;
            case GateType::XCX:
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs1_buf});
                break;
            case GateType::XCY:
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::S, {}, qs2_buf});
                break;
            case GateType::YCX:
                yield({GateType::S, {}, qs1_buf});
                yield({GateType::S, {}, qs1_buf});
                yield({GateType::S, {}, qs1_buf});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::S, {}, qs1_buf});
                break;
            case GateType::YCY:
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::S, {}, qs_buf});
                break;
            case GateType::YCZ:
                yield({GateType::S, {}, qs1_buf});
                yield({GateType::S, {}, qs1_buf});
                yield({GateType::S, {}, qs1_buf});
                do_xcz(ts);
                yield({GateType::S, {}, qs1_buf});
                break;
            case GateType::CY:
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::S, {}, qs2_buf});
                break;
            case GateType::CZ:
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs2_buf});
                break;
            case GateType::SQRT_XX:
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::H, {}, qs_buf});
                break;
            case GateType::SQRT_XX_DAG:
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::H, {}, qs_buf});
                break;
            case GateType::SQRT_YY:
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::H, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                break;
            case GateType::SQRT_YY_DAG:
                yield({GateType::S, {}, qs1_buf});
                yield({GateType::S, {}, qs1_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::H, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::S, {}, qs2_buf});
                break;
            case GateType::SQRT_ZZ:
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::S, {}, qs_buf});
                break;
            case GateType::SQRT_ZZ_DAG:
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                break;
            case GateType::SWAP:
                yield({GateType::CX, {}, ts});
                do_xcz(ts);
                yield({GateType::CX, {}, ts});
                break;
            case GateType::ISWAP:
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                do_xcz(ts);
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::S, {}, qs_buf});
                break;
            case GateType::ISWAP_DAG:
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                do_xcz(ts);
                yield({GateType::H, {}, qs2_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                yield({GateType::S, {}, qs_buf});
                break;
            case GateType::CXSWAP:
                do_xcz(ts);
                yield({GateType::CX, {}, ts});
                break;
            case GateType::SWAPCX:
                yield({GateType::CX, {}, ts});
                do_xcz(ts);
                break;
            case GateType::CZSWAP:
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                do_xcz(ts);
                yield({GateType::H, {}, qs2_buf});
                break;

            case GateType::MXX:
                yield({GateType::CX, {}, ts});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::M, {}, qs1_buf});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                break;
            case GateType::MYY:
                yield({GateType::S, {}, qs_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::S, {}, qs2_buf});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::M, {}, qs1_buf});
                yield({GateType::H, {}, qs1_buf});
                yield({GateType::CX, {}, ts});
                yield({GateType::S, {}, qs_buf});
                break;
            case GateType::MZZ:
                yield({GateType::CX, {}, ts});
                yield({GateType::M, {}, qs2_buf});
                yield({GateType::CX, {}, ts});
                break;

            default:
                throw std::invalid_argument("Unhandled in Simplifier::simplify_instruction: " + inst.str());
        }
    }

    void simplify_instruction(const CircuitInstruction &inst) {
        const Gate &g = GATE_DATA[inst.gate_type];

        switch (inst.gate_type) {
            case GateType::MPP:
                decompose_mpp_operation(inst, num_qubits, [&](const CircuitInstruction sub) {
                    simplify_instruction(sub);
                });
                break;
            case GateType::SPP:
            case GateType::SPP_DAG:
                decompose_spp_or_spp_dag_operation(inst, num_qubits, false, [&](const CircuitInstruction sub) {
                    simplify_instruction(sub);
                });
                break;

            case GateType::MPAD:
                // Can't be easily simplified into M.
                yield(inst);
                break;

            case GateType::DETECTOR:
            case GateType::OBSERVABLE_INCLUDE:
            case GateType::TICK:
            case GateType::QUBIT_COORDS:
            case GateType::SHIFT_COORDS:
                // Annotations can't be simplified.
                yield(inst);
                break;

            case GateType::DEPOLARIZE1:
            case GateType::DEPOLARIZE2:
            case GateType::X_ERROR:
            case GateType::Y_ERROR:
            case GateType::Z_ERROR:
            case GateType::PAULI_CHANNEL_1:
            case GateType::PAULI_CHANNEL_2:
            case GateType::E:
            case GateType::ELSE_CORRELATED_ERROR:
            case GateType::HERALDED_ERASE:
            case GateType::HERALDED_PAULI_CHANNEL_1:
                // Noise isn't simplified.
                yield(inst);
                break;
            default: {
                if (g.flags & GATE_IS_SINGLE_QUBIT_GATE) {
                    simplify_potentially_overlapping_1q_instruction(inst);
                } else if (g.flags & GATE_TARGETS_PAIRS) {
                    simplify_potentially_overlapping_2q_instruction(inst);
                } else {
                    throw std::invalid_argument(
                        "Unhandled in simplify_potentially_overlapping_instruction: " + inst.str());
                }
            }
        }
    }
};

Circuit stim::simplified_circuit(const Circuit &circuit) {
    Circuit output;
    Simplifier simplifier(circuit.count_qubits(), [&](const CircuitInstruction &inst) {
        output.safe_append(inst);
    });
    for (auto inst : circuit.operations) {
        if (inst.gate_type == GateType::REPEAT) {
            output.append_repeat_block(
                inst.repeat_block_rep_count(), simplified_circuit(inst.repeat_block_body(circuit)));
        } else {
            simplifier.simplify_instruction(inst);
        }
    }
    return output;
}
