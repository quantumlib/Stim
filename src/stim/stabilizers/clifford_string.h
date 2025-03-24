///*
// * Copyright 2021 Google LLC
// *
// * Licensed under the Apache License, Version 2.0 (the "License");
// * you may not use this file except in compliance with the License.
// * You may obtain a copy of the License at
// *
// *      http://www.apache.org/licenses/LICENSE-2.0
// *
// * Unless required by applicable law or agreed to in writing, software
// * distributed under the License is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// * See the License for the specific language governing permissions and
// * limitations under the License.
// */
//
//#ifndef _STIM_STABILIZERS_CLIFFORD_STRING_H
//#define _STIM_STABILIZERS_CLIFFORD_STRING_H
//
//#include "stim/mem/simd_bits.h"
//#include "stim/gates/gates.h"
//
//namespace stim {
//
//template <size_t W>
//struct CliffordWord {
//    bitword<W> x_signs;
//    bitword<W> z_signs;
//    bitword<W> inv_x2x;
//    bitword<W> x2z;
//    bitword<W> z2x;
//    bitword<W> inv_z2z;
//};
//
//template <size_t W>
//inline CliffordWord<W> operator*(const CliffordWord<W> &lhs, const CliffordWord<W> &rhs) const {
//    CliffordWord<W> result;
//    result.inv_x2x = (lhs.inv_x2x | rhs.inv_x2x) ^ (lhs.z2x & rhs.x2z);
//    result.x2z = rhs.inv_x2x.andnot(lhs.x2z) ^ lhs.inv_z2z.andnot(rhs.x2z);
//    result.z2x = lhs.inv_x2x.andnot(rhs.z2x) ^ rhs.z2z.andnot(lhs.z2x);
//    result.inv_z2z = (lhs.x2z & rhs.z2x) ^ (lhs.inv_z2z | rhs.inv_z2z);
//    simd_word<W> rhs_x2y = rhs.inv_x2x.andnot(rhs.x2z);
//    simd_word<W> rhs_z2y = rhs.z2z.andnot(rhs.z2x);
//    simd_word<W> dy = (lhs.x2z & lhs.z2x) ^ lhs.inv_x2x ^ lhs.z2x ^ lhs.x2z ^ lhs.inv_z2z;
//    result.x_signs = (
//        rhs.x_signs
//        ^ rhs.inv_x2x.andnot(lhs.x_signs)
//        ^ (rhs_x2y & dy)
//        ^ (rhs.x2z & lhs.z_signs)
//    );
//    result.z_signs = (
//        rhs.z_signs
//        ^ (rhs.z2x & lhs.x_signs)
//        ^ (rhs_z2y & dy)
//        ^ rhs.inv_z2z.andnot(lhs.z_signs)
//    );
//    return result
//}
//
//template <size_t W>
//struct CliffordString {
//    size_t num_qubits;
//    size_t num_words;
//    CliffordWord<W> *buf;
//
//    CliffordString(size_t num_qubits, bool do_not_initialize = false)
//        : num_qubits(num_qubits),
//          num_words(((num_qubits + W - 1) / W) * W),
//          buf(nullptr) {
//        if (num_words) {
//            buf = bit_word<W>::aligned_malloc(num_words * (W / 8));
//        }
//    }
//    static inline CliffordString<W> uninitialized(size_t num_qubits) {
//        return CliffordString(num_qubits);
//    }
//    static inline CliffordString<W> identities(size_t num_qubits) {
//        CliffordString<W> result(num_qubits);
//        memset(result.buf, 0, num_words * (W / 8));
//        return result;
//    }
//
//    stim::GateType gate_at(size_t index) const {
//        return table[flat_value];
//    }
//    void set_gate_at(size_t index, GateType gate) const {
//        const auto &flows = stim::GATE_DATA[gate].flow_data;
//        std::string_view tx = flows[0];
//        std::string_view tz = flows[1];
//        bool inv_x2x = tx[0] == 'X' || tx[0] == 'Y';
//        bool x2z = tx[0] == 'Z' || tx[0] == 'Y';
//        bool x_sign = tx[0] == '-';
//
//        bool inv_z2x = tz[0] == 'X' || tz[0] == 'Y';
//        bool z2z = tz[0] == 'Z' || tz[0] == 'Y';
//        bool z_sign = tz[0] == '-';
//
//
//    }
//
//    CliffordString(const CliffordString &other)
//        : num_qubits(other.num_qubits),
//          num_words(other.num_words),
//          buf(nullptr) {
//        buf = bit_word<W>::aligned_malloc(num_words * (W / 8));
//        memcpy(buf, other.buf, num_words * (W / 8));
//    }
//    CliffordString(CliffordString &&other)
//        : num_qubits(other.num_qubits),
//          num_words(other.num_words),
//          buf(other.buf) {
//        other.buf = nullptr;
//        other.num_qubits = 0;
//        other.num_words = 0;
//    }
//    ~CliffordString() {
//        if (buf != nullptr) {
//            bit_word<W>::aligned_free(buf);
//            buf = nullptr;
//        }
//        num_qubits = 0;
//        num_words = 0;
//    }
//    CliffordString &operator=(const CliffordString &other) {
//        if (num_words != other.num_words && buf != nullptr) {
//            bit_word<W>::aligned_free(buf);
//            buf = nullptr;
//            num_words = 0;
//            num_qubits = 0;
//        }
//        if (buf == nullptr && num_words > 0) {
//            buf = bit_word<W>::aligned_malloc(other.num_words * (W / 8));
//        }
//        num_words = other.num_words;
//        num_qubits = other.num_qubits;
//        memcpy(buf, other.buf, num_words * (W / 8));
//        return *this;
//    }
//    CliffordString &operator=(CliffordString &&other) {
//        num_words = other.num_words;
//        num_qubits = other.num_qubits;
//        if (buf != nullptr) {
//            bit_word<W>::aligned_free(buf);
//        }
//        buf = other.buf;
//        other.buf = nullptr;
//        other.num_qubits = 0;
//        other.num_words = 0;
//        return *this;
//    }
//
//    CliffordString &operator*=(const CliffordString &rhs) {
//        if (num_words < rhs.num_words) {
//            throw std::invalid_argument("Can't inplace-multiply by a larger Clifford string.");
//        }
//        for (size_t k = 0; k < rhs.num_words; k++) {
//            buf[k] = buf[k] * rhs.buf[k];
//        }
//        return *this;
//    }
//    CliffordString &inplace_left_mul_by(const CliffordString &lhs) {
//        if (num_words < lhs.num_words) {
//            throw std::invalid_argument("Can't inplace-multiply by a larger Clifford string.");
//        }
//        for (size_t k = 0; k < num_words; k++) {
//            buf[k] = lhs.buf[k] * buf[k];
//        }
//        return *this;
//    }
//    CliffordString operator*(const CliffordString &rhs) const {
//        CliffordString<W> result = CliffordString<W>::uninitialized(std::max(num_qubits, rhs.num_qubits));
//        size_t min_words = std::min(num_words, rhs.num_words);
//        for (size_t k = 0; k < min_size; k++) {
//            result.buf[k] = buf[k] * rhs.buf[k];
//        }
//        for (size_t k = min_size; k < num_words; k++) {
//            result.buf[k] = buf[k];
//        }
//        for (size_t k = min_size; k < rhs.num_words; k++) {
//            result.buf[k] = rhs.buf[k];
//        }
//        return *this;
//    }
//};
//
///// Writes a string describing the given Clifford string to an output stream.
//template <size_t W>
//std::ostream &operator<<(std::ostream &out, const PauliString<W> &ps);
//
//}  // namespace stim
//
//#include "stim/stabilizers/pauli_string.inl"
//
//#endif
