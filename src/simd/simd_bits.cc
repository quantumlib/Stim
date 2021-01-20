#include <algorithm>
#include <cstring>
#include <immintrin.h>
#include <random>

#include "simd_bits.h"
#include "simd_util.h"

size_t num_bytes(size_t num_bits) {
    return ((num_bits + 255) / 256) * sizeof(__m256i);
}

uint64_t *alloc_aligned_bits(size_t num_bits) {
    auto bytes = num_bytes(num_bits);
    auto result = (uint64_t *)_mm_malloc(bytes, 32);
    memset(result, 0, bytes);
    return result;
}

simd_bits::~simd_bits() {
    if (u64 != nullptr) {
        _mm_free(u64);
        u64 = nullptr;
        num_bits = 0;
    }
}

void simd_bits::clear() {
    memset(u64, 0, num_bits >> 3);
}

simd_bits::simd_bits(size_t init_num_bits) :
        num_bits(init_num_bits),
        u64(alloc_aligned_bits(init_num_bits)) {
}

simd_bits::simd_bits(size_t num_bits, const void *other) :
        num_bits(num_bits),
        u64(alloc_aligned_bits(num_bits)) {
    memcpy(u64, other, num_bytes(num_bits));
}

simd_bits::simd_bits(simd_bits&& other) noexcept :
        num_bits(other.num_bits),
        u64(other.u64) {
    other.u64 = nullptr;
    other.num_bits = 0;
}

simd_bits::simd_bits(const simd_bits& other) :
        num_bits(other.num_bits),
        u64(alloc_aligned_bits(other.num_bits)) {
    memcpy(u64, other.u64, num_bytes(num_bits));
}

simd_bits::simd_bits(const simd_range_ref& other) :
        num_bits(other.count << 8),
        u64(alloc_aligned_bits(other.count << 8)) {
    memcpy(u64, other.start, num_bytes(num_bits));
}

simd_bits& simd_bits::operator=(simd_bits&& other) noexcept {
    (*this).~simd_bits();
    new(this) simd_bits(std::move(other));
    return *this;
}

simd_bits& simd_bits::operator=(const simd_bits& other) {
    // Avoid re-allocating if already the same size.
    if (this->num_bits == other.num_bits) {
        memcpy(u64, other.u64, num_bytes(num_bits));
        return *this;
    }

    (*this).~simd_bits();
    new(this) simd_bits(other);
    return *this;
}

bool simd_bits::operator==(const simd_bits &other) const {
    if (num_bits != other.num_bits) {
        return false;
    }
    return memcmp(u64, other.u64, (num_bits + 7) >> 3) == 0;
}

bool simd_bits::operator!=(const simd_bits &other) const {
    return !(*this == other);
}

simd_bits simd_bits::random(size_t num_bits) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis(
            std::numeric_limits<std::uint64_t>::min(),
            std::numeric_limits<std::uint64_t>::max());
    size_t num_u64 = (num_bits + 63) / 64;
    auto data = simd_bits(num_bits);
    for (size_t k = 0; k < num_u64; k++) {
        data.u64[k] = dis(gen);
    }
    if (num_bits & 63) {
      data.u64[num_u64 - 1] &= (1 << (num_bits & 63)) - 1;
    }
    return data;
}

BitRef simd_bits::operator[](size_t k) {
    return BitRef(u64, k);
}

bool simd_bits::operator[](size_t k) const {
    return (bool)BitRef(u64, k);
}

simd_range_ref simd_bits::range_ref() {
    return simd_range_ref {u256, ceil256(num_bits) >> 8};
}

const simd_range_ref simd_bits::range_ref() const {
    return simd_range_ref {u256, ceil256(num_bits) >> 8};
}

simd_range_ref simd_bits::word_range_ref(size_t word_offset, size_t word_count) {
    return simd_range_ref {u256 + word_offset, word_count};
}

const simd_range_ref simd_bits::word_range_ref(size_t word_offset, size_t word_count) const {
    return simd_range_ref {u256 + word_offset, word_count};
}
