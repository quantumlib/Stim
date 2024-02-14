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

#include <cstring>
#include <sstream>

#include "stim/mem/simd_util.h"

namespace stim {

template <size_t W>
simd_bits_range_ref<W>::simd_bits_range_ref(bitword<W> *ptr_simd_init, size_t num_simd_words)
    : ptr_simd(ptr_simd_init), num_simd_words(num_simd_words) {
}

template <size_t W>
simd_bits_range_ref<W> simd_bits_range_ref<W>::operator^=(const simd_bits_range_ref<W> other) {
    for_each_word(other, [](bitword<W> &w0, bitword<W> &w1) {
        w0 ^= w1;
    });
    return *this;
}

template <size_t W>
simd_bits_range_ref<W> simd_bits_range_ref<W>::operator|=(const simd_bits_range_ref<W> other) {
    for_each_word(other, [](bitword<W> &w0, bitword<W> &w1) {
        w0 |= w1;
    });
    return *this;
}

template <size_t W>
simd_bits_range_ref<W> simd_bits_range_ref<W>::operator&=(const simd_bits_range_ref<W> other) {
    for_each_word(other, [](bitword<W> &w0, bitword<W> &w1) {
        w0 &= w1;
    });
    return *this;
}

template <size_t W>
simd_bits_range_ref<W> simd_bits_range_ref<W>::operator=(const simd_bits_range_ref<W> other) {
    memcpy(ptr_simd, other.ptr_simd, num_u8_padded());
    return *this;
}

template <size_t W>
simd_bits_range_ref<W> simd_bits_range_ref<W>::operator+=(const simd_bits_range_ref<W> other) {
    size_t num_u64 = num_u64_padded();
    uint64_t carry{0};
    for (size_t w = 0; w < num_u64; w++) {
        uint64_t val_before = u64[w];
        u64[w] += other.u64[w] + carry;
        carry = u64[w] < val_before || (carry & (val_before == u64[w]));
    }
    return *this;
}

template <size_t W>
simd_bits_range_ref<W> simd_bits_range_ref<W>::operator-=(const simd_bits_range_ref<W> other) {
    invert_bits();
    *this += other;
    invert_bits();
    return *this;
}

template <size_t W>
simd_bits_range_ref<W> simd_bits_range_ref<W>::operator>>=(int offset) {
    uint64_t incoming_word;
    uint64_t cur_word;
    if (offset == 0) {
        return *this;
    }
    while (offset >= 64) {
        incoming_word = 0ULL;
        for (int w = num_u64_padded() - 1; w >= 0; w--) {
            cur_word = u64[w];
            u64[w] = incoming_word;
            incoming_word = cur_word;
        }
        offset -= 64;
    }
    if (offset == 0) {
        return *this;
    }
    incoming_word = 0ULL;
    for (int w = num_u64_padded() - 1; w >= 0; w--) {
        cur_word = u64[w];
        u64[w] >>= offset;
        u64[w] |= incoming_word << (64 - offset);
        incoming_word = cur_word & ((uint64_t{1} << offset) - 1);
    }
    return *this;
}

template <size_t W>
simd_bits_range_ref<W> simd_bits_range_ref<W>::operator<<=(int offset) {
    uint64_t incoming_word;
    uint64_t cur_word;
    if (offset == 0) {
        return *this;
    }
    while (offset >= 64) {
        incoming_word = 0ULL;
        for (size_t w = 0; w < num_u64_padded(); w++) {
            cur_word = u64[w];
            u64[w] = incoming_word;
            incoming_word = cur_word;
        }
        offset -= 64;
    }
    if (offset == 0) {
        return *this;
    }
    incoming_word = 0ULL;
    for (size_t w = 0; w < num_u64_padded(); w++) {
        cur_word = u64[w];
        u64[w] <<= offset;
        u64[w] |= incoming_word;
        incoming_word = (cur_word >> (64 - offset));
    }
    return *this;
}

template <size_t W>
void simd_bits_range_ref<W>::swap_with(simd_bits_range_ref<W> other) {
    for_each_word(other, [](bitword<W> &w0, bitword<W> &w1) {
        std::swap(w0, w1);
    });
}

template <size_t W>
void simd_bits_range_ref<W>::invert_bits() {
    auto mask = bitword<W>::tile8(0xFF);
    for_each_word([&mask](bitword<W> &w) {
        w ^= mask;
    });
}

template <size_t W>
void simd_bits_range_ref<W>::clear() {
    memset(u8, 0, num_u8_padded());
}

template <size_t W>
bool simd_bits_range_ref<W>::operator==(const simd_bits_range_ref<W> &other) const {
    return num_simd_words == other.num_simd_words && memcmp(ptr_simd, other.ptr_simd, num_u8_padded()) == 0;
}

template <size_t W>
bool simd_bits_range_ref<W>::not_zero() const {
    bitword<W> acc{};
    for_each_word([&acc](bitword<W> &w) {
        acc |= w;
    });
    return (bool)acc;
}

template <size_t W>
bool simd_bits_range_ref<W>::operator!=(const simd_bits_range_ref<W> &other) const {
    return !(*this == other);
}

template <size_t W>
std::ostream &operator<<(std::ostream &out, const simd_bits_range_ref<W> m) {
    for (size_t k = 0; k < m.num_bits_padded(); k++) {
        out << "_1"[m[k]];
    }
    return out;
}

template <size_t W>
std::string simd_bits_range_ref<W>::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

template <size_t W>
void simd_bits_range_ref<W>::randomize(size_t num_bits, std::mt19937_64 &rng) {
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

template <size_t W>
void simd_bits_range_ref<W>::truncated_overwrite_from(simd_bits_range_ref<W> other, size_t num_bits) {
    size_t n8 = num_bits >> 3;
    memcpy(u8, other.u8, n8);
    if (num_bits & 7) {
        uint8_t m8 = uint8_t{0xFF} >> (8 - (num_bits & 7));
        u8[n8] &= ~m8;
        u8[n8] |= other.u8[n8] & m8;
    }
}

template <size_t W>
size_t simd_bits_range_ref<W>::popcnt() const {
    auto end = u64 + num_u64_padded();
    size_t result = 0;
    for (const uint64_t *p = u64; p != end; p++) {
        result += std::popcount(*p);
    }
    return result;
}

template <size_t W>
size_t simd_bits_range_ref<W>::countr_zero() const {
    size_t n = num_u64_padded();
    for (size_t k = 0; k < n; k++) {
        uint64_t u = u64[k];
        if (u) {
            for (size_t r = 0; r < 64; r++) {
                if ((u >> r) & 1) {
                    return r + 64 * k;
                }
            }
        }
    }
    return SIZE_MAX;
}

template <size_t W>
bool simd_bits_range_ref<W>::intersects(const simd_bits_range_ref<W> other) const {
    size_t n = std::min(num_u64_padded(), other.num_u64_padded());
    uint64_t v = 0;
    for (size_t k = 0; k < n; k++) {
        v |= u64[k] & other.u64[k];
    }
    return v != 0;
}

template <size_t W>
uint64_t simd_bits_range_ref<W>::as_u64() const {
    size_t n64 = num_u64_padded();
    for (size_t k = 1; k < n64; k++) {
        if (u64[k]) {
            throw std::invalid_argument("Too large to fit into a uint64_t.");
        }
    }
    size_t n1 = std::min(num_bits_padded(), (size_t)64);
    uint64_t v = 0;
    for (size_t k = 0; k < n1; k++) {
        v ^= uint64_t{(*this)[k]} << k;
    }
    return v;
}

}  // namespace stim
