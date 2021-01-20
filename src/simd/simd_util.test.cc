#include "gtest/gtest.h"
#include "simd_util.h"
#include "simd_bits.h"
#include "../test_util.test.h"

simd_bits reference_transpose_of(size_t bit_width, const simd_bits &data) {
    auto expected = simd_bits(ceil256(bit_width) * ceil256(bit_width));
    for (size_t i = 0; i < bit_width; i++) {
        for (size_t j = 0; j < bit_width; j++) {
            expected[i*bit_width + j] = data[j*bit_width + i];
        }
    }
    return expected;
}

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
    ASSERT_EQ(hex(m),
              "...2..1..2.1.213 "
              ".2....1....2...1 "
              ".....2.....1.... "
              ".......2......1."
              );
    ASSERT_EQ(bits, m256i_to_bits(m));
}

simd_bits reference_blockwise_transpose_of(size_t bit_area, const simd_bits &data) {
    auto expected = simd_bits(data.num_bits_padded());
    for (size_t block = 0; block < bit_area; block += 1 << 16) {
        for (size_t i = 0; i < 256; i++) {
            for (size_t j = 0; j < 256; j++) {
                expected[i + (j << 8) + block] = data[j + (i << 8) + block];
            }
        }
    }
    return expected;
}

TEST(simd_util, block_transpose_bit_matrix) {
    size_t bit_area = 9 << 16;
    auto data = simd_bits::random(bit_area, SHARED_TEST_RNG());
    auto expected = reference_blockwise_transpose_of(bit_area, data);
    blockwise_transpose_256x256(data.u64, bit_area);
    ASSERT_EQ(data, expected);
}

template <size_t w>
uint8_t determine_permutation_bit(const std::function<void(simd_bits &)> &func, uint8_t bit) {
    auto data = simd_bits(1 << w);
    data[1 << bit] = true;
    func(data);
    uint32_t seen = 0;
    for (size_t k = 0; k < 1 << w; k++) {
        if (data[k]) {
            seen++;
        }
    }
    if (seen != 1) {
        throw std::runtime_error("Not a permutation.");
    }
    for (uint8_t k = 0; k < w; k++) {
        if (data[1 << k]) {
            return k;
        }
    }
    throw std::runtime_error("Not a permutation.");
}

template <size_t w>
bool function_performs_address_bit_permutation(
        const std::function<void(simd_bits &)> &func,
        const std::vector<uint8_t> &bit_permutation) {
    size_t area = 1 << w;
    auto data = simd_bits::random(area, SHARED_TEST_RNG());
    auto expected = simd_bits(area);

    for (size_t k_in = 0; k_in < area; k_in++) {
        size_t k_out = 0;
        for (size_t bit = 0; bit < w; bit++) {
            if ((k_in >> bit) & 1) {
                k_out ^= 1 << bit_permutation[bit];
            }
        }
        expected[k_out] = data[k_in];
    }
    func(data);
    bool result = data == expected;
    if (!result) {
        std::cerr << "actual permutation:";
        for (uint8_t k = 0; k < w; k++) {
            std::cerr << " " << (uint32_t)determine_permutation_bit<w>(func, k) << ",";
        }
        std::cerr << "\n";
    }
    return result;
}


