#ifndef SIMD_UTIL_H
#define SIMD_UTIL_H

#include <immintrin.h>
#include <vector>
#include <iostream>

/// For each 2 bit subgroup, performs x -> popcnt(x).
__m256i popcnt2(__m256i r);

/// For each 4 bit subgroup, performs x -> popcnt(x).
__m256i popcnt4(__m256i r);

/// For each 8 bit subgroup, performs x -> popcnt(x).
__m256i popcnt8(__m256i r);

/// For each 16 bit subgroup, performs x -> popcnt(x).
__m256i popcnt16(__m256i r);

__m256i bits_to_m256i(std::vector<bool> data);

std::vector<bool> m256i_to_bits(__m256i data);

std::ostream &operator<<(std::ostream &out, __m256i data);

std::string hex256(__m256i data);

#endif
