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

#ifndef _STIM_STABILIZERS_CLIFFORD_STRING_H
#define _STIM_STABILIZERS_CLIFFORD_STRING_H

#include "stim/mem/simd_bits.h"
#include "stim/gates/gates.h"

namespace stim {

template <size_t W>
struct CliffordWord {
    bitword<W> x_signs;
    bitword<W> z_signs;
    bitword<W> inv_x2x;
    bitword<W> x2z;
    bitword<W> z2x;
    bitword<W> inv_z2z;
};

template <size_t W>
inline CliffordWord<W> operator*(const CliffordWord<W> &lhs, const CliffordWord<W> &rhs) {
    CliffordWord<W> result;

    // I don't have a simple explanation of why this is correct. It was produced by starting from something that was
    // obviously correct, having tests to check all 24*24 cases, then iteratively applying simple rewrites to reduce
    // the number of operations. So the result is correct, but somewhat incomprehensible.
    result.inv_x2x = (lhs.inv_x2x | rhs.inv_x2x) ^ lhs.z2x & rhs.x2z;
    result.x2z = rhs.inv_x2x.andnot(lhs.x2z) ^ lhs.inv_z2z.andnot(rhs.x2z);
    result.z2x = lhs.inv_x2x.andnot(rhs.z2x) ^ rhs.inv_z2z.andnot(lhs.z2x);
    result.inv_z2z = lhs.x2z & rhs.z2x ^ (lhs.inv_z2z | rhs.inv_z2z);

    // I *especially* don't have an explanation of why this part is correct. But every case is tested and verified.
    simd_word<W> rhs_x2y = rhs.inv_x2x.andnot(rhs.x2z);
    simd_word<W> rhs_z2y = rhs.inv_z2z.andnot(rhs.z2x);
    simd_word<W> dy = lhs.x2z & lhs.z2x ^ lhs.inv_x2x ^ lhs.z2x ^ lhs.x2z ^ lhs.inv_z2z;
    result.x_signs = rhs.x_signs
        ^ rhs.inv_x2x.andnot(lhs.x_signs)
        ^ rhs_x2y & dy
        ^ rhs.x2z & lhs.z_signs;
    result.z_signs = rhs.z_signs
        ^ rhs.z2x & lhs.x_signs
        ^ rhs_z2y & dy
        ^ rhs.inv_z2z.andnot(lhs.z_signs);

    return result;
}

template <size_t W>
struct CliffordString;

template <size_t W>
std::ostream &operator<<(std::ostream &out, const CliffordString<W> &v);

/// A string of single-qubit Clifford rotations.
template <size_t W>
struct CliffordString {
    size_t num_qubits;

    // The 2 sign bits of a single qubit Clifford, packed into arrays for easy processing.
    simd_bits<W> x_signs;
    simd_bits<W> z_signs;

    // The 4 tableau bits of a single qubit Clifford, packed into arrays for easy processing.
    // The x2x and z2z terms are inverted so that zero-initializing produces the identity gate.
    simd_bits<W> inv_x2x;
    simd_bits<W> x2z;
    simd_bits<W> z2x;
    simd_bits<W> inv_z2z;

    /// Constructs an identity CliffordString for the given number of qubits.
    explicit CliffordString(size_t num_qubits)
        : num_qubits(num_qubits),
          x_signs(num_qubits),
          z_signs(num_qubits),
          inv_x2x(num_qubits),
          x2z(num_qubits),
          z2x(num_qubits),
          inv_z2z(num_qubits) {
    }

    inline CliffordWord<W> word_at(size_t k) const {
        return CliffordWord<W>{
            x_signs.ptr_simd[k],
            z_signs.ptr_simd[k],
            inv_x2x.ptr_simd[k],
            x2z.ptr_simd[k],
            z2x.ptr_simd[k],
            inv_z2z.ptr_simd[k],
        };
    }
    inline void set_word_at(size_t k, CliffordWord<W> new_value) const {
        x_signs.ptr_simd[k] = new_value.x_signs;
        z_signs.ptr_simd[k] = new_value.z_signs;
        inv_x2x.ptr_simd[k] = new_value.inv_x2x;
        x2z.ptr_simd[k] = new_value.x2z;
        z2x.ptr_simd[k] = new_value.z2x;
        inv_z2z.ptr_simd[k] = new_value.inv_z2z;
    }

    GateType gate_at(size_t q) const {
        constexpr std::array<GateType, 64> table{
            GateType::I,
            GateType::X,
            GateType::Z,
            GateType::Y,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::S,
            GateType::H_XY,
            GateType::S_DAG,
            GateType::H_NXY,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::SQRT_X_DAG,
            GateType::SQRT_X,
            GateType::H_YZ,
            GateType::H_NYZ,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::C_ZYX,
            GateType::C_ZNYX,
            GateType::C_ZYNX,
            GateType::C_NZYX,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,
            GateType::NOT_A_GATE,

            GateType::C_XYZ,
            GateType::C_XYNZ,
            GateType::C_XNYZ,
            GateType::C_NXYZ,

            GateType::H,
            GateType::SQRT_Y_DAG,
            GateType::SQRT_Y,
            GateType::H_NXZ,
        };
        int k = x_signs[q]*2 + z_signs[q] + inv_x2x[q]*4 + x2z[q]*8 + z2x[q]*16 + inv_z2z[q]*32;
        return table[k];
    }

