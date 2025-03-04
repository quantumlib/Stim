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

#include <cassert>
#include <sstream>

#include "stim/circuit/circuit.h"
#include "stim/circuit/gate_decomposition.h"
#include "stim/mem/simd_util.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

template <size_t W>
PauliStringRef<W>::PauliStringRef(
    size_t init_num_qubits, bit_ref init_sign, simd_bits_range_ref<W> init_xs, simd_bits_range_ref<W> init_zs)
    : num_qubits(init_num_qubits), sign(init_sign), xs(init_xs), zs(init_zs) {
    assert(init_xs.num_bits_padded() == init_zs.num_bits_padded());
    assert(init_xs.num_simd_words == (init_num_qubits + W - 1) / W);
}

template <size_t W>
std::string PauliStringRef<W>::sparse_str() const {
    std::stringstream out;
    out << "+-"[(bool)sign];
    bool first = true;
    for_each_active_pauli([&](size_t q) {
        auto p = xs[q] + 2 * zs[q];
        if (!first) {
            out << '*';
        }
        first = false;
        out << "IXZY"[p] << q;
    });
    if (first) {
        out << 'I';
    }
    return out.str();
}

template <size_t W>
void PauliStringRef<W>::swap_with(PauliStringRef<W> other) {
    assert(num_qubits == other.num_qubits);
    sign.swap_with(other.sign);
    xs.swap_with(other.xs);
    zs.swap_with(other.zs);
}

template <size_t W>
PauliStringRef<W> &PauliStringRef<W>::operator=(const PauliStringRef<W> &other) {
    assert(num_qubits == other.num_qubits);
    sign = other.sign;
    assert((bool)sign == (bool)other.sign);
    xs = other.xs;
    zs = other.zs;
    return *this;
}

template <size_t W>
std::string PauliStringRef<W>::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

template <size_t W>
bool PauliStringRef<W>::operator==(const PauliStringRef<W> &other) const {
    return num_qubits == other.num_qubits && sign == other.sign && xs == other.xs && zs == other.zs;
}

template <size_t W>
bool PauliStringRef<W>::operator!=(const PauliStringRef<W> &other) const {
    return !(*this == other);
}

template <size_t W>
bool PauliStringRef<W>::operator<(const PauliStringRef<W> &other) const {
    size_t n = std::min(num_qubits, other.num_qubits);
    for (size_t q = 0; q < n; q++) {
        uint8_t p1 = (xs[q] ^ zs[q]) + zs[q] * 2;
        uint8_t p2 = (other.xs[q] ^ other.zs[q]) + other.zs[q] * 2;
        if (p1 != p2) {
            return p1 < p2;
        }
    }
    if (num_qubits != other.num_qubits) {
        return num_qubits < other.num_qubits;
    }
    if (sign != other.sign) {
        return sign < other.sign;
    }
    return false;
}

template <size_t W>
PauliStringRef<W> &PauliStringRef<W>::operator*=(const PauliStringRef<W> &rhs) {
    uint8_t log_i = inplace_right_mul_returning_log_i_scalar(rhs);
    assert((log_i & 1) == 0);
    sign ^= log_i & 2;
    return *this;
}

template <size_t W>
uint8_t PauliStringRef<W>::inplace_right_mul_returning_log_i_scalar(const PauliStringRef<W> &rhs) noexcept {
    assert(num_qubits == rhs.num_qubits);

    // Accumulator registers for counting mod 4 in parallel across each bit position.
    simd_word<W> cnt1{};
    simd_word<W> cnt2{};

    xs.for_each_word(
        zs, rhs.xs, rhs.zs, [&cnt1, &cnt2](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            // Update the left hand side Paulis.
            auto old_x1 = x1;
            auto old_z1 = z1;
            x1 ^= x2;
            z1 ^= z2;

            // At each bit position: accumulate anti-commutation (+i or -i) counts.
            auto x1z2 = old_x1 & z2;
            auto anti_commutes = (x2 & old_z1) ^ x1z2;
            cnt2 ^= (cnt1 ^ x1 ^ z1 ^ x1z2) & anti_commutes;
            cnt1 ^= anti_commutes;
        });

    // Combine final anti-commutation phase tally (mod 4).
    auto s = (uint8_t)cnt1.popcount();
    s ^= cnt2.popcount() << 1;
    s ^= (uint8_t)rhs.sign << 1;
    return s & 3;
}

template <size_t W>
bool PauliStringRef<W>::commutes(const PauliStringRef<W> &other) const noexcept {
    if (num_qubits > other.num_qubits) {
        return other.commutes(*this);
    }
    simd_word<W> cnt1{};
    xs.for_each_word(
        zs, other.xs, other.zs, [&cnt1](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2) {
            cnt1 ^= (x1 & z2) ^ (x2 & z1);
        });
    return (cnt1.popcount() & 1) == 0;
}

