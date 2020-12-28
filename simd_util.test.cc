#include "gtest/gtest.h"
#include "simd_util.h"

__m256i pack16(std::vector<uint16_t> r) {
    std::vector<uint16_t> result;
    alignas(256) uint16_t tmp[16] = {};
    for (size_t i = 0; i < r.size(); i++) {
        tmp[i] = r[i];
    }
    return _mm256_load_si256((__m256i*)tmp);
}

std::vector<uint16_t> unpack16(__m256i r) {
    std::vector<uint16_t> result;
    alignas(256) uint16_t tmp[16];
    _mm256_store_si256((__m256i*)tmp, r);
    for (auto e : tmp) {
        result.push_back(e);
    }
    return result;
}

TEST(popcnt, popcnt16) {
    ASSERT_EQ(unpack16(popcnt16(pack16({0, 1, 2, 3, 0xFFFF, 0x1111}))),
              (std::vector<uint16_t>{0, 1, 1, 2, 16, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
}