    void set_gate_at(size_t index, GateType gate_type) {
        Gate g = GATE_DATA[gate_type];
        if (!(g.flags & GATE_IS_SINGLE_QUBIT_GATE) || !(g.flags & GATE_IS_UNITARY)) {
            throw std::invalid_argument("Not a single qubit gate: " + std::string(g.name));
        }
        const auto &flows = g.flow_data;
        std::string_view tx = flows[0];
        std::string_view tz = flows[1];
        bool new_inv_x2x = !(tx[1] == 'X' || tx[1] == 'Y');
        bool new_x2z = tx[1] == 'Z' || tx[1] == 'Y';
        bool new_x_sign = tx[0] == '-';

        bool new_z2x = tz[1] == 'X' || tz[1] == 'Y';
        bool new_inv_z2z = !(tz[1] == 'Z' || tz[1] == 'Y');
        bool new_z_sign = tz[0] == '-';

        x_signs[index] = new_x_sign;
        z_signs[index] = new_z_sign;
        inv_x2x[index] = new_inv_x2x;
        x2z[index] = new_x2z;
        z2x[index] = new_z2x;
        inv_z2z[index] = new_inv_z2z;
    }

    CliffordString &operator*=(const CliffordString &rhs) {
        if (num_qubits < rhs.num_qubits) {
            throw std::invalid_argument("Can't inplace-multiply by a larger Clifford string.");
        }
        for (size_t k = 0; k < rhs.x_signs.num_simd_words; k++) {
            auto lhs_w = word_at(k);
            auto rhs_w = rhs.word_at(k);
            set_word_at(k, lhs_w * rhs_w);
        }
        return *this;
    }
    CliffordString &inplace_left_mul_by(const CliffordString &lhs) {
        if (num_qubits < lhs.num_qubits) {
            throw std::invalid_argument("Can't inplace-multiply by a larger Clifford string.");
        }
        for (size_t k = 0; k < x_signs.num_simd_words; k++) {
            auto lhs_w = lhs.word_at(k);
            auto rhs_w = word_at(k);
            set_word_at(k, lhs_w * rhs_w);
        }
        return *this;
    }
    CliffordString operator*(const CliffordString &rhs) const {
        CliffordString<W> result = CliffordString<W>(std::max(num_qubits, rhs.num_qubits));
        size_t min_words = std::min(x_signs.num_simd_words, rhs.x_signs.num_simd_words);
        for (size_t k = 0; k < min_words; k++) {
            auto lhs_w = word_at(k);
            auto rhs_w = rhs.word_at(k);
            result.set_word_at(k, lhs_w * rhs_w);
        }

        // The longer string copies its tail into the result.
        size_t min_qubits = std::min(num_qubits, rhs.num_qubits);
        for (size_t q = min_qubits; q < num_qubits; q++) {
            result.x_signs[q] = x_signs[q];
            result.z_signs[q] = z_signs[q];
            result.inv_x2x[q] = inv_x2x[q];
            result.x2z[q] = x2z[q];
            result.z2x[q] = z2x[q];
            result.inv_z2z[q] = inv_z2z[q];
        }
        for (size_t q = min_qubits; q < rhs.num_qubits; q++) {
            result.x_signs[q] = rhs.x_signs[q];
            result.z_signs[q] = rhs.z_signs[q];
            result.inv_x2x[q] = rhs.inv_x2x[q];
            result.x2z[q] = rhs.x2z[q];
            result.z2x[q] = rhs.z2x[q];
            result.inv_z2z[q] = rhs.inv_z2z[q];
        }

        return result;
    }

    bool operator==(const CliffordString<W> &other) const {
        return x_signs == other.x_signs
            && z_signs == other.z_signs
            && inv_x2x == other.inv_x2x
            && x2z == other.x2z
            && z2x == other.z2x
            && inv_z2z == other.inv_z2z;
    }
    bool operator!=(const CliffordString<W> &other) const {
        return !(*this == other);
    }

    std::string str() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
};

template <size_t W>
std::ostream &operator<<(std::ostream &out, const CliffordString<W> &v) {
    for (size_t q = 0; q < v.num_qubits; q++) {
        if (q > 0) {
            out << " ";
        }
        int c = v.inv_x2x[q] + v.x2z[q] * 2 + v.z2x[q] * 4 + v.inv_z2z[q] * 8;
        int p = v.z_signs[q] + v.x_signs[q] * 2;
        out << "_?S?V??D??????UH"[c];
        out << "IXZY"[p];
    }
    return out;
}

}  // namespace stim

#endif
