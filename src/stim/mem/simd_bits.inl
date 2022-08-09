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

#include <algorithm>
#include <cstring>
#include <random>
#include <sstream>

#include "stim/mem/simd_util.h"

namespace stim {

template <size_t W>
uint64_t *malloc_aligned_padded_zeroed(size_t min_bits) {
    size_t num_u8 = min_bits_to_num_bits_padded<W>(min_bits) >> 3;
    void *result = bitword<W>::aligned_malloc(num_u8);
    memset(result, 0, num_u8);
    return (uint64_t *)result;
}

template <size_t W>
simd_bits<W>::simd_bits(size_t min_bits)
    : num_simd_words(min_bits_to_num_simd_words<W>(min_bits)), u64(malloc_aligned_padded_zeroed<W>(min_bits)) {
}

template <size_t W>
simd_bits<W>::simd_bits(const simd_bits<W> &other)
    : num_simd_words(other.num_simd_words), u64(malloc_aligned_padded_zeroed<W>(other.num_bits_padded())) {
    memcpy(u8, other.u8, num_u8_padded());
}

template <size_t W>
simd_bits<W>::simd_bits(const simd_bits_range_ref<W> other)
    : num_simd_words(other.num_simd_words), u64(malloc_aligned_padded_zeroed<W>(other.num_bits_padded())) {
    memcpy(u8, other.u8, num_u8_padded());
}

template <size_t W>
simd_bits<W>::simd_bits(simd_bits<W> &&other) noexcept : num_simd_words(other.num_simd_words), u64(other.u64) {
    other.u64 = nullptr;
    other.num_simd_words = 0;
}

template <size_t W>
simd_bits<W>::~simd_bits() {
    if (u64 != nullptr) {
        bitword<W>::aligned_free(u64);
        u64 = nullptr;
        num_simd_words = 0;
    }
}

template <size_t W>
void simd_bits<W>::clear() {
    simd_bits_range_ref<W>(*this).clear();
}

template <size_t W>
void simd_bits<W>::invert_bits() {
    simd_bits_range_ref<W>(*this).invert_bits();
}

template <size_t W>
simd_bits<W> &simd_bits<W>::operator=(simd_bits<W> &&other) noexcept {
    (*this).~simd_bits();
    new (this) simd_bits(std::move(other));
    return *this;
}

template <size_t W>
simd_bits<W> &simd_bits<W>::operator=(const simd_bits<W> &other) {
    *this = simd_bits_range_ref<W>(other);
    return *this;
}

template <size_t W>
simd_bits<W> &simd_bits<W>::operator=(const simd_bits_range_ref<W> other) {
    // Avoid re-allocating if already the same size.
    if (num_simd_words == other.num_simd_words) {
        simd_bits_range_ref<W>(*this) = other;
        return *this;
    }

    (*this).~simd_bits();
    new (this) simd_bits(other);
    return *this;
}

template <size_t W>
bool simd_bits<W>::operator==(const simd_bits_range_ref<W> &other) const {
    return simd_bits_range_ref<W>(*this) == other;
}

template <size_t W>
bool simd_bits<W>::operator!=(const simd_bits_range_ref<W> &other) const {
    return !(*this == other);
}

template <size_t W>
simd_bits<W> simd_bits<W>::random(size_t min_bits, std::mt19937_64 &rng) {
    simd_bits<W> result(min_bits);
    result.randomize(min_bits, rng);
    return result;
}

template <size_t W>
void simd_bits<W>::randomize(size_t num_bits, std::mt19937_64 &rng) {
    simd_bits_range_ref<W>(*this).randomize(num_bits, rng);
}

template <size_t W>
void simd_bits<W>::truncated_overwrite_from(simd_bits_range_ref<W> other, size_t num_bits) {
    simd_bits_range_ref<W>(*this).truncated_overwrite_from(other, num_bits);
}

template <size_t W>
bit_ref simd_bits<W>::operator[](size_t k) {
    return bit_ref(u64, k);
}

template <size_t W>
const bit_ref simd_bits<W>::operator[](size_t k) const {
    return bit_ref(u64, k);
}

template <size_t W>
simd_bits<W>::operator simd_bits_range_ref<W>() {
    return simd_bits_range_ref<W>(ptr_simd, num_simd_words);
}

template <size_t W>
simd_bits<W>::operator const simd_bits_range_ref<W>() const {
    return simd_bits_range_ref<W>(ptr_simd, num_simd_words);
}

template <size_t W>
simd_bits<W> &simd_bits<W>::operator^=(const simd_bits_range_ref<W> other) {
    simd_bits_range_ref<W>(*this) ^= other;
    return *this;
}

template <size_t W>
simd_bits<W> &simd_bits<W>::operator&=(const simd_bits_range_ref<W> other) {
    simd_bits_range_ref<W>(*this) &= other;
    return *this;
}

template <size_t W>
simd_bits<W> &simd_bits<W>::operator|=(const simd_bits_range_ref<W> other) {
    simd_bits_range_ref<W>(*this) |= other;
    return *this;
}

template <size_t W>
bool simd_bits<W>::not_zero() const {
    return simd_bits_range_ref<W>(*this).not_zero();
}

template <size_t W>
bool simd_bits<W>::intersects(const simd_bits_range_ref<W> other) const {
    return simd_bits_range_ref<W>(*this).intersects(other);
}

template <size_t W>
std::string simd_bits<W>::str() const {
    return simd_bits_range_ref<W>(*this).str();
}

template <size_t W>
simd_bits<W> &simd_bits<W>::swap_with(simd_bits_range_ref<W> other) {
    simd_bits_range_ref<W>(*this).swap_with(other);
    return *this;
}

template <size_t W>
size_t simd_bits<W>::popcnt() const {
    return simd_bits_range_ref<W>(*this).popcnt();
}

}
