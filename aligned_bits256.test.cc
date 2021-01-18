#include "gtest/gtest.h"
#include "aligned_bits256.h"
#include "simd_util.h"

TEST(aligned_bits256, move) {
    auto a = aligned_bits256(512);
    auto ptr = a.u64;
    aligned_bits256 b = std::move(a);
    ASSERT_EQ(b.u64, ptr);
}

TEST(aligned_bits256, small_copy) {
    auto a = aligned_bits256(3);
    a.u64[0] = 1;
    auto b = a;
    ASSERT_EQ(b.u64[0], 1);
}

TEST(aligned_bits256, clear) {
    auto a = aligned_bits256::random(1001);
    ASSERT_TRUE(any_non_zero(a.u256, 4));
    a.clear();
    ASSERT_FALSE(any_non_zero(a.u256, 4));
}
