#include <immintrin.h>
#include <vector>

/// For each 2 bit subgroup, performs x -> popcnt(x).
__m256i popcnt2(__m256i r);

/// For each 4 bit subgroup, performs x -> popcnt(x).
__m256i popcnt4(__m256i r);

/// For each 8 bit subgroup, performs x -> popcnt(x).
__m256i popcnt8(__m256i r);

/// For each 16 bit subgroup, performs x -> popcnt(x).
__m256i popcnt16(__m256i r);
