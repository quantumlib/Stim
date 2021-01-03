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
    auto u16 = (uint16_t *)&m;
    u16[1] = 1;
    u16[2] = 2;
    u16[4] = 3;
    u16[6] = 0xFFFF;
    u16[10] = 0x1111;
    u16[11] = 0x1113;
    __m256i s = popcnt16(m);
    auto s16 = (uint16_t *)&s;
    ASSERT_EQ(s16[0], 0);
    ASSERT_EQ(s16[1], 1);
    ASSERT_EQ(s16[2], 1);
    ASSERT_EQ(s16[3], 0);
    ASSERT_EQ(s16[4], 2);
    ASSERT_EQ(s16[5], 0);
    ASSERT_EQ(s16[6], 16);
    ASSERT_EQ(s16[7], 0);
    ASSERT_EQ(s16[8], 0);
    ASSERT_EQ(s16[9], 0);
    ASSERT_EQ(s16[10], 4);
    ASSERT_EQ(s16[11], 5);
    ASSERT_EQ(s16[12], 0);
    ASSERT_EQ(s16[13], 0);
    ASSERT_EQ(s16[14], 0);
    ASSERT_EQ(s16[15], 0);
}

TEST(simd_util, transpose_bit_matrix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis(
            std::numeric_limits<std::uint64_t>::min(),
            std::numeric_limits<std::uint64_t>::max());
    constexpr size_t bit_width = 256 * 3;
    constexpr size_t words = bit_width*bit_width/64;
    alignas(64) uint64_t data[words];
    for (auto &e : data) {
        e = dis(gen);
    }

    uint64_t expected[words] {};
    for (size_t i = 0; i < bit_width; i++) {
        for (size_t j = 0; j < bit_width; j++) {
            size_t k1 = i*bit_width + j;
            size_t k2 = j*bit_width + i;
            size_t i0 = k1 / 64;
            size_t i1 = k1 & 63;
            size_t j0 = k2 / 64;
            size_t j1 = k2 & 63;
            uint64_t bit = (data[i0] >> i1) & 1;
            expected[j0] |= bit << j1;
        }
    }

    transpose_bit_matrix(data, bit_width);
    for (size_t i = 0; i < words; i++) {
        ASSERT_EQ(data[i], expected[i]);
    }
}
