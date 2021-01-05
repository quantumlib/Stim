#include "gtest/gtest.h"
#include "aligned_bits256.h"

TEST(aligned_bits256, move) {
    auto a = aligned_bits256(512);
    auto ptr = a.data;
    aligned_bits256 b = std::move(a);
    ASSERT_EQ(b.data, ptr);
}

TEST(aligned_bits256, small_copy) {
    auto a = aligned_bits256(3);
    a.data[0] = 1;
    auto b = a;
    ASSERT_EQ(b.data[0], 1);
}
