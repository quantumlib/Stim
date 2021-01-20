#include <cstring>
#include "simd_range.h"
#include "simd_util.h"

simd_range_ref &simd_range_ref::operator^=(const simd_range_ref &other) {
    mem_xor256(start, other.start, count);
    return *this;
}

simd_range_ref &simd_range_ref::operator^=(const __m256i *other) {
    mem_xor256(start, other, count);
    return *this;
}

simd_range_ref SimdRange::operator*() {
    return simd_range_ref {start, count};
}

const simd_range_ref SimdRange::operator*() const {
    return simd_range_ref {start, count};
}

simd_range_ref &simd_range_ref::operator=(const simd_range_ref &other) {
    memcpy(start, other.start, count << 5);
    return *this;
}

simd_range_ref &simd_range_ref::operator=(const __m256i *other) {
    memcpy(start, other, count << 5);
    return *this;
}

void SimdRange::clear() {
    memset(start, 0, count << 5);
}

void SimdRange::swap_with(SimdRange other) {
    mem_swap256(start, other.start, count);
}

void SimdRange::swap_with(__m256i *other) {
    mem_swap256(start, other, count);
}

void simd_range_ref::swap_with(simd_range_ref &other) {
    mem_swap256(start, other.start, count);
}

void simd_range_ref::swap_with(__m256i *other) {
    mem_swap256(start, other, count);
}

void simd_range_ref::clear() {
    memset(start, 0, count << 5);
}

bit_ref SimdRange::operator[](size_t k) {
    return bit_ref(start, k);
}

bool SimdRange::operator[](size_t k) const {
    return (bool)bit_ref(start, k);
}

bool simd_range_ref::operator==(const simd_range_ref &other) const {
    return count == other.count && memcmp(start, other.start, count << 5) == 0;
}

bool simd_range_ref::not_zero() const {
    return any_non_zero(start, count);
}

bool simd_range_ref::operator!=(const simd_range_ref &other) const {
    return !(*this == other);
}

bit_ref simd_range_ref::operator[](size_t k) {
    return bit_ref(start, k);
}

bool simd_range_ref::operator[](size_t k) const {
    return (bool)bit_ref(start, k);
}

simd_range_ref simd_range_ref::word_range_ref(size_t word_offset, size_t word_count) {
    return simd_range_ref {start + word_offset, word_count};
}

const simd_range_ref simd_range_ref::word_range_ref(size_t word_offset, size_t word_count) const {
    return simd_range_ref {start + word_offset, word_count};
}
