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
#include <cstring>
#include <string>

#include "stim/mem/simd_util.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

template <size_t W>
PauliString<W>::operator const PauliStringRef<W>() const {
    return ref();
}

template <size_t W>
PauliString<W>::operator PauliStringRef<W>() {
    return ref();
}

template <size_t W>
PauliString<W>::PauliString(size_t num_qubits) : num_qubits(num_qubits), sign(false), xs(num_qubits), zs(num_qubits) {
}

template <size_t W>
PauliString<W>::PauliString(std::string_view text) : num_qubits(0), sign(false), xs(0), zs(0) {
    *this = std::move(PauliString<W>::from_str(text));
}

template <size_t W>
PauliString<W>::PauliString(const PauliStringRef<W> &other)
    : num_qubits(other.num_qubits), sign((bool)other.sign), xs(other.xs), zs(other.zs) {
}

template <size_t W>
PauliString<W>::PauliString(const PauliString<W> &other)
    : num_qubits(other.num_qubits), sign((bool)other.sign), xs(other.xs), zs(other.zs) {
}

template <size_t W>
PauliString<W>::PauliString(PauliString<W> &&other) noexcept
    : num_qubits(other.num_qubits), sign((bool)other.sign), xs(std::move(other.xs)), zs(std::move(other.zs)) {
    other.num_qubits = 0;
    other.sign = false;
}

template <size_t W>
PauliString<W> &PauliString<W>::operator=(const PauliStringRef<W> &other) {
    xs = other.xs;
    zs = other.zs;
    num_qubits = other.num_qubits;
    sign = other.sign;
    return *this;
}

template <size_t W>
PauliString<W> &PauliString<W>::operator=(const PauliString<W> &other) {
    xs = other.xs;
    zs = other.zs;
    num_qubits = other.num_qubits;
    sign = other.sign;
    return *this;
}

template <size_t W>
PauliString<W> &PauliString<W>::operator=(PauliString<W> &&other) {
    if (&other == this) {
        return *this;
    }
    xs = std::move(other.xs);
    zs = std::move(other.zs);
    num_qubits = other.num_qubits;
    sign = other.sign;
    other.num_qubits = 0;
    other.sign = false;
    return *this;
}

template <size_t W>
const PauliStringRef<W> PauliString<W>::ref() const {
    size_t nw = (num_qubits + W - 1) / W;
    return PauliStringRef<W>(
        num_qubits,
        // HACK: const correctness is temporarily removed, but immediately restored.
        bit_ref((bool *)&sign, 0),
        xs.word_range_ref(0, nw),
        zs.word_range_ref(0, nw));
}

template <size_t W>
PauliStringRef<W> PauliString<W>::ref() {
    size_t nw = (num_qubits + W - 1) / W;
    return PauliStringRef<W>(num_qubits, bit_ref(&sign, 0), xs.word_range_ref(0, nw), zs.word_range_ref(0, nw));
}

template <size_t W>
std::string PauliString<W>::str() const {
    return ref().str();
}

template <size_t W>
PauliString<W> PauliString<W>::from_func(bool sign, size_t num_qubits, const std::function<char(size_t)> &func) {
    PauliString<W> result(num_qubits);
    result.sign = sign;
    for (size_t i = 0; i < num_qubits; i++) {
        char c = func(i);
        bool x;
        bool z;
        if (c == 'X') {
            x = true;
            z = false;
        } else if (c == 'Y') {
            x = true;
            z = true;
        } else if (c == 'Z') {
            x = false;
            z = true;
        } else if (c == '_' || c == 'I') {
            x = false;
            z = false;
        } else {
            throw std::invalid_argument("Unrecognized pauli character. " + std::to_string(c));
        }
        result.xs.u64[i / 64] ^= (uint64_t)x << (i & 63);
        result.zs.u64[i / 64] ^= (uint64_t)z << (i & 63);
    }
    return result;
}

template <size_t W>
PauliString<W> PauliString<W>::from_str(std::string_view text) {
    bool is_negated = text.starts_with('-');
    bool is_prefixed = text.starts_with('+');
    if (is_prefixed || is_negated) {
        text = text.substr(1);
    }
    return PauliString::from_func(is_negated, text.size(), [&](size_t i) {
        return text[i];
    });
}

