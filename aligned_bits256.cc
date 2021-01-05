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
    if (data != nullptr) {
        _mm_free(data);
        data = nullptr;
        num_bits = 0;
    }
}

aligned_bits256::aligned_bits256(size_t init_num_bits) :
    num_bits(init_num_bits),
    data(alloc_aligned_bits(init_num_bits)) {
}

aligned_bits256::aligned_bits256(size_t num_bits, const uint64_t *other) :
        num_bits(num_bits),
        data(alloc_aligned_bits(num_bits)) {
    memcpy(data, other, num_bytes(num_bits));
}

aligned_bits256::aligned_bits256(aligned_bits256&& other) noexcept :
        data(other.data),
        num_bits(other.num_bits) {
    other.data = nullptr;
    other.num_bits = 0;
}

aligned_bits256::aligned_bits256(const aligned_bits256& other) :
        num_bits(other.num_bits),
        data(alloc_aligned_bits(other.num_bits)) {
    memcpy(data, other.data, num_bytes(num_bits));
}

aligned_bits256& aligned_bits256::operator=(aligned_bits256&& other) noexcept {
    (*this).~aligned_bits256();
    new(this) aligned_bits256(std::move(other));
    return *this;
}

aligned_bits256& aligned_bits256::operator=(const aligned_bits256& other) {
    // Avoid re-allocating if already the same size.
    if (this->num_bits == other.num_bits) {
        memcpy(data, other.data, num_bytes(num_bits));
        return *this;
    }

    (*this).~aligned_bits256();
    new(this) aligned_bits256(other);
    return *this;
}

bool aligned_bits256::get_bit(size_t k) const {
    return ((data[k >> 6] >> (k & 63)) & 1) != 0;
}

void aligned_bits256::set_bit(size_t k, bool value) {
    if (value) {
        data[k >> 6] |= (uint64_t)1 << (k & 63);
    } else {
        data[k >> 6] &= ~((uint64_t)1 << (k & 63));
    }
}

bool aligned_bits256::operator==(const aligned_bits256 &other) const {
    if (num_bits != other.num_bits) {
        return false;
    }
    __m256i acc = _mm256_set1_epi32(-1);
    auto a = (__m256i *)data;
    auto b = (__m256i *)other.data;
    size_t n = num_bits >> 8;
    for (size_t i = 0; i < n; i++) {
        acc = _mm256_andnot_si256(_mm256_xor_si256(a[i], b[i]), acc);
    }
    auto acc64 = (uint64_t *)&acc;
    for (size_t k = 0; k < 4; k++) {
        if (acc64[k] != UINT64_MAX) {
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
        data.data[k] = dis(gen);
    }
    if (num_bits & 63) {
      data.data[num_u64 - 1] &= (1 << (num_bits & 63)) - 1;
    }
    return data;
}
