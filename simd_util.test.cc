#include "gtest/gtest.h"
#include "simd_util.h"

TEST(simd_util, hex256) {
    ASSERT_EQ(
            hex256(_mm256_set1_epi8(1)),
            ".1.1.1.1.1.1.1.1"
            " .1.1.1.1.1.1.1.1"
            " .1.1.1.1.1.1.1.1"
            " .1.1.1.1.1.1.1.1");
    ASSERT_EQ(
            hex256(_mm256_set1_epi16(1)),
            "...1...1...1...1 "
            "...1...1...1...1 "
            "...1...1...1...1 "
            "...1...1...1...1");
    ASSERT_EQ(
            hex256(_mm256_set1_epi32(1)),
            ".......1.......1"
            " .......1.......1"
            " .......1.......1"
            " .......1.......1");
    ASSERT_EQ(
            hex256(_mm256_set_epi32(1, 2, -1, 4, 5, 255, 7, 8)),
            ".......7.......8"
            " .......5......FF"
            " FFFFFFFF.......4"
            " .......1.......2");
}

TEST(simd_util, pack256_1) {
    std::vector<bool> bits(256);
    for (size_t i = 0; i < 16; i++) {
        bits[i*i] = true;
    }
    auto m = bits_to_m256i(bits);
    ASSERT_EQ(bits, m256i_to_bits(m));
    ASSERT_EQ(hex256(m),
              "...2..1..2.1.213 "
              ".2....1....2...1 "
              ".....2.....1.... "
              ".......2......1.");
}

TEST(simd_util, popcnt) {
    __m256i m {};
    m256i_u16(m)[1] = 1;
    m256i_u16(m)[2] = 2;
    m256i_u16(m)[4] = 3;
    m256i_u16(m)[6] = 0xFFFF;
    m256i_u16(m)[10] = 0x1111;
    m256i_u16(m)[11] = 0x1113;
    __m256i s = popcnt16(m);
    ASSERT_EQ(m256i_u16(s)[0], 0);
    ASSERT_EQ(m256i_u16(s)[1], 1);
    ASSERT_EQ(m256i_u16(s)[2], 1);
    ASSERT_EQ(m256i_u16(s)[3], 0);
    ASSERT_EQ(m256i_u16(s)[4], 2);
    ASSERT_EQ(m256i_u16(s)[5], 0);
    ASSERT_EQ(m256i_u16(s)[6], 16);
    ASSERT_EQ(m256i_u16(s)[7], 0);
    ASSERT_EQ(m256i_u16(s)[8], 0);
    ASSERT_EQ(m256i_u16(s)[9], 0);
    ASSERT_EQ(m256i_u16(s)[10], 4);
    ASSERT_EQ(m256i_u16(s)[11], 5);
    ASSERT_EQ(m256i_u16(s)[12], 0);
    ASSERT_EQ(m256i_u16(s)[13], 0);
    ASSERT_EQ(m256i_u16(s)[14], 0);
    ASSERT_EQ(m256i_u16(s)[15], 0);
}