template <size_t W>
void PauliStringRef<W>::do_tableau(const Tableau<W> &tableau, SpanRef<const size_t> indices, bool inverse) {
    if (tableau.num_qubits == 0 || indices.size() % tableau.num_qubits != 0) {
        throw std::invalid_argument("len(tableau) == 0 or len(indices) % len(tableau) != 0");
    }
    for (auto e : indices) {
        if (e >= num_qubits) {
            throw std::invalid_argument("Attempted to apply a tableau past the end of the pauli string.");
        }
    }
    if (inverse) {
        auto inverse_tableau = tableau.inverse();
        for (size_t k = indices.size(); k > 0;) {
            k -= tableau.num_qubits;
            inverse_tableau.apply_within(*this, {indices.ptr_start + k, indices.ptr_start + k + tableau.num_qubits});
        }
    } else {
        for (size_t k = 0; k < indices.size(); k += tableau.num_qubits) {
            tableau.apply_within(*this, {indices.ptr_start + k, indices.ptr_start + k + tableau.num_qubits});
        }
    }
}

template <size_t W>
void PauliStringRef<W>::undo_circuit(const Circuit &circuit) {
    circuit.for_each_operation_reverse([&](const CircuitInstruction &inst) {
        undo_instruction(inst);
    });
}

template <size_t W>
void PauliStringRef<W>::do_circuit(const Circuit &circuit) {
    circuit.for_each_operation([&](const CircuitInstruction &inst) {
        do_instruction(inst);
    });
}

template <size_t W>
void PauliStringRef<W>::undo_reset_xyz(const CircuitInstruction &inst) {
    bool x_dep, z_dep;
    if (inst.gate_type == GateType::R || inst.gate_type == GateType::MR) {
        x_dep = true;
        z_dep = false;
    } else if (inst.gate_type == GateType::RX || inst.gate_type == GateType::MRX) {
        x_dep = false;
        z_dep = true;
    } else if (inst.gate_type == GateType::RY || inst.gate_type == GateType::MRY) {
        x_dep = true;
        z_dep = true;
    } else {
        throw std::invalid_argument("Unrecognized measurement type: " + inst.str());
    }
    for (const auto &t : inst.targets) {
        assert(t.is_qubit_target());
        auto q = t.qubit_value();
        if (q < num_qubits && ((xs[q] & x_dep) ^ (zs[q] & z_dep))) {
            std::stringstream ss;
            ss << "The pauli observable '" << *this;
            ss << "' doesn't have a well specified value before '" << inst;
            ss << "' because it anticommutes with the reset.";
            throw std::invalid_argument(ss.str());
        }
    }
    for (const auto &t : inst.targets) {
        auto q = t.qubit_value();
        xs[q] = false;
        zs[q] = false;
    }
}

template <size_t W>
void PauliStringRef<W>::check_avoids_measurement(const CircuitInstruction &inst) {
    bool x_dep, z_dep;
    if (inst.gate_type == GateType::M) {
        x_dep = true;
        z_dep = false;
    } else if (inst.gate_type == GateType::MX) {
        x_dep = false;
        z_dep = true;
    } else if (inst.gate_type == GateType::MY) {
        x_dep = true;
        z_dep = true;
    } else {
        throw std::invalid_argument("Unrecognized measurement type: " + inst.str());
    }
    for (const auto &t : inst.targets) {
        assert(t.is_qubit_target());
        auto q = t.qubit_value();
        if (q < num_qubits && ((xs[q] & x_dep) ^ (zs[q] & z_dep))) {
            std::stringstream ss;
            ss << "The pauli observable '" << *this;
            ss << "' doesn't have a well specified value across '" << inst;
            ss << "' because it anticommutes with the measurement.";
            throw std::invalid_argument(ss.str());
        }
    }
}

template <size_t W>
void PauliStringRef<W>::check_avoids_reset(const CircuitInstruction &inst) {
    // Only fail if the pauli string actually touches the reset.
    for (const auto &t : inst.targets) {
        assert(t.is_qubit_target());
        auto q = t.qubit_value();
        if (q < num_qubits && (xs[q] || zs[q])) {
            std::stringstream ss;
            ss << "The pauli observable '" << *this;
            ss << "' doesn't have a well specified value after '" << inst;
            ss << "' because the reset discards information.";
            throw std::invalid_argument(ss.str());
        }
    }
}

