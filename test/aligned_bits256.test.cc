#include "gtest/gtest.h"
#include "../src/aligned_bits256.h"
#include "../src/simd_util.h"

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

TEST(aligned_bits256, bit_manipulation) {
    aligned_bits256 a(1001);
    ASSERT_EQ(a.get_bit(300), false);
    a.set_bit(300, true);
    ASSERT_EQ(a.get_bit(300), true);
    a.set_bit(300, true);
    ASSERT_EQ(a.get_bit(300), true);
    a.set_bit(300, false);
    ASSERT_EQ(a.get_bit(300), false);
    a.toggle_bit_if(300, false);
    ASSERT_EQ(a.get_bit(300), false);
    a.toggle_bit_if(300, true);
    ASSERT_EQ(a.get_bit(300), true);
    a.toggle_bit_if(300, true);
    ASSERT_EQ(a.get_bit(300), false);
    a.toggle_bit_if(300, true);
    ASSERT_EQ(a.get_bit(300), true);
    a.toggle_bit_if(300, false);
    ASSERT_EQ(a.get_bit(300), true);
}
