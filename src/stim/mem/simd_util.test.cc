// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/mem/simd_util.h"

#include <algorithm>

#include "gtest/gtest.h"

#include "stim/mem/simd_bit_table.h"
#include "stim/mem/simd_bits.h"
#include "stim/test_util.test.h"

using namespace stim;

simd_bits reference_transpose_of(size_t bit_width, const simd_bits &data) {
    assert(!(bit_width & 0xFF));
    auto expected = simd_bits(bit_width * bit_width);
    for (size_t i = 0; i < bit_width; i++) {
        for (size_t j = 0; j < bit_width; j++) {
            expected[i * bit_width + j] = data[j * bit_width + i];
        }
    }
    return expected;
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
    const std::function<void(simd_bits &)> &func, const std::vector<uint8_t> &bit_permutation) {
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
        std::vector<uint8_t> perm;
        std::cerr << "actual permutation:";
        for (uint8_t k = 0; k < w; k++) {
            auto v = (uint32_t)determine_permutation_bit<w>(func, k);
            std::cerr << " " << v << ",";
            perm.push_back(v);
        }
        std::cerr << "\n";
        if (perm == bit_permutation) {
            std::cerr << "[BUT PERMUTATION ACTS INCORRECTLY ON SOME BITS.]\n";
        }
    }
    return result;
}

TEST(simd_bit_matrix, transpose) {
    ASSERT_TRUE(function_performs_address_bit_permutation<20>(
        [](simd_bits &d) {
            d = reference_transpose_of(1024, d);
        },
        {
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        }));
    ASSERT_TRUE(function_performs_address_bit_permutation<18>(
        [](simd_bits &d) {
            simd_bit_table t(512, 512);
            t.data = d;
            t.do_square_transpose();
            d = t.data;
        },
        {
            9,
            10,
            11,
            12,
            13,
            14,
            15,
            16,
            17,
            0,
            1,
            2,
            3,
            4,
            5,
            6,
            7,
            8,
        }));
    ASSERT_TRUE(function_performs_address_bit_permutation<20>(
        [](simd_bits &d) {
            simd_bit_table t(1024, 1024);
            t.data = d;
            t.do_square_transpose();
            d = t.data;
        },
        {
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        }));
    ASSERT_TRUE(function_performs_address_bit_permutation<19>(
        [](simd_bits &d) {
            simd_bit_table t(512, 1024);
            t.data = d;
            auto t2 = t.transposed();
            d = t2.data;
        },
        {
            9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 0, 1, 2, 3, 4, 5, 6, 7, 8,
        }));
}

TEST(simd_util, interleave_mask) {
    ASSERT_EQ(interleave_mask(1), 0x5555555555555555ULL);
    ASSERT_EQ(interleave_mask(2), 0x3333333333333333ULL);
    ASSERT_EQ(interleave_mask(4), 0x0F0F0F0F0F0F0F0FULL);
    ASSERT_EQ(interleave_mask(8), 0x00FF00FF00FF00FFULL);
    ASSERT_EQ(interleave_mask(16), 0x0000FFFF0000FFFFULL);
    ASSERT_EQ(interleave_mask(32), 0x00000000FFFFFFFFULL);
}

TEST(simd_util, popcnt64) {
    for (size_t expected = 0; expected <= 64; expected++) {
        std::vector<uint64_t> bits{};
        for (size_t i = 0; i < 64; i++) {
            bits.push_back(i < expected);
        }
        for (size_t reps = 0; reps < 100; reps++) {
            std::shuffle(bits.begin(), bits.end(), SHARED_TEST_RNG());
            uint64_t v = 0;
            for (size_t i = 0; i < 64; i++) {
                v |= bits[i] << i;
            }
            ASSERT_EQ(popcnt64(v), expected);
        }
    }
}

TEST(simd_util, inplace_transpose_64x64) {
    simd_bits data = simd_bits::random(64 * 64, SHARED_TEST_RNG());
    simd_bits copy = data;
    inplace_transpose_64x64(copy.u64);
    for (size_t i = 0; i < 64; i++) {
        for (size_t j = 0; j < 64; j++) {
            ASSERT_EQ(data[i * 64 + j], copy[j * 64 + i]);
        }
    }
}
