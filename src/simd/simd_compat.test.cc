#include <gtest/gtest.h>
#include "simd_compat.h"
//
//TEST(simd_compat, popcount) {
//    uint64_t u64[4] {0, 0b01000010, 0b11111, UINT64_MAX};
//    auto u128 = (__m128i *)u64;
//    auto u256 = (__m256i *)u64;
//
//    ASSERT_EQ(popcount(u64[0]), 0);
//    ASSERT_EQ(popcount(u64[1]), 2);
//    ASSERT_EQ(popcount(u64[2]), 5);
//    ASSERT_EQ(popcount(u64[3]), 64);
//
//    ASSERT_EQ(popcount(u128[0]), 2);
//    ASSERT_EQ(popcount(u128[1]), 69);
//
//    ASSERT_EQ(popcount(u256[0]), 71);
//}
