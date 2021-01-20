#include <cstring>
#include "simd_range.h"
#include "simd_util.h"

SimdRange &SimdRange::operator^=(const SimdRange &other) {
    return *this ^= other.start;
}

SimdRange &SimdRange::operator^=(const __m256i *other) {
    mem_xor256(start, other, count);
    return *this;
}

void SimdRange::overwrite_with(const SimdRange &other) {
    overwrite_with(other.start);
}

void SimdRange::overwrite_with(const __m256i *other) {
    memcpy(start, other, count << 5);
}

void SimdRange::clear() {
    memset(start, 0, count << 5);
}

void SimdRange::swap_with(SimdRange other) {
    swap_with(other.start);
}

void SimdRange::swap_with(__m256i *other) {
    mem_swap256(start, other, count);
}

BitRef SimdRange::operator[](size_t k) {
    return BitRef(start, k);
}

bool SimdRange::operator[](size_t k) const {
    return (bool)BitRef(start, k);
}
