#include <algorithm>
#include <cstring>
#include <immintrin.h>
#include <random>

#include "aligned_bits256.h"

size_t num_bytes(size_t num_bits) {
    return ((num_bits + 255) / 256) * sizeof(__m256i);
}

uint64_t *alloc_aligned_bits(size_t num_bits) {
    auto bytes = num_bytes(num_bits);
    auto result = (uint64_t *)_mm_malloc(bytes, 32);
    memset(result, 0, bytes);
    return result;
}

aligned_bits256::~aligned_bits256() {
    if (u64 != nullptr) {
        _mm_free(u64);
        u64 = nullptr;
        num_bits = 0;
    }
}

aligned_bits256::aligned_bits256(size_t init_num_bits) :
        num_bits(init_num_bits),
        u64(alloc_aligned_bits(init_num_bits)) {
}

aligned_bits256::aligned_bits256(size_t num_bits, const uint64_t *other) :
        num_bits(num_bits),
        u64(alloc_aligned_bits(num_bits)) {
    memcpy(u64, other, num_bytes(num_bits));
}

aligned_bits256::aligned_bits256(aligned_bits256&& other) noexcept :
        u64(other.u64),
        num_bits(other.num_bits) {
    other.u64 = nullptr;
    other.num_bits = 0;
}

aligned_bits256::aligned_bits256(const aligned_bits256& other) :
        num_bits(other.num_bits),
        u64(alloc_aligned_bits(other.num_bits)) {
    memcpy(u64, other.u64, num_bytes(num_bits));
}

aligned_bits256& aligned_bits256::operator=(aligned_bits256&& other) noexcept {
    (*this).~aligned_bits256();
    new(this) aligned_bits256(std::move(other));
    return *this;
}

aligned_bits256& aligned_bits256::operator=(const aligned_bits256& other) {
    // Avoid re-allocating if already the same size.
    if (this->num_bits == other.num_bits) {
        memcpy(u64, other.u64, num_bytes(num_bits));
        return *this;
    }

    (*this).~aligned_bits256();
    new(this) aligned_bits256(other);
    return *this;
}

bool aligned_bits256::get_bit(size_t k) const {
    return ((u64[k >> 6] >> (k & 63)) & 1) != 0;
}

void aligned_bits256::set_bit(size_t k, bool value) {
    if (value) {
        u64[k >> 6] |= (uint64_t)1 << (k & 63);
    } else {
        u64[k >> 6] &= ~((uint64_t)1 << (k & 63));
    }
}

bool aligned_bits256::operator==(const aligned_bits256 &other) const {
    if (num_bits != other.num_bits) {
        return false;
    }
    union {__m256i m256; uint64_t u64[4]; } acc;
    acc.m256 = _mm256_set1_epi32(-1);
    auto a = (__m256i *)u64;
    auto b = (__m256i *)other.u64;
    size_t n = (num_bits + 0xFF) >> 8;
    for (size_t i = 0; i < n; i++) {
        acc.m256 = _mm256_andnot_si256(a[i] ^ b[i], acc.m256);
    }
    for (size_t k = 0; k < 4; k++) {
        if (acc.u64[k] != UINT64_MAX) {
            return false;
        }
    }
    return true;
}

bool aligned_bits256::operator!=(const aligned_bits256 &other) const {
    return !(*this == other);
}

aligned_bits256 aligned_bits256::random(size_t num_bits) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis(
            std::numeric_limits<std::uint64_t>::min(),
            std::numeric_limits<std::uint64_t>::max());
    size_t num_u64 = (num_bits + 63) / 64;
    auto data = aligned_bits256(num_bits);
    for (size_t k = 0; k < num_u64; k++) {
        data.u64[k] = dis(gen);
    }
    if (num_bits & 63) {
      data.u64[num_u64 - 1] &= (1 << (num_bits & 63)) - 1;
    }
    return data;
}