template <size_t W>
void PauliStringRef<W>::check_avoids_MPP(const CircuitInstruction &inst) {
    size_t start = 0;
    const auto &targets = inst.targets;
    while (start < targets.size()) {
        size_t end = start + 1;
        bool anticommutes = false;
        while (true) {
            auto t = targets[end - 1];
            auto q = t.qubit_value();
            if (q < num_qubits) {
                anticommutes ^= zs[q] && (t.data & TARGET_PAULI_X_BIT);
                anticommutes ^= xs[q] && (t.data & TARGET_PAULI_Z_BIT);
            }
            if (end >= targets.size() || !targets[end].is_combiner()) {
                break;
            }
            end += 2;
        }
        if (anticommutes) {
            std::stringstream ss;
            ss << "The pauli observable '" << *this;
            ss << "' doesn't have a well specified value across '" << inst;
            ss << "' because it anticommutes with the measurement.";
            throw std::invalid_argument(ss.str());
        }
        start = end;
    }
}

template <size_t W>
void PauliStringRef<W>::do_instruction(const CircuitInstruction &inst) {
    for (const auto &t : inst.targets) {
        if (t.has_qubit_value() && t.qubit_value() >= num_qubits &&
            !(GATE_DATA[inst.gate_type].flags & GATE_HAS_NO_EFFECT_ON_QUBITS)) {
            std::stringstream ss;
            ss << "The instruction '" << inst;
            ss << "' targets qubits outside the pauli string '" << *this;
            ss << "'.";
            throw std::invalid_argument(ss.str());
        }
    }

    const auto &gate_data = GATE_DATA[inst.gate_type];
    switch (gate_data.id) {
        case GateType::H:
            do_H_XZ(inst);
            break;
        case GateType::H_YZ:
            do_H_YZ(inst);
            break;
        case GateType::H_XY:
            do_H_XY(inst);
            break;
        case GateType::H_NXY:
            do_H_NXY(inst);
            break;
        case GateType::H_NXZ:
            do_H_NXZ(inst);
            break;
        case GateType::H_NYZ:
            do_H_NYZ(inst);
            break;
        case GateType::C_XYZ:
            do_C_XYZ(inst);
            break;
        case GateType::C_NXYZ:
            do_C_NXYZ(inst);
            break;
        case GateType::C_XNYZ:
            do_C_XNYZ(inst);
            break;
        case GateType::C_XYNZ:
            do_C_XYNZ(inst);
            break;
        case GateType::C_ZYX:
            do_C_ZYX(inst);
            break;
        case GateType::C_NZYX:
            do_C_NZYX(inst);
            break;
        case GateType::C_ZNYX:
            do_C_ZNYX(inst);
            break;
        case GateType::C_ZYNX:
            do_C_ZYNX(inst);
            break;
        case GateType::SQRT_X:
            do_SQRT_X(inst);
            break;
        case GateType::SQRT_Y:
            do_SQRT_Y(inst);
            break;
        case GateType::S:
            do_SQRT_Z(inst);
            break;
        case GateType::SQRT_X_DAG:
            do_SQRT_X_DAG(inst);
            break;
        case GateType::SQRT_Y_DAG:
            do_SQRT_Y_DAG(inst);
            break;
        case GateType::S_DAG:
            do_SQRT_Z_DAG(inst);
            break;
        case GateType::SQRT_XX:
            do_SQRT_XX(inst);
            break;
        case GateType::SQRT_XX_DAG:
            do_SQRT_XX_DAG(inst);
            break;
        case GateType::SQRT_YY:
            do_SQRT_YY(inst);
            break;
        case GateType::SQRT_YY_DAG:
            do_SQRT_YY_DAG(inst);
            break;
        case GateType::SQRT_ZZ:
            do_SQRT_ZZ(inst);
            break;
        case GateType::SQRT_ZZ_DAG:
            do_SQRT_ZZ_DAG(inst);
            break;
        case GateType::CX:
            do_ZCX<false>(inst);
            break;
        case GateType::CY:
            do_ZCY<false>(inst);
            break;
        case GateType::CZ:
            do_ZCZ(inst);
            break;
        case GateType::SWAP:
            do_SWAP<false>(inst);
            break;
        case GateType::X:
            do_X(inst);
            break;
        case GateType::Y:
            do_Y(inst);
            break;
        case GateType::Z:
            do_Z(inst);
            break;
        case GateType::ISWAP:
            do_ISWAP<false>(inst);
            break;
        case GateType::ISWAP_DAG:
            do_ISWAP_DAG<false>(inst);
            break;
        case GateType::CXSWAP:
            do_CXSWAP<false>(inst);
            break;
        case GateType::CZSWAP:
            do_CZSWAP<false>(inst);
            break;
        case GateType::SWAPCX:
            do_SWAPCX<false>(inst);
            break;
        case GateType::XCX:
            do_XCX(inst);
            break;
        case GateType::XCY:
            do_XCY<false>(inst);
            break;
        case GateType::XCZ:
            do_XCZ<false>(inst);
            break;
        case GateType::YCX:
            do_YCX<false>(inst);
            break;
        case GateType::YCY:
            do_YCY(inst);
            break;
        case GateType::YCZ:
            do_YCZ<false>(inst);
            break;

        case GateType::DETECTOR:
        case GateType::OBSERVABLE_INCLUDE:
        case GateType::TICK:
        case GateType::QUBIT_COORDS:
        case GateType::SHIFT_COORDS:
        case GateType::MPAD:
        case GateType::I:
        case GateType::II:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
            // No effect.
            break;

        case GateType::R:
        case GateType::RX:
        case GateType::RY:
        case GateType::MR:
        case GateType::MRX:
        case GateType::MRY:
            check_avoids_reset(inst);
            break;

        case GateType::M:
        case GateType::MX:
        case GateType::MY:
            check_avoids_measurement(inst);
            break;

        case GateType::MPP:
            check_avoids_MPP(inst);
            break;

        case GateType::SPP:
        case GateType::SPP_DAG:
            decompose_spp_or_spp_dag_operation(inst, num_qubits, false, [&](CircuitInstruction sub_inst) {
                do_instruction(sub_inst);
            });
            break;

        case GateType::X_ERROR:
        case GateType::Y_ERROR:
        case GateType::Z_ERROR:
        case GateType::DEPOLARIZE1:
        case GateType::DEPOLARIZE2:
        case GateType::E:
        case GateType::ELSE_CORRELATED_ERROR: {
            std::stringstream ss;
            ss << "The pauli string '" << *this;
            ss << "' doesn't have a well defined deterministic value after '" << inst;
            ss << "'.";
            throw std::invalid_argument(ss.str());
        }

        default:
            throw std::invalid_argument("Not implemented in PauliStringRef<W>::do_instruction: " + inst.str());
    }
}

