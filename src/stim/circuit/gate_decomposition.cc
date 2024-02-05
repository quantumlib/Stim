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
        CircuitInstruction inst,
        const std::function<void(const CircuitInstruction &inst)> &do_instruction_callback)
        : inst(inst),
          do_instruction_callback(do_instruction_callback) {
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
            uint32_t q = (uint32_t)(w*64 + j);
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
            obs->safe_accumulate_pauli_term(t, &imag);
        } else if (t.is_classical_bit_target() && bits != nullptr) {
            bits->push_back(t);
        } else {
            throw std::invalid_argument(
                "Found an unsupported target `" + t.str() + "` in " + instruction.str());
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

    auto flush = [&](){
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
    while (accumulate_next_obs_terms_to_pauli_string_helper(
        mpp_op,
        &start,
        &current,
        nullptr)) {

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
            obs,
            bits,
            invert_sign,
            do_instruction_callback,
            &h_xz_buf,
            &h_yz_buf,
            &cnot_buf);
    }
}

static void decompose_cpp_operation_with_reverse_independence_helper(
    CircuitInstruction cpp_op,
    PauliStringRef<64> obs1,
    PauliStringRef<64> obs2,
    std::span<const GateTarget> classical_bits1,
    std::span<const GateTarget> classical_bits2,
    const std::function<void(const CircuitInstruction &inst)> &do_instruction_callback,
    Circuit *workspace,
    std::vector<GateTarget> *buf) {
    assert(obs1.num_qubits == obs2.num_qubits);

    if (!obs1.commutes(obs2)) {
        std::stringstream ss;
        ss << "Attempted to CPP two anticommuting observables.\n";
        ss << "    obs1: " << obs1 << "\n";
        ss << "    obs2: " << obs2 << "\n";
        ss << "    instruction: " << cpp_op;
        throw std::invalid_argument(ss.str());
    }

    workspace->clear();
    auto apply_fixup = [&](CircuitInstruction inst) {
        workspace->safe_append(inst);
        obs1.do_instruction(inst);
        obs2.do_instruction(inst);
    };

    auto reduce = [&](PauliStringRef<64> target_obs) {
        // Turn all non-identity terms into Z terms.
        for_each_active_qubit_in<true, false>(target_obs, [&](uint32_t q) {
            GateTarget t = GateTarget::qubit(q);
            apply_fixup({target_obs.zs[q] ? GateType::H_YZ : GateType::H, {}, &t});
        });

        // Cancel any extra Z terms.
        uint64_t pivot = UINT64_MAX;
        for_each_active_qubit_in<true, true>(target_obs, [&](uint32_t q) {
            if (pivot == UINT64_MAX) {
                pivot = q;
            } else {
                std::array<GateTarget, 2> ts{GateTarget::qubit(q), GateTarget::qubit(pivot)};
                apply_fixup({GateType::CX, {}, ts});
            }
        });

        return pivot;
    };

    uint64_t pivot1 = reduce(obs1);
    uint64_t pivot2 = reduce(obs2);

    if (pivot1 == pivot2 && pivot1 != UINT64_MAX) {
        // Both observables had identical quantum parts (up to sign).
        // If their sign differed, we should do nothing.
        // If their sign matched, we should apply Z to obs1.
        assert(obs1.xs == obs2.xs);
        assert(obs1.zs == obs2.zs);
        obs2.zs[pivot2] = false;
        obs2.sign ^= obs1.sign;
        obs2.sign ^= true;
        pivot2 = UINT64_MAX;
    }
    assert(obs1.weight() <= 1);
    assert(obs2.weight() <= 1);
    assert((pivot1 == UINT64_MAX) == (obs1.weight() == 0));
    assert((pivot2 == UINT64_MAX) == (obs2.weight() == 0));
    assert(pivot1 == UINT64_MAX || obs1.xs[pivot1] + 2*obs1.zs[pivot1] == 2);
    assert(pivot1 == UINT64_MAX || obs2.xs[pivot1] + 2*obs2.zs[pivot1] == 0);
    assert(pivot2 == UINT64_MAX || obs1.xs[pivot2] + 2*obs1.zs[pivot2] == 0);
    assert(pivot2 == UINT64_MAX || obs2.xs[pivot2] + 2*obs2.zs[pivot2] == 2);

    // Apply rewrites.
    workspace->for_each_operation(do_instruction_callback);

    // Handle the quantum-quantum interaction.
    if (pivot1 != UINT64_MAX && pivot2 != UINT64_MAX) {
        assert(pivot1 != pivot2);
        std::array<GateTarget, 2> ts{GateTarget::qubit(pivot1), GateTarget::qubit(pivot2)};
        do_instruction_callback({GateType::CZ, {}, ts});
    }

    // Handle sign and classical feedback into obs1.
    if (pivot1 != UINT64_MAX) {
        for (const auto &t : classical_bits2) {
            std::array<GateTarget, 2> ts{t, GateTarget::qubit(pivot1)};
            do_instruction_callback({GateType::CZ, {}, ts});
        }
        if (obs2.sign) {
            GateTarget t = GateTarget::qubit(pivot1);
            do_instruction_callback({GateType::Z, {}, &t});
        }
    }

    // Handle sign and classical feedback into obs2.
    if (pivot2 != UINT64_MAX) {
        for (const auto &t : classical_bits1) {
            std::array<GateTarget, 2> ts{t, GateTarget::qubit(pivot2)};
            do_instruction_callback({GateType::CZ, {}, ts});
        }
        if (obs1.sign) {
            GateTarget t = GateTarget::qubit(pivot2);
            do_instruction_callback({GateType::Z, {}, &t});
        }
    }

    // Undo rewrites.
    workspace->for_each_operation_reverse([&](CircuitInstruction inst) {
        assert(inst.args.empty());
        if (inst.gate_type == GateType::CX) {
            buf->clear();
            for (size_t k = inst.targets.size(); k;) {
                k -= 2;
                buf->push_back(inst.targets[k]);
                buf->push_back(inst.targets[k + 1]);
            }
            do_instruction_callback({GateType::CX, {}, *buf});
        } else {
            assert(inst.gate_type == GATE_DATA[inst.gate_type].inverse().id);
            do_instruction_callback(inst);
        }
    });
}

void stim::decompose_cpp_operation_with_reverse_independence(
    const CircuitInstruction &cpp_op,
    size_t num_qubits,
    const std::function<void(const CircuitInstruction &inst)> &do_instruction_callback) {
    PauliString<64> obs1(num_qubits);
    PauliString<64> obs2(num_qubits);
    std::vector<GateTarget> bits1;
    std::vector<GateTarget> bits2;
    Circuit circuit_workspace;
    std::vector<GateTarget> target_buf;

    size_t start = 0;
    while (true) {
        bool b1 = accumulate_next_obs_terms_to_pauli_string_helper(cpp_op, &start, &obs1, &bits1);
        bool b2 = accumulate_next_obs_terms_to_pauli_string_helper(cpp_op, &start, &obs2, &bits2);
        if (!b2) {
            break;
        }
        if (!b1) {
            throw std::invalid_argument("Odd number of products.");
        }

        decompose_cpp_operation_with_reverse_independence_helper(
            cpp_op,
            obs1,
            obs2,
            bits1,
            bits2,
            do_instruction_callback,
            &circuit_workspace,
            &target_buf);
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
