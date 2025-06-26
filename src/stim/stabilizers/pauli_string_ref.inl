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

#include "stim/gates/gates.h"
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
    assert(num_qubits >= rhs.num_qubits);

    // Accumulator registers for counting mod 4 in parallel across each bit position.
    simd_word<W> cnt1{};
    simd_word<W> cnt2{};

    rhs.xs.for_each_word(
        rhs.zs, xs, zs, [&cnt1, &cnt2](simd_word<W> &x2, simd_word<W> &z2, simd_word<W> &x1, simd_word<W> &z1) {
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

}  // namespace stim