template <size_t W>
void PauliStringRef<W>::undo_instruction(const CircuitInstruction &inst) {
    for (const auto &t : inst.targets) {
        if (t.has_qubit_value() && t.qubit_value() >= num_qubits &&
            !(GATE_DATA[inst.gate_type].flags & GATE_HAS_NO_EFFECT_ON_QUBITS)) {
            std::stringstream ss;
            ss << "The instruction '" << inst;
            ss << "' targets qubits outside the pauli string '" << *this;
            ss << "'.";
            throw std::invalid_argument(ss.str());
        }
    }

    const auto &gate_data = GATE_DATA[inst.gate_type];
    switch (gate_data.id) {
        case GateType::H:
            do_H_XZ(inst);
            break;
        case GateType::H_YZ:
            do_H_YZ(inst);
            break;
        case GateType::H_XY:
            do_H_XY(inst);
            break;
        case GateType::H_NXY:
            do_H_NXY(inst);
            break;
        case GateType::H_NXZ:
            do_H_NXZ(inst);
            break;
        case GateType::H_NYZ:
            do_H_NYZ(inst);
            break;
        case GateType::C_XYZ:
            do_C_ZYX(inst);
            break;
        case GateType::C_NXYZ:
            do_C_ZYNX(inst);
            break;
        case GateType::C_XNYZ:
            do_C_ZNYX(inst);
            break;
        case GateType::C_XYNZ:
            do_C_NZYX(inst);
            break;
        case GateType::C_ZYX:
            do_C_XYZ(inst);
            break;
        case GateType::C_NZYX:
            do_C_XYNZ(inst);
            break;
        case GateType::C_ZNYX:
            do_C_XNYZ(inst);
            break;
        case GateType::C_ZYNX:
            do_C_NXYZ(inst);
            break;
        case GateType::SQRT_X:
            do_SQRT_X_DAG(inst);
            break;
        case GateType::SQRT_Y:
            do_SQRT_Y_DAG(inst);
            break;
        case GateType::S:
            do_SQRT_Z_DAG(inst);
            break;
        case GateType::SQRT_X_DAG:
            do_SQRT_X(inst);
            break;
        case GateType::SQRT_Y_DAG:
            do_SQRT_Y(inst);
            break;
        case GateType::S_DAG:
            do_SQRT_Z(inst);
            break;
        case GateType::SQRT_XX:
            do_SQRT_XX_DAG(inst);
            break;
        case GateType::SQRT_XX_DAG:
            do_SQRT_XX(inst);
            break;
        case GateType::SQRT_YY:
            do_SQRT_YY_DAG(inst);
            break;
        case GateType::SQRT_YY_DAG:
            do_SQRT_YY(inst);
            break;
        case GateType::SQRT_ZZ:
            do_SQRT_ZZ_DAG(inst);
            break;
        case GateType::SQRT_ZZ_DAG:
            do_SQRT_ZZ(inst);
            break;
        case GateType::CX:
            do_ZCX<true>(inst);
            break;
        case GateType::CY:
            do_ZCY<true>(inst);
            break;
        case GateType::CZ:
            do_ZCZ(inst);
            break;
        case GateType::SWAP:
            do_SWAP<true>(inst);
            break;
        case GateType::X:
            do_X(inst);
            break;
        case GateType::Y:
            do_Y(inst);
            break;
        case GateType::Z:
            do_Z(inst);
            break;
        case GateType::ISWAP:
            do_ISWAP_DAG<true>(inst);
            break;
        case GateType::ISWAP_DAG:
            do_ISWAP<true>(inst);
            break;
        case GateType::CXSWAP:
            do_SWAPCX<true>(inst);
            break;
        case GateType::CZSWAP:
            do_CZSWAP<true>(inst);
            break;
        case GateType::SWAPCX:
            do_CXSWAP<true>(inst);
            break;
        case GateType::XCX:
            do_XCX(inst);
            break;
        case GateType::XCY:
            do_XCY<true>(inst);
            break;
        case GateType::XCZ:
            do_XCZ<true>(inst);
            break;
        case GateType::YCX:
            do_YCX<true>(inst);
            break;
        case GateType::YCY:
            do_YCY(inst);
            break;
        case GateType::YCZ:
            do_YCZ<true>(inst);
            break;

        case GateType::SPP:
        case GateType::SPP_DAG: {
            std::vector<GateTarget> buf_targets;
            buf_targets.insert(buf_targets.end(), inst.targets.begin(), inst.targets.end());
            std::reverse(buf_targets.begin(), buf_targets.end());
            decompose_spp_or_spp_dag_operation(
                CircuitInstruction{inst.gate_type, {}, buf_targets, inst.tag},
                num_qubits,
                false,
                [&](CircuitInstruction sub_inst) {
                    undo_instruction(sub_inst);
                });
            break;
        }

        case GateType::DETECTOR:
        case GateType::OBSERVABLE_INCLUDE:
        case GateType::TICK:
        case GateType::QUBIT_COORDS:
        case GateType::SHIFT_COORDS:
        case GateType::MPAD:
        case GateType::I:
        case GateType::II:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
            // No effect.
            break;

        case GateType::R:
        case GateType::RX:
        case GateType::RY:
        case GateType::MR:
        case GateType::MRX:
        case GateType::MRY:
            undo_reset_xyz(inst);
            break;

        case GateType::M:
        case GateType::MX:
        case GateType::MY:
            check_avoids_measurement(inst);
            break;

        case GateType::MPP:
            check_avoids_MPP(inst);
            break;

        case GateType::X_ERROR:
        case GateType::Y_ERROR:
        case GateType::Z_ERROR:
        case GateType::DEPOLARIZE1:
        case GateType::DEPOLARIZE2:
        case GateType::E:
        case GateType::ELSE_CORRELATED_ERROR: {
            std::stringstream ss;
            ss << "The pauli string '" << *this;
            ss << "' doesn't have a well defined deterministic value before '" << inst;
            ss << "'.";
            throw std::invalid_argument(ss.str());
        }

        default:
            throw std::invalid_argument("Not implemented in PauliStringRef<W>::undo_instruction: " + inst.str());
    }
}

