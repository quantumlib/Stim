#include <cstring>
#include <sstream>
#include "simd_bits_range_ref.h"
#include "simd_util.h"

simd_bits_range_ref::simd_bits_range_ref(__m256i *ptr, size_t num_simd_words) :
    u256(ptr), num_simd_words(num_simd_words) {
}

simd_bits_range_ref simd_bits_range_ref::operator^=(const simd_bits_range_ref other) {
    mem_xor256(u256, other.u256, num_simd_words);
    return *this;
}

simd_bits_range_ref simd_bits_range_ref::operator=(const simd_bits_range_ref other) {
    memcpy(u256, other.u256, num_u8_padded());
    return *this;
}

void simd_bits_range_ref::swap_with(simd_bits_range_ref other) {
    mem_swap256(u256, other.u256, num_simd_words);
}

void simd_bits_range_ref::clear() {
    memset(u256, 0, num_u8_padded());
}

bool simd_bits_range_ref::operator==(const simd_bits_range_ref other) const {
    return num_simd_words == other.num_simd_words && memcmp(u256, other.u256, num_u8_padded()) == 0;
}

bool simd_bits_range_ref::not_zero() const {
    __m256i acc {};
    simd_for_each(u256, num_simd_words, [&acc](auto v) {
        acc |= *v;
    });
    return not_zero256(acc);
}

bool simd_bits_range_ref::operator!=(const simd_bits_range_ref other) const {
    return !(*this == other);
}

bit_ref simd_bits_range_ref::operator[](size_t k) {
    return bit_ref(u8, k);
}

const bit_ref simd_bits_range_ref::operator[](size_t k) const {
    return bit_ref(u8, k);
}

simd_bits_range_ref simd_bits_range_ref::word_range_ref(size_t word_offset, size_t sub_num_simd_words) {
    return simd_bits_range_ref(u256 + word_offset, sub_num_simd_words);
}

const simd_bits_range_ref simd_bits_range_ref::word_range_ref(size_t word_offset, size_t sub_num_simd_words) const {
    return simd_bits_range_ref(u256 + word_offset, sub_num_simd_words);
}

std::ostream &operator<<(std::ostream &out, const simd_bits_range_ref m) {
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

void simd_bits_range_ref::randomize(size_t num_bits, std::mt19937 &rng) {
    auto n = num_bits >> 5;
    for (size_t k = 0; k < n; k++) {
        u32[k] = rng();
    }
    auto leftover = num_bits & 31;
    if (leftover) {
        uint32_t mask = ((uint32_t)1 << leftover) - 1;
        u32[n] &= ~mask;
        u32[n] |= rng() & mask;
    }
}
