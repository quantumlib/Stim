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

#include "stim/stabilizers/pauli_string.h"

#include <cassert>
#include <cstring>
#include <string>

#include "stim/mem/simd_util.h"

using namespace stim;

PauliString::operator const PauliStringRef() const {
    return ref();
}

PauliString::operator PauliStringRef() {
    return ref();
}

PauliString::PauliString(size_t num_qubits) : num_qubits(num_qubits), sign(false), xs(num_qubits), zs(num_qubits) {
}

PauliString::PauliString(const PauliStringRef &other)
    : num_qubits(other.num_qubits), sign((bool)other.sign), xs(other.xs), zs(other.zs) {
}

const PauliStringRef PauliString::ref() const {
    return PauliStringRef(
        num_qubits,
        // HACK: const correctness is temporarily removed, but immediately restored.
        bit_ref((bool *)&sign, 0),
        xs,
        zs);
}

PauliStringRef PauliString::ref() {
    return PauliStringRef(num_qubits, bit_ref(&sign, 0), xs, zs);
}

std::string PauliString::str() const {
    return ref().str();
}

PauliString &PauliString::operator=(const PauliStringRef &other) noexcept {
    (*this).~PauliString();
    new (this) PauliString(other);
    return *this;
}

std::ostream &stim::operator<<(std::ostream &out, const PauliString &ps) {
    return out << ps.ref();
}

PauliString PauliString::from_func(bool sign, size_t num_qubits, const std::function<char(size_t)> &func) {
    PauliString result(num_qubits);
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
            throw std::runtime_error("Unrecognized pauli character. " + std::to_string(c));
        }
        result.xs.u64[i / 64] ^= (uint64_t)x << (i & 63);
        result.zs.u64[i / 64] ^= (uint64_t)z << (i & 63);
    }
    return result;
}

PauliString PauliString::from_str(const char *text) {
    auto sign = text[0] == '-';
    if (text[0] == '+' || text[0] == '-') {
        text++;
    }
    return PauliString::from_func(sign, strlen(text), [&](size_t i) {
        return text[i];
    });
}

PauliString PauliString::random(size_t num_qubits, std::mt19937_64 &rng) {
    auto result = PauliString(num_qubits);
    result.xs.randomize(num_qubits, rng);
    result.zs.randomize(num_qubits, rng);
    result.sign ^= rng() & 1;
    return result;
}

bool PauliString::operator==(const PauliStringRef &other) const {
    return ref() == other;
}

bool PauliString::operator!=(const PauliStringRef &other) const {
    return ref() != other;
}

void PauliString::ensure_num_qubits(size_t min_num_qubits) {
    if (min_num_qubits <= num_qubits) {
        return;
    }
    if (xs.num_bits_padded() >= min_num_qubits) {
        num_qubits = min_num_qubits;
        return;
    }

    simd_bits new_xs(min_num_qubits);
    simd_bits new_zs(min_num_qubits);
    new_xs.truncated_overwrite_from(xs, num_qubits);
    new_zs.truncated_overwrite_from(zs, num_qubits);
    xs = std::move(new_xs);
    zs = std::move(new_zs);
    num_qubits = min_num_qubits;
}

uint8_t PauliString::py_get_item(int64_t index) const {
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

PauliString PauliString::py_get_slice(int64_t start, int64_t step, int64_t slice_length) const {
    assert(start >= 0);
    return PauliString::from_func(false, slice_length, [&](size_t i) {
        int j = start + i * step;
        return "_XZY"[xs[j] + zs[j] * 2];
    });
}