template <size_t W>
PauliString<W> PauliStringRef<W>::after(const Circuit &circuit) const {
    PauliString<W> result = *this;
    result.ref().do_circuit(circuit);
    return result;
}

template <size_t W>
PauliString<W> PauliStringRef<W>::after(const Tableau<W> &tableau, SpanRef<const size_t> indices) const {
    PauliString<W> result = *this;
    result.ref().do_tableau(tableau, indices, false);
    return result;
}

template <size_t W>
PauliString<W> PauliStringRef<W>::after(const CircuitInstruction &inst) const {
    PauliString<W> result = *this;
    result.ref().do_instruction(inst);
    return result;
}

template <size_t W>
PauliString<W> PauliStringRef<W>::before(const Circuit &circuit) const {
    PauliString<W> result = *this;
    result.ref().undo_circuit(circuit);
    return result;
}

template <size_t W>
PauliString<W> PauliStringRef<W>::before(const CircuitInstruction &inst) const {
    PauliString<W> result = *this;
    result.ref().undo_instruction(inst);
    return result;
}

template <size_t W>
PauliString<W> PauliStringRef<W>::before(const Tableau<W> &tableau, SpanRef<const size_t> indices) const {
    PauliString<W> result = *this;
    result.ref().do_tableau(tableau, indices, true);
    return result;
}

