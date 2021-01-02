#include "gtest/gtest.h"
#include "simd_util.h"
#include <random>

TEST(simd_util, hex) {
    ASSERT_EQ(
            hex(_mm256_set1_epi8(1)),
            ".1.1.1.1.1.1.1.1"
            " .1.1.1.1.1.1.1.1"
            " .1.1.1.1.1.1.1.1"
            " .1.1.1.1.1.1.1.1");
    ASSERT_EQ(
            hex(_mm256_set1_epi16(1)),
            "...1...1...1...1 "
            "...1...1...1...1 "
            "...1...1...1...1 "
            "...1...1...1...1");
    ASSERT_EQ(
            hex(_mm256_set1_epi32(1)),
            ".......1.......1"
            " .......1.......1"
            " .......1.......1"
            " .......1.......1");
    ASSERT_EQ(
            hex(_mm256_set_epi32(1, 2, -1, 4, 5, 255, 7, 8)),
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
    ASSERT_EQ(hex(m),
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

TEST(simd_util, transpose256) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis(
            std::numeric_limits<std::uint64_t>::min(),
            std::numeric_limits<std::uint64_t>::max());
    alignas(64) uint64_t data[256*256/64];
    for (auto &e : data) {
        e = dis(gen);
    }

    uint64_t expected[256 * 256 / 64] {};
    for (size_t i = 0; i < 256; i++) {
        for (size_t j = 0; j < 256; j++) {
            size_t k1 = i*256 + j;
            size_t k2 = j*256 + i;
            size_t i0 = k1 / 64;
            size_t i1 = k1 & 63;
            size_t j0 = k2 / 64;
            size_t j1 = k2 & 63;
            uint64_t bit = (data[i0] >> i1) & 1;
            expected[j0] |= bit << j1;
        }
    }

    BitsPtr p {};
    p.u64 = data;
    transpose256(p);
    for (size_t i = 0; i < 256*256/64; i++) {
        ASSERT_EQ(data[i], expected[i]);
    }
}
