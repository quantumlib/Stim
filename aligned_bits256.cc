#include <algorithm>
#include <cstring>
#include <immintrin.h>

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
