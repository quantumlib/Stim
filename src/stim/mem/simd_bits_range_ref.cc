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

#include "stim/mem/simd_bits_range_ref.h"

#include <cstring>
#include <sstream>

#include "stim/mem/simd_util.h"

using namespace stim;

simd_bits_range_ref::simd_bits_range_ref(simd_word *ptr_simd_init, size_t num_simd_words)
    : ptr_simd(ptr_simd_init), num_simd_words(num_simd_words) {
}

simd_bits_range_ref simd_bits_range_ref::operator^=(const simd_bits_range_ref other) {
    for_each_word(other, [](simd_word &w0, simd_word &w1) {
        w0 ^= w1;
    });
    return *this;
}

simd_bits_range_ref simd_bits_range_ref::operator|=(const simd_bits_range_ref other) {
    for_each_word(other, [](simd_word &w0, simd_word &w1) {
        w0 |= w1;
    });
    return *this;
}

simd_bits_range_ref simd_bits_range_ref::operator&=(const simd_bits_range_ref other) {
    for_each_word(other, [](simd_word &w0, simd_word &w1) {
        w0 &= w1;
    });
    return *this;
}

simd_bits_range_ref simd_bits_range_ref::operator=(const simd_bits_range_ref other) {
    memcpy(ptr_simd, other.ptr_simd, num_u8_padded());
    return *this;
}

void simd_bits_range_ref::swap_with(simd_bits_range_ref other) {
    for_each_word(other, [](simd_word &w0, simd_word &w1) {
        std::swap(w0, w1);
    });
}

void simd_bits_range_ref::invert_bits() {
    auto mask = simd_word::tile8(0xFF);
    for_each_word([&mask](simd_word &w) {
        w ^= mask;
    });
}

void simd_bits_range_ref::clear() {
    memset(u8, 0, num_u8_padded());
}

bool simd_bits_range_ref::operator==(const simd_bits_range_ref other) const {
    return num_simd_words == other.num_simd_words && memcmp(ptr_simd, other.ptr_simd, num_u8_padded()) == 0;
}

bool simd_bits_range_ref::not_zero() const {
    simd_word acc{};
    for_each_word([&acc](simd_word &w) {
        acc |= w;
    });
    return (bool)acc;
}

bool simd_bits_range_ref::operator!=(const simd_bits_range_ref other) const {
    return !(*this == other);
}

std::ostream &stim::operator<<(std::ostream &out, const simd_bits_range_ref m) {
    for (size_t k = 0; k < m.num_bits_padded(); k++) {
        out << "_1"[m[k]];
    }
    return out;
}

std::string simd_bits_range_ref::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

void simd_bits_range_ref::randomize(size_t num_bits, std::mt19937_64 &rng) {
    auto n = num_bits >> 6;
    for (size_t k = 0; k < n; k++) {
        u64[k] = rng();
    }
    auto leftover = num_bits & 63;
    if (leftover) {
        uint64_t mask = ((uint64_t)1 << leftover) - 1;
        u64[n] &= ~mask;
        u64[n] |= rng() & mask;
    }
}

void simd_bits_range_ref::truncated_overwrite_from(simd_bits_range_ref other, size_t num_bits) {
    size_t n8 = num_bits >> 3;
    memcpy(u8, other.u8, n8);
    if (num_bits & 7) {
        uint8_t m8 = uint8_t{0xFF} >> (8 - (num_bits & 7));
        u8[n8] &= ~m8;
        u8[n8] |= other.u8[n8] & m8;
    }
}
size_t simd_bits_range_ref::popcnt() const {
    auto end = u64 + num_u64_padded();
    size_t result = 0;
    for (const uint64_t *p = u64; p != end; p++) {
        result += popcnt64(*p);
    }
    return result;
}
bool simd_bits_range_ref::intersects(const simd_bits_range_ref other) const {
    size_t n = std::min(num_u64_padded(), other.num_u64_padded());
    uint64_t v = 0;
    for (size_t k = 0; k < n; k++) {
        v |= u64[k] & other.u64[k];
    }
    return v != 0;
}
