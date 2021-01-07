#include "gtest/gtest.h"
#include "simd_util.h"
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
    ASSERT_EQ(hex(m),
              "...2..1..2.1.213 "
              ".2....1....2...1 "
              ".....2.....1.... "
              ".......2......1."
              );
    ASSERT_EQ(bits, m256i_to_bits(m));
}

aligned_bits256 reference_blockwise_transpose_of(size_t bit_area, const aligned_bits256 &data) {
    auto expected = aligned_bits256(data.num_bits);
    for (size_t block = 0; block < bit_area; block += 1 << 16) {
        for (size_t i = 0; i < 256; i++) {
            for (size_t j = 0; j < 256; j++) {
                auto a = i + (j << 8) + block;
                auto b = j + (i << 8) + block;
                expected.set_bit(a, data.get_bit(b));
            }
        }
    }
    return expected;
}

TEST(simd_util, block_transpose_bit_matrix) {
    size_t bit_area = 9 << 16;
    auto data = aligned_bits256::random(bit_area);
    auto expected = reference_blockwise_transpose_of(bit_area, data);
    transpose_bit_matrix_256x256blocks(data.u64, bit_area);
    ASSERT_EQ(data, expected);
}

uint8_t determine_permutation_bit(const std::function<void(uint64_t *)> &func, uint8_t bit) {
    auto data = aligned_bits256(256 * 256);
    data.set_bit(1 << bit, true);
    func(data.u64);
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
        const std::function<void(uint64_t *)> &func,
        const std::vector<uint8_t> &bit_permutation) {
    size_t area = 256 * 256;
    auto data = aligned_bits256::random(area);
    auto expected = aligned_bits256(area);

    for (size_t k_in = 0; k_in < area; k_in++) {
        size_t k_out = 0;
        for (size_t j = 0; j < 16; j++) {
            if ((k_in >> j) & 1) {
                k_out ^= 1 << bit_permutation[j];
            }
        }
        expected.set_bit(k_out, data.get_bit(k_in));
    }
    func(data.u64);
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
            [](uint64_t *d){ mat256_permute_address_swap_ck_rk<1>(d, _mm256_set1_epi8(0x55)); },
            {
                    8, 1, 2, 3, 4, 5, 6, 7,
                    0, 9, 10, 11, 12, 13, 14, 15
            }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
        [](uint64_t *d){ mat256_permute_address_swap_ck_rk<2>(d, _mm256_set1_epi8(0x33)); },
        {
            0, 9, 2, 3, 4, 5, 6, 7,
            8, 1, 10, 11, 12, 13, 14, 15
        }));
    ASSERT_TRUE(mat256_function_performs_address_bit_permutation(
            [](uint64_t *d){ mat256_permute_address_swap_ck_rk<4>(d, _mm256_set1_epi8(0xF)); },
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
            transpose_bit_block_256x256,
            {
                    8, 9, 10, 11, 12, 13, 14, 15,
                    0, 1, 2, 3, 4, 5, 6, 7,
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
