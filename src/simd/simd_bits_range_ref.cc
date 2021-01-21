#include <cstring>
#include <sstream>
#include "simd_bits_range_ref.h"
#include "simd_util.h"

simd_bits_range_ref::simd_bits_range_ref(SIMD_WORD_TYPE *ptr_simd_init, size_t num_simd_words) :
        ptr_simd(ptr_simd_init), num_simd_words(num_simd_words) {
}

simd_bits_range_ref simd_bits_range_ref::operator^=(const simd_bits_range_ref other) {
    for_each_word(other, [](auto &w0, auto &w1) {
        w0 ^= w1;
    });
    return *this;
}

simd_bits_range_ref simd_bits_range_ref::operator=(const simd_bits_range_ref other) {
    memcpy(ptr_simd, other.ptr_simd, num_u8_padded());
    return *this;
}

void simd_bits_range_ref::swap_with(simd_bits_range_ref other) {
    for_each_word(other, [](auto &w0, auto &w1) {
        std::swap(w0, w1);
    });
}

void simd_bits_range_ref::clear() {
    memset(ptr_simd, 0, num_u8_padded());
}

bool simd_bits_range_ref::operator==(const simd_bits_range_ref other) const {
    return num_simd_words == other.num_simd_words && memcmp(ptr_simd, other.ptr_simd, num_u8_padded()) == 0;
}

bool simd_bits_range_ref::not_zero() const {
    SIMD_WORD_TYPE acc {};
    for_each_word([&acc](auto &w) {
        acc |= w;
    });
    return acc;
}

bool simd_bits_range_ref::operator!=(const simd_bits_range_ref other) const {
    return !(*this == other);
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