TEST(simd_util, address_permutation) {
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat256_permute_address_swap_ck_rk<1>(d.u64, _mm256_set1_epi8(0x55)); },
            {
                    8, 1, 2, 3, 4, 5, 6, 7,
                    0, 9, 10, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat256_permute_address_swap_ck_rk<2>(d.u64, _mm256_set1_epi8(0x33)); },
            {
                    0, 9, 2, 3, 4, 5, 6, 7,
                    8, 1, 10, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat256_permute_address_swap_ck_rk<4>(d.u64, _mm256_set1_epi8(0xF)); },
            {
                    0, 1, 10, 3, 4, 5, 6, 7,
                    8, 9, 2, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<1>(d.u64); },
            {
                    0, 1, 2, 4, 5, 6, 8, 7,
                    3, 9, 10, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<2>(d.u64); },
            {
                    0, 1, 2, 4, 5, 6, 9, 7,
                    8, 3, 10, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<4>(d.u64); },
            {
                    0, 1, 2, 4, 5, 6, 10, 7,
                    8, 9, 3, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<8>(d.u64); },
            {
                    0, 1, 2, 4, 5, 6, 11, 7,
                    8, 9, 10, 3, 12, 13, 14, 15
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat256_permute_address_swap_c7_r7(d.u64); },
            {
                    0, 1, 2, 3, 4, 5, 6, 15,
                    8, 9, 10, 11, 12, 13, 14, 7
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { transpose_bit_block_256x256(d.u64); },
            {
                    8, 9, 10, 11, 12, 13, 14, 15,
                    0, 1, 2, 3, 4, 5, 6, 7,
            }));

    ASSERT_TRUE(function_performs_address_bit_permutation<16>(
            [](simd_bits &d) { mat_permute_address_swap_ck_rs<1>(d.u64, 1, _mm256_set1_epi8(0x55)); },
            {
                    8, 1, 2, 3, 4, 5, 6, 7,
                    0, 9, 10, 11, 12, 13, 14, 15,
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<20>(
            [](simd_bits &d) { d = reference_transpose_of(1024, d); },
            {
                    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<20>(
            [](simd_bits &d) {
                for (size_t col = 0; col < 1024; col += 256) {
                    for (size_t row = 0; row < 1024; row += 256) {
                        mat_permute_address_swap_ck_rs<1>(
                                d.u64 + ((col + row * 1024) >> 6),
                                4,
                                _mm256_set1_epi8(0x55));
                    }
                }
            },
            {
                    10, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                    0, 11, 12, 13, 14, 15, 16, 17, 18, 19,
            }));
    ASSERT_TRUE(function_performs_address_bit_permutation<20>(
            [](simd_bits &d) {
                for (size_t col = 0; col < 1024; col += 256) {
                    for (size_t row = 0; row < 1024; row += 256) {
                        avx_transpose_64x64s_within_256x256(
                                d.u64 + ((col + row * 1024) >> 6),
                                4);
                    }
                }
            },
            {
                    10, 11, 12, 13, 14, 15, 6, 7, 8, 9,
                    0, 1, 2, 3, 4, 5, 16, 17, 18, 19,
            }));
}

TEST(simd_util, ceil256) {
    ASSERT_EQ(ceil256(0), 0);
    ASSERT_EQ(ceil256(1), 256);
    ASSERT_EQ(ceil256(100), 256);
    ASSERT_EQ(ceil256(255), 256);
    ASSERT_EQ(ceil256(256), 256);
    ASSERT_EQ(ceil256(257), 512);
    ASSERT_EQ(ceil256((1 << 30) - 1), 1 << 30);
    ASSERT_EQ(ceil256(1 << 30), 1 << 30);
    ASSERT_EQ(ceil256((1 << 30) + 1), (1 << 30) + 256);
}

TEST(simd_util, any_non_zero) {
    auto d = simd_bits(5000);
    ASSERT_FALSE(any_non_zero(d.u256, 1));
    ASSERT_FALSE(any_non_zero(d.u256, 2));
    d[256] = true;
    ASSERT_FALSE(any_non_zero(d.u256, 1));
    ASSERT_TRUE(any_non_zero(d.u256, 2));
    d[257] = true;
    ASSERT_FALSE(any_non_zero(d.u256, 1));
    ASSERT_TRUE(any_non_zero(d.u256, 2));
    d[255] = true;
    ASSERT_TRUE(any_non_zero(d.u256, 1));
    ASSERT_TRUE(any_non_zero(d.u256, 2));
}

TEST(simd_util, transpose_bit_matrix) {
    size_t bit_width = 256 * 3;
    auto data = simd_bits::random(bit_width * bit_width, SHARED_TEST_RNG());
    auto expected = reference_transpose_of(bit_width, data);
    transpose_bit_matrix(data.u64, bit_width);
    ASSERT_EQ(data, expected);
}

TEST(simd_util, mem_xor256) {
    auto d1 = simd_bits::random(500, SHARED_TEST_RNG());
    auto d2 = simd_bits::random(500, SHARED_TEST_RNG());
    simd_bits d3(500);
    mem_xor256(d3.u256, d1.u256, 2);
    ASSERT_EQ(d1, d3);
    mem_xor256(d3.u256, d2.u256, 2);
    for (size_t k = 0; k < 500; k++) {
        ASSERT_EQ(d3[k], d1[k] ^ d2[k]);
    }
}