template <size_t W>
PauliString<W> PauliString<W>::random(size_t num_qubits, std::mt19937_64 &rng) {
    auto result = PauliString(num_qubits);
    result.xs.randomize(num_qubits, rng);
    result.zs.randomize(num_qubits, rng);
    result.sign ^= rng() & 1;
    return result;
}

template <size_t W>
bool PauliString<W>::operator==(const PauliStringRef<W> &other) const {
    return ref() == other;
}

template <size_t W>
bool PauliString<W>::operator==(const PauliString<W> &other) const {
    return ref() == other.ref();
}

template <size_t W>
bool PauliString<W>::operator!=(const PauliStringRef<W> &other) const {
    return ref() != other;
}

template <size_t W>
bool PauliString<W>::operator!=(const PauliString<W> &other) const {
    return ref() != other.ref();
}

template <size_t W>
bool PauliString<W>::operator<(const PauliString<W> &other) const {
    return ref() < other.ref();
}

template <size_t W>
bool PauliString<W>::operator<(const PauliStringRef<W> &other) const {
    return ref() < other;
}

template <size_t W>
void PauliString<W>::ensure_num_qubits(size_t min_num_qubits, double resize_pad_factor) {
    assert(resize_pad_factor >= 1);
    if (min_num_qubits <= num_qubits) {
        return;
    }
    if (xs.num_bits_padded() >= min_num_qubits) {
        num_qubits = min_num_qubits;
        return;
    }

    size_t new_num_qubits = (size_t)(min_num_qubits * resize_pad_factor);
    simd_bits<W> new_xs(new_num_qubits);
    simd_bits<W> new_zs(new_num_qubits);
    new_xs.truncated_overwrite_from(xs, num_qubits);
    new_zs.truncated_overwrite_from(zs, num_qubits);
    xs = std::move(new_xs);
    zs = std::move(new_zs);
    num_qubits = min_num_qubits;
}

template <size_t W>
void PauliString<W>::mul_pauli_term(GateTarget t, bool *imag, bool right_mul) {
    auto q = t.qubit_value();
    ensure_num_qubits(q + 1, 1.25);
    bool x2 = (bool)(t.data & TARGET_PAULI_X_BIT);
    bool z2 = (bool)(t.data & TARGET_PAULI_Z_BIT);
    if (!(x2 | z2)) {
        throw std::invalid_argument("Not a pauli target: " + t.str());
    }

    bit_ref x1 = xs[q];
    bit_ref z1 = zs[q];
    bool old_x1 = x1;
    bool old_z1 = z1;
    x1 ^= x2;
    z1 ^= z2;

    // At each bit position: accumulate anti-commutation (+i or -i) counts.
    bool x1z2 = x1 & z2;
    bool anti_commutes = (x2 & z1) ^ x1z2;
    sign ^= (*imag ^ old_x1 ^ old_z1 ^ x1z2) & anti_commutes;
    sign ^= (bool)(t.data & TARGET_INVERTED_BIT);
    *imag ^= anti_commutes;
    sign ^= right_mul && anti_commutes;
}

template <size_t W>
void PauliString<W>::left_mul_pauli(GateTarget t, bool *imag) {
    mul_pauli_term(t, imag, false);
}

template <size_t W>
void PauliString<W>::right_mul_pauli(GateTarget t, bool *imag) {
    mul_pauli_term(t, imag, true);
}

template <size_t W>
uint8_t PauliString<W>::py_get_item(int64_t index) const {
    if (index < 0) {
        index += num_qubits;
    }
    if (index < 0 || (size_t)index >= num_qubits) {
        throw std::out_of_range("index");
    }
    size_t u = (size_t)index;
    int x = xs[u];
    int z = zs[u];
    return pauli_xz_to_xyz(x, z);
}

template <size_t W>
PauliString<W> PauliString<W>::py_get_slice(int64_t start, int64_t step, int64_t slice_length) const {
    assert(slice_length >= 0);
    assert(slice_length == 0 || start >= 0);
    return PauliString::from_func(false, slice_length, [&](size_t i) {
        int j = start + i * step;
        return "_XZY"[xs[j] + zs[j] * 2];
    });
}

template <size_t W>
std::ostream &operator<<(std::ostream &out, const PauliString<W> &ps) {
    return out << ps.ref();
}

}  // namespace stim
