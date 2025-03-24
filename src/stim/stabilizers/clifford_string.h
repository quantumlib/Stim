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
    result.inv_x2x = (lhs.inv_x2x | rhs.inv_x2x) ^ (lhs.z2x & rhs.x2z);
    result.x2z = rhs.inv_x2x.andnot(lhs.x2z) ^ lhs.inv_z2z.andnot(rhs.x2z);
    result.z2x = lhs.inv_x2x.andnot(rhs.z2x) ^ rhs.z2z.andnot(lhs.z2x);
    result.inv_z2z = (lhs.x2z & rhs.z2x) ^ (lhs.inv_z2z | rhs.inv_z2z);
    simd_word<W> rhs_x2y = rhs.inv_x2x.andnot(rhs.x2z);
    simd_word<W> rhs_z2y = rhs.z2z.andnot(rhs.z2x);
    simd_word<W> dy = (lhs.x2z & lhs.z2x) ^ lhs.inv_x2x ^ lhs.z2x ^ lhs.x2z ^ lhs.inv_z2z;
    result.x_signs = (
        rhs.x_signs
        ^ rhs.inv_x2x.andnot(lhs.x_signs)
        ^ (rhs_x2y & dy)
        ^ (rhs.x2z & lhs.z_signs)
    );
    result.z_signs = (
        rhs.z_signs
        ^ (rhs.z2x & lhs.x_signs)
        ^ (rhs_z2y & dy)
        ^ rhs.inv_z2z.andnot(lhs.z_signs)
    );
    return result;
}

template <size_t W>
struct CliffordString {
    size_t num_qubits;
    simd_bits<W> x_signs;
    simd_bits<W> z_signs;
    simd_bits<W> inv_x2x;
    simd_bits<W> x2z;
    simd_bits<W> z2x;
    simd_bits<W> inv_z2z;

    CliffordString(size_t num_qubits)
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

//    stim::GateType gate_at(size_t index) const {
//        return table[flat_value];
//    }

    void set_gate_at(size_t index, GateType gate) const {
        const auto &flows = stim::GATE_DATA[gate].flow_data;
        std::string_view tx = flows[0];
        std::string_view tz = flows[1];
        bool new_inv_x2x = !(tx[0] == 'X' || tx[0] == 'Y');
        bool new_x2z = tx[0] == 'Z' || tx[0] == 'Y';
        bool new_x_sign = tx[0] == '-';

        bool new_z2x = tz[0] == 'X' || tz[0] == 'Y';
        bool new_inv_z2z = !(tz[0] == 'Z' || tz[0] == 'Y');
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
        for (size_t k = 0; k < rhs.num_words; k++) {
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
        size_t min_words = std::min(x_signs.num_simd_words, rhs.x_signs.num_words);
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
        return *this;
    }
};

//template <size_t W>
//std::ostream &operator<<(std::ostream &out, const CliffordString<W> &v);

}  // namespace stim

#endif