template <size_t W>
void PauliStringRef<W>::gather_into(PauliStringRef<W> out, SpanRef<const size_t> in_indices) const {
    assert(in_indices.size() == out.num_qubits);
    for (size_t k_out = 0; k_out < out.num_qubits; k_out++) {
        size_t k_in = in_indices[k_out];
        out.xs[k_out] = xs[k_in];
        out.zs[k_out] = zs[k_in];
    }
}

template <size_t W>
void PauliStringRef<W>::scatter_into(PauliStringRef<W> out, SpanRef<const size_t> out_indices) const {
    assert(num_qubits == out_indices.size());
    for (size_t k_in = 0; k_in < num_qubits; k_in++) {
        size_t k_out = out_indices[k_in];
        out.xs[k_out] = xs[k_in];
        out.zs[k_out] = zs[k_in];
    }
    out.sign ^= sign;
}

template <size_t W>
bool PauliStringRef<W>::intersects(const PauliStringRef<W> other) const {
    size_t n = std::min(xs.num_u64_padded(), other.xs.num_u64_padded());
    uint64_t v = 0;
    for (size_t k = 0; k < n; k++) {
        v |= (xs.u64[k] | zs.u64[k]) & (other.xs.u64[k] | other.zs.u64[k]);
    }
    return v != 0;
}

template <size_t W>
size_t PauliStringRef<W>::weight() const {
    size_t total = 0;
    xs.for_each_word(zs, [&](const simd_word<W> &w1, const simd_word<W> &w2) {
        total += (w1 | w2).popcount();
    });
    return total;
}

template <size_t W>
bool PauliStringRef<W>::has_no_pauli_terms() const {
    size_t total = 0;
    size_t n = xs.num_u64_padded();
    for (size_t k = 0; k < n; k++) {
        total |= xs.u64[k] | zs.u64[k];
    }
    return total == 0;
}

template <size_t W>
std::ostream &operator<<(std::ostream &out, const PauliStringRef<W> &ps) {
    out << "+-"[ps.sign];
    for (size_t k = 0; k < ps.num_qubits; k++) {
        out << "_XZY"[ps.xs[k] + 2 * ps.zs[k]];
    }
    return out;
}

template <size_t W>
void PauliStringRef<W>::do_H_XZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q].swap_with(zs[q]);
        sign ^= xs[q] && zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_H_NXY(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        zs[q] ^= xs[q];
        sign ^= !xs[q] && !zs[q];
        sign ^= true;
    }
}

template <size_t W>
void PauliStringRef<W>::do_H_NXZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q].swap_with(zs[q]);
        sign ^= !xs[q] && !zs[q];
        sign ^= true;
    }
}

