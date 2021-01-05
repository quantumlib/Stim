#include "gtest/gtest.h"
#include "simd_util.h"
#include <random>
#include "aligned_bits256.h"

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
    size_t bit_width = 256 * 3;
    size_t num_bits = bit_width * bit_width;
    size_t words = num_bits / 64;
    auto data = aligned_bits256(bit_width * bit_width);
    auto expected = aligned_bits256(bit_width * bit_width);

    for (size_t k = 0; k < words; k++) {
        data.data[k] = dis(gen);
    }
    for (size_t i = 0; i < bit_width; i++) {
        for (size_t j = 0; j < bit_width; j++) {
            size_t k1 = i*bit_width + j;
            size_t k2 = j*bit_width + i;
            size_t i0 = k1 / 64;
            size_t i1 = k1 & 63;
            size_t j0 = k2 / 64;
            size_t j1 = k2 & 63;
            uint64_t bit = (data.data[i0] >> i1) & 1;
            expected.data[j0] |= bit << j1;
        }
    }

    transpose_bit_matrix(data.data, bit_width);
    for (size_t i = 0; i < words; i++) {
        ASSERT_EQ(data.data[i], expected.data[i]);
    }
}

TEST(simd_util, block_transpose_bit_matrix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis(
            std::numeric_limits<std::uint64_t>::min(),
            std::numeric_limits<std::uint64_t>::max());
    size_t bit_width = 256 * 3;
    size_t num_bits = bit_width * bit_width;
    size_t words = num_bits / 64;
    auto data = aligned_bits256(bit_width * bit_width);
    auto expected = aligned_bits256(bit_width * bit_width);

    for (size_t k = 0; k < words; k++) {
        data.data[k] = dis(gen);
    }
    for (size_t i = 0; i < bit_width; i++) {
        for (size_t j = 0; j < bit_width; j++) {
            size_t i0 = i & 255;
            size_t i1 = i >> 8;
            size_t j0 = j & 255;
            size_t j1 = j >> 8;
            auto a = i0 + (j0 << 8) + (i1 << 16) + j1 * (bit_width << 8);
            auto b = j0 + (i0 << 8) + (i1 << 16) + j1 * (bit_width << 8);
            auto a0 = a & 63;
            auto a1 = a >> 6;
            auto b0 = b & 63;
            auto b1 = b >> 6;
            uint64_t bit = (data.data[a1] >> a0) & 1;
            expected.data[b1] |= bit << b0;
        }
    }

    transpose_bit_matrix_256x256blocks(data.data, bit_width);
    for (size_t i = 0; i < words; i++) {
        ASSERT_EQ(data.data[i], expected.data[i]);
    }
}

uint8_t determine_permutation_bit(const std::function<void(__m256i *)> &func, uint8_t bit) {
    auto data = aligned_bits256(256 * 256);
    data.set_bit(1 << bit, true);
    func((__m256i *)data.data);
    uint32_t seen = 0;
    for (size_t k = 0; k < 1 << 16; k++) {
        if (data.get_bit(k)) {
            seen++;
        }
    }
    if (seen != 1) {
        throw std::runtime_error("Not a permutation.");
    }
    for (uint8_t k = 0; k < 16; k++) {
        if (data.get_bit(1 << k)) {
            return k;
        }
    }
    throw std::runtime_error("Not a permutation.");
}

bool mat256_function_performs_address_bit_permutation(
        const std::function<void(__m256i *)> &func,
        const std::vector<uint8_t> &bit_permutation) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis(
            std::numeric_limits<std::uint64_t>::min(),
            std::numeric_limits<std::uint64_t>::max());
    size_t area = 256 * 256;
    size_t words = area / 64;
    auto data = aligned_bits256(area);
    auto expected = aligned_bits256(area);

    for (size_t k = 0; k < words; k++) {
        data.data[k] = dis(gen);
    }
    for (size_t k_in = 0; k_in < area; k_in++) {
        size_t k_out = 0;
        for (size_t j = 0; j < 16; j++) {
            if ((k_in >> j) & 1) {
                k_out ^= 1 << bit_permutation[j];
            }
        }
        expected.set_bit(k_out, data.get_bit(k_in));
    }
    func((__m256i *)data.data);
    bool result = data == expected;
    if (!result) {
        std::cerr << "actual permutation:";
        for (uint8_t k = 0; k < 16; k++) {
            std::cerr << " " << (uint32_t)determine_permutation_bit(func, k);
        }
        std::cerr << "\n";
    }
    return result;
}


TEST(simd_util, address_permutation) {
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            [](__m256i *d){ mat256_permute_address_swap_ck_rk<1>(d, _mm256_set1_epi8(0x55)); },
            {
                    8, 1, 2, 3, 4, 5, 6, 7,
                    0, 9, 10, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
        [](__m256i *d){ mat256_permute_address_swap_ck_rk<2>(d, _mm256_set1_epi8(0x33)); },
        {
            0, 9, 2, 3, 4, 5, 6, 7,
            8, 1, 10, 11, 12, 13, 14, 15
        }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            [](__m256i *d){ mat256_permute_address_swap_ck_rk<4>(d, _mm256_set1_epi8(0xF)); },
            {
                    0, 1, 10, 3, 4, 5, 6, 7,
                    8, 9, 2, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<1>,
            {
                    0, 1, 2, 4, 5, 6, 8, 7,
                    3, 9, 10, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<2>,
            {
                    0, 1, 2, 4, 5, 6, 9, 7,
                    8, 3, 10, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<4>,
            {
                    0, 1, 2, 4, 5, 6, 10, 7,
                    8, 9, 3, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<8>,
            {
                    0, 1, 2, 4, 5, 6, 11, 7,
                    8, 9, 10, 3, 12, 13, 14, 15
            }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            mat256_permute_address_swap_c7_r7,
            {
                    0, 1, 2, 3, 4, 5, 6, 15,
                    8, 9, 10, 11, 12, 13, 14, 7
            }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            transpose_bit_matrix_256x256,
            {
                    8, 9, 10, 11, 12, 13, 14, 15,
                    0, 1, 2, 3, 4, 5, 6, 7,
            }));
}

TEST(simd_util, acc_plus_minus_epi2) {
    for (uint8_t a = 0; a < 4; a++) {
        for (uint8_t b = 0; b < 4; b++) {
            for (uint8_t c = 0; c < 4; c++) {
                uint8_t e = (a + b - c) & 3;
                __m256i ma = _mm256_set1_epi8(a);
                __m256i mb = _mm256_set1_epi8(b);
                __m256i mc = _mm256_set1_epi8(c);
                __m256i me = _mm256_set1_epi8(e);
                __m256i actual = acc_plus_minus_epi2(ma, mb, mc);
                ASSERT_EQ(*(uint64_t *)&me, *(uint64_t *)&actual);
            }
        }
    }
}
