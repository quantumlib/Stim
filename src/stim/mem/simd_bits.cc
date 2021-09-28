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

#include "stim/mem/simd_bits.h"

#include <algorithm>
#include <cstring>
#include <random>
#include <sstream>

#include "stim/mem/simd_util.h"

using namespace stim;

uint64_t *malloc_aligned_padded_zeroed(size_t min_bits) {
    size_t num_u8 = min_bits_to_num_bits_padded(min_bits) >> 3;
    void *result = simd_word::aligned_malloc(num_u8);
    memset(result, 0, num_u8);
    return (uint64_t *)result;
}

simd_bits::simd_bits(size_t min_bits)
    : num_simd_words(min_bits_to_num_simd_words(min_bits)), u64(malloc_aligned_padded_zeroed(min_bits)) {
}

simd_bits::simd_bits(const simd_bits &other)
    : num_simd_words(other.num_simd_words), u64(malloc_aligned_padded_zeroed(other.num_bits_padded())) {
    memcpy(u8, other.u8, num_u8_padded());
}

simd_bits::simd_bits(const simd_bits_range_ref other)
    : num_simd_words(other.num_simd_words), u64(malloc_aligned_padded_zeroed(other.num_bits_padded())) {
    memcpy(u8, other.u8, num_u8_padded());
}

simd_bits::simd_bits(simd_bits &&other) noexcept : num_simd_words(other.num_simd_words), u64(other.u64) {
    other.u64 = nullptr;
    other.num_simd_words = 0;
}

simd_bits::~simd_bits() {
    if (u64 != nullptr) {
        simd_word::aligned_free(u64);
        u64 = nullptr;
        num_simd_words = 0;
    }
}

void simd_bits::clear() {
    simd_bits_range_ref(*this).clear();
}

void simd_bits::invert_bits() {
    simd_bits_range_ref(*this).invert_bits();
}

simd_bits &simd_bits::operator=(simd_bits &&other) noexcept {
    (*this).~simd_bits();
    new (this) simd_bits(std::move(other));
    return *this;
}

simd_bits &simd_bits::operator=(const simd_bits &other) {
    *this = simd_bits_range_ref(other);
    return *this;
}

simd_bits &simd_bits::operator=(const simd_bits_range_ref other) {
    // Avoid re-allocating if already the same size.
    if (num_simd_words == other.num_simd_words) {
        simd_bits_range_ref(*this) = other;
        return *this;
    }

    (*this).~simd_bits();
    new (this) simd_bits(other);
    return *this;
}

bool simd_bits::operator==(const simd_bits_range_ref &other) const {
    return simd_bits_range_ref(*this) == other;
}

bool simd_bits::operator!=(const simd_bits_range_ref &other) const {
    return !(*this == other);
}

simd_bits simd_bits::random(size_t min_bits, std::mt19937_64 &rng) {
    simd_bits result(min_bits);
    result.randomize(min_bits, rng);
    return result;
}

void simd_bits::randomize(size_t num_bits, std::mt19937_64 &rng) {
    simd_bits_range_ref(*this).randomize(num_bits, rng);
}

void simd_bits::truncated_overwrite_from(simd_bits_range_ref other, size_t num_bits) {
    simd_bits_range_ref(*this).truncated_overwrite_from(other, num_bits);
}

bit_ref simd_bits::operator[](size_t k) {
    return bit_ref(u64, k);
}

const bit_ref simd_bits::operator[](size_t k) const {
    return bit_ref(u64, k);
}

simd_bits::operator simd_bits_range_ref() {
    return simd_bits_range_ref(ptr_simd, num_simd_words);
}

simd_bits::operator const simd_bits_range_ref() const {
    return simd_bits_range_ref(ptr_simd, num_simd_words);
}

simd_bits &simd_bits::operator^=(const simd_bits_range_ref other) {
    simd_bits_range_ref(*this) ^= other;
    return *this;
}

simd_bits &simd_bits::operator&=(const simd_bits_range_ref other) {
    simd_bits_range_ref(*this) &= other;
    return *this;
}

simd_bits &simd_bits::operator|=(const simd_bits_range_ref other) {
    simd_bits_range_ref(*this) |= other;
    return *this;
}

bool simd_bits::not_zero() const {
    return simd_bits_range_ref(*this).not_zero();
}

bool simd_bits::intersects(const simd_bits_range_ref other) const {
    return simd_bits_range_ref(*this).intersects(other);
}

std::string simd_bits::str() const {
    return simd_bits_range_ref(*this).str();
}

simd_bits &simd_bits::swap_with(simd_bits_range_ref other) {
    simd_bits_range_ref(*this).swap_with(other);
    return *this;
}

size_t simd_bits::popcnt() const {
    return simd_bits_range_ref(*this).popcnt();
}