template <size_t W>
void PauliStringRef<W>::do_H_NYZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q] ^= zs[q];
        sign ^= !xs[q] && !zs[q];
        sign ^= true;
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_Y(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q].swap_with(zs[q]);
        sign ^= !xs[q] && zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_Y_DAG(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q].swap_with(zs[q]);
        sign ^= xs[q] && !zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_H_XY(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        zs[q] ^= xs[q];
        sign ^= !xs[q] && zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_Z(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        zs[q] ^= xs[q];
        sign ^= xs[q] && !zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_Z_DAG(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        zs[q] ^= xs[q];
        sign ^= xs[q] && zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_H_YZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q] ^= zs[q];
        sign ^= xs[q] && !zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_X(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q] ^= zs[q];
        sign ^= xs[q] && zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_X_DAG(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q] ^= zs[q];
        sign ^= !xs[q] && zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_C_XYZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_C_NXYZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= xs[q];
        sign ^= zs[q];
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_C_XNYZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= xs[q];
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_C_XYNZ(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= zs[q];
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_C_ZYX(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_C_ZYNX(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= xs[q];
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_C_ZNYX(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= zs[q];
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_C_NZYX(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= xs[q];
        sign ^= zs[q];
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_X(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_Y(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= xs[q] ^ zs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_Z(const CircuitInstruction &inst) {
    for (auto t : inst.targets) {
        auto q = t.data;
        sign ^= xs[q];
    }
}

template <size_t W>
void PauliStringRef<W>::do_single_cx(const CircuitInstruction &inst, uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        bit_ref x1 = xs[c], x2 = xs[t], z1 = zs[c], z2 = zs[t];
        z1 ^= z2;
        x2 ^= x1;
        sign ^= x1 && z2 && (z1 == x2);
    } else if (t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) {
        throw std::invalid_argument(
            "CX had a bit (" + GateTarget{t}.str() + ") as its target, instead of its control.");
    } else {
        if (zs[t]) {
            std::stringstream ss;
            ss << "The pauli observable '" << *this;
            ss << "' is affected by a controlled operation in '" << inst;
            ss << "' but the controlling measurement result isn't known.";
            throw std::invalid_argument(ss.str());
        }
    }
}

template <size_t W>
void PauliStringRef<W>::do_single_cy(const CircuitInstruction &inst, uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        bit_ref x1 = xs[c], x2 = xs[t], z1 = zs[c], z2 = zs[t];
        z1 ^= x2 ^ z2;
        z2 ^= x1;
        x2 ^= x1;
        sign ^= x1 && !z1 && x2 && !z2;
        sign ^= x1 && z1 && !x2 && z2;
    } else if (t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) {
        throw std::invalid_argument(
            "CY had a bit (" + GateTarget{t}.str() + ") as its target, instead of its control.");
    } else {
        if (xs[t] ^ zs[t]) {
            std::stringstream ss;
            ss << "The pauli observable '" << *this;
            ss << "' is affected by a controlled operation in '" << inst;
            ss << "' but the controlling measurement result isn't known.";
            throw std::invalid_argument(ss.str());
        }
    }
}

template <size_t W>
void PauliStringRef<W>::do_single_cz(const CircuitInstruction &inst, uint32_t c, uint32_t t) {
    c &= ~TARGET_INVERTED_BIT;
    t &= ~TARGET_INVERTED_BIT;
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        bit_ref x1 = xs[c], x2 = xs[t], z1 = zs[c], z2 = zs[t];
        z1 ^= x2;
        z2 ^= x1;
        sign ^= x1 && x2 && (z1 ^ z2);
    } else {
        bool bc = !(c & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) && xs[c];
        bool bt = !(t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) && xs[t];
        if (bc || bt) {
            std::stringstream ss;
            ss << "The pauli observable '" << *this;
            ss << "' is affected by a controlled operation in '" << inst;
            ss << "' but the controlling measurement result isn't known.";
            throw std::invalid_argument(ss.str());
        }
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_ZCX(const CircuitInstruction &inst) {
    assert((inst.targets.size() & 1) == 0);
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        do_single_cx(inst, q1, q2);
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_ZCY(const CircuitInstruction &inst) {
    assert((inst.targets.size() & 1) == 0);
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        do_single_cy(inst, q1, q2);
    }
}

template <size_t W>
void PauliStringRef<W>::do_ZCZ(const CircuitInstruction &inst) {
    const auto &targets = inst.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        do_single_cz(inst, targets[k].data, targets[k + 1].data);
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_SWAP(const CircuitInstruction &inst) {
    const auto &targets = inst.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t k2 = reverse_order ? targets.size() - 2 - k : k;
        size_t q1 = targets[k2].data, q2 = targets[k2 + 1].data;
        zs[q1].swap_with(zs[q2]);
        xs[q1].swap_with(xs[q2]);
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_ISWAP(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        sign ^= x1 && z1 && !x2 && !z2;
        sign ^= !x1 && !z1 && x2 && z2;
        sign ^= (x1 ^ x2) && z1 && z2;
        auto dx = x1 ^ x2;
        z1 ^= dx;
        z2 ^= dx;
        z1.swap_with(z2);
        x1.swap_with(x2);
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_ISWAP_DAG(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        auto dx = x1 ^ x2;
        z1 ^= dx;
        z2 ^= dx;
        z1.swap_with(z2);
        x1.swap_with(x2);
        sign ^= x1 && z1 && !x2 && !z2;
        sign ^= !x1 && !z1 && x2 && z2;
        sign ^= (x1 ^ x2) && z1 && z2;
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_CXSWAP(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        sign ^= x1 && z2 && (z1 == x2);
        z2 ^= z1;
        z1 ^= z2;
        x1 ^= x2;
        x2 ^= x1;
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_CZSWAP(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        z1.swap_with(z2);
        x1.swap_with(x2);
        z1 ^= x2;
        z2 ^= x1;
        sign ^= x1 && x2 && (z1 ^ z2);
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_SWAPCX(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        z1 ^= z2;
        z2 ^= z1;
        x2 ^= x1;
        x1 ^= x2;
        sign ^= x1 && z2 && (z1 == x2);
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_XX(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t q1 = inst.targets[k].data, q2 = inst.targets[k + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        sign ^= !x1 && z1 && !z2;
        sign ^= !x2 && !z1 && z2;
        auto dz = z1 ^ z2;
        x1 ^= dz;
        x2 ^= dz;
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_XX_DAG(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t q1 = inst.targets[k].data, q2 = inst.targets[k + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        auto dz = z1 ^ z2;
        x1 ^= dz;
        x2 ^= dz;
        sign ^= !x1 && z1 && !z2;
        sign ^= !x2 && !z1 && z2;
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_YY(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t q1 = inst.targets[k].data, q2 = inst.targets[k + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        sign ^= x1 && z1 && x2 && !z2;
        sign ^= x1 && !z1 && x2 && z2;
        sign ^= x1 && !z1 && !x2 && !z2;
        sign ^= !x1 && !z1 && x2 && !z2;
        auto d = x1 ^ z1 ^ x2 ^ z2;
        x1 ^= d;
        z1 ^= d;
        x2 ^= d;
        z2 ^= d;
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_YY_DAG(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t q1 = inst.targets[k].data, q2 = inst.targets[k + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        auto d = x1 ^ z1 ^ x2 ^ z2;
        x1 ^= d;
        z1 ^= d;
        x2 ^= d;
        z2 ^= d;
        sign ^= x1 && z1 && x2 && !z2;
        sign ^= x1 && !z1 && x2 && z2;
        sign ^= x1 && !z1 && !x2 && !z2;
        sign ^= !x1 && !z1 && x2 && !z2;
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_ZZ(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t q1 = inst.targets[k].data, q2 = inst.targets[k + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        auto dx = x1 ^ x2;
        z1 ^= dx;
        z2 ^= dx;
        sign ^= !z1 && x1 && !x2;
        sign ^= !z2 && !x1 && x2;
    }
}

template <size_t W>
void PauliStringRef<W>::do_SQRT_ZZ_DAG(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t q1 = inst.targets[k].data, q2 = inst.targets[k + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        sign ^= !z1 && x1 && !x2;
        sign ^= !z2 && !x1 && x2;
        auto dx = x1 ^ x2;
        z1 ^= dx;
        z2 ^= dx;
    }
}

template <size_t W>
void PauliStringRef<W>::do_XCX(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t q1 = inst.targets[k].data, q2 = inst.targets[k + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        sign ^= (x1 != x2) && z1 && z2;
        x1 ^= z2;
        x2 ^= z1;
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_XCY(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        x1 ^= x2 ^ z2;
        x2 ^= z1;
        z2 ^= z1;
        sign ^= !x1 && z1 && !x2 && z2;
        sign ^= x1 && z1 && x2 && !z2;
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_XCZ(const CircuitInstruction &inst) {
    const auto &targets = inst.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t k2 = reverse_order ? targets.size() - 2 - k : k;
        size_t q1 = targets[k2].data, q2 = targets[k2 + 1].data;
        do_single_cx(inst, q2, q1);
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_YCX(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t k2 = reverse_order ? inst.targets.size() - 2 - k : k;
        size_t q1 = inst.targets[k2].data, q2 = inst.targets[k2 + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        x2 ^= x1 ^ z1;
        x1 ^= z2;
        z1 ^= z2;
        sign ^= !x2 && z2 && !x1 && z1;
        sign ^= x2 && z2 && x1 && !z1;
    }
}

template <size_t W>
void PauliStringRef<W>::do_YCY(const CircuitInstruction &inst) {
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        size_t q1 = inst.targets[k].data, q2 = inst.targets[k + 1].data;
        bit_ref x1 = xs[q1], z1 = zs[q1], x2 = xs[q2], z2 = zs[q2];
        bool y1 = x1 ^ z1;
        bool y2 = x2 ^ z2;
        x1 ^= y2;
        z1 ^= y2;
        x2 ^= y1;
        z2 ^= y1;
        sign ^= x1 && !z1 && !x2 && z2;
        sign ^= !x1 && z1 && x2 && !z2;
    }
}

template <size_t W>
template <bool reverse_order>
void PauliStringRef<W>::do_YCZ(const CircuitInstruction &inst) {
    const auto &targets = inst.targets;
    assert((targets.size() & 1) == 0);
    for (size_t k = 0; k < targets.size(); k += 2) {
        size_t k2 = reverse_order ? targets.size() - 2 - k : k;
        size_t q1 = targets[k2].data, q2 = targets[k2 + 1].data;
        do_single_cy(inst, q2, q1);
    }
}

}  // namespace stim
