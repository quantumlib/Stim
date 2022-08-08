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
#include "stim/mem/simd_word.test.h"

using namespace stim;

template <size_t W>
simd_bits<W> reference_transpose_of(size_t bit_width, const simd_bits<W> &data) {
    assert(!(bit_width & 0xFF));
    auto expected = simd_bits<W>(bit_width * bit_width);
    for (size_t i = 0; i < bit_width; i++) {
        for (size_t j = 0; j < bit_width; j++) {
            expected[i * bit_width + j] = data[j * bit_width + i];
        }
    }
    return expected;
}

template <size_t A, size_t W>
uint8_t determine_permutation_bit_else_0xFF(const std::function<void(simd_bits<W> &)> &func, uint8_t bit) {
    auto data = simd_bits<W>(1 << A);
    data[1 << bit] = true;
    func(data);
    uint32_t seen = 0;
    for (size_t k = 0; k < 1 << A; k++) {
        if (data[k]) {
            seen++;
        }
    }
    if (seen != 1) {
        return 0xFF;
    }
    for (uint8_t k = 0; k < A; k++) {
        if (data[1 << k]) {
            return k;
        }
    }
    return 0xFF;
}

template <size_t A, size_t W>
std::string determine_if_function_performs_bit_permutation_helper(
    const std::function<void(simd_bits<W> &)> &func, const std::array<uint8_t, A> &bit_permutation) {
    size_t area = 1 << A;
    auto data = simd_bits<W>::random(area, SHARED_TEST_RNG());
    auto expected = simd_bits<W>(area);

    for (size_t k_in = 0; k_in < area; k_in++) {
        size_t k_out = 0;
        for (size_t bit = 0; bit < A; bit++) {
            if ((k_in >> bit) & 1) {
                k_out ^= 1 << bit_permutation[bit];
            }
        }
        expected[k_out] = data[k_in];
    }
    func(data);
    bool result = data == expected;
    if (!result) {
        std::stringstream ss;
        std::array<uint8_t, A> perm;
        ss << "actual permutation:";
        for (uint8_t k = 0; k < A; k++) {
            auto v = (uint32_t)determine_permutation_bit_else_0xFF<A>(func, k);
            if (v == 0xFF) {
                ss << " [not a permutation],";
            } else {
                ss << " " << v << ",";
            }
            perm[k] = v;
        }
        ss << "\n";
        if (perm == bit_permutation) {
            ss << "[BUT PERMUTATION ACTS INCORRECTLY ON SOME BITS.]\n";
        }
        return ss.str();
    }
    return "";
}

template <size_t A, size_t W>
void EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION(
    const std::function<void(simd_bits<W> &)> &func, const std::array<uint8_t, A> &bit_permutation) {
    size_t area = 1 << A;
    auto data = simd_bits<W>::random(area, SHARED_TEST_RNG());
    auto expected = simd_bits<W>(area);

    for (size_t k_in = 0; k_in < area; k_in++) {
        size_t k_out = 0;
        for (size_t bit = 0; bit < A; bit++) {
            if ((k_in >> bit) & 1) {
                k_out ^= 1 << bit_permutation[bit];
            }
        }
        expected[k_out] = data[k_in];
    }
    func(data);
    if (data != expected) {
        std::stringstream ss;
        std::array<uint8_t, A> perm;
        ss << "\nexpected permutation:";
        for (const auto &e : bit_permutation) {
            ss << " " << (int)e << ",";
        }

        ss << "\n  actual permutation:";
        for (uint8_t k = 0; k < A; k++) {
            auto v = (uint32_t)determine_permutation_bit_else_0xFF<A>(func, k);
            if (v == 0xFF) {
                ss << " [not a permutation],";
            } else {
                ss << " " << v << ",";
            }
            perm[k] = v;
        }
        ss << "\n";
        if (perm == bit_permutation) {
            ss << "[BUT PERMUTATION ACTS INCORRECTLY ON SOME BITS.]\n";
        }
        EXPECT_TRUE(false) << ss.str();
    }
}

TEST(simd_util, inplace_transpose_64x64) {
    constexpr size_t W = 64;
    simd_bits<W> data = simd_bits<W>::random(64 * 64, SHARED_TEST_RNG());
    simd_bits<W> copy = data;
    inplace_transpose_64x64(copy.u64, 1);
    for (size_t i = 0; i < 64; i++) {
        for (size_t j = 0; j < 64; j++) {
            ASSERT_EQ(data[i * 64 + j], copy[j * 64 + i]);
        }
    }
}

TEST_EACH_WORD_SIZE_W(simd_util, inplace_transpose, {
    if (W == 64) {
        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<12, W>(
            [](simd_bits<W> &d) {
                inplace_transpose_64x64(d.u64, 1);
            },
            {
                6,
                7,
                8,
                9,
                10,
                11,
                0,
                1,
                2,
                3,
                4,
                5,
            });

        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<13, W>(
            [](simd_bits<W> &d) {
                inplace_transpose_64x64(d.u64, 1);
                inplace_transpose_64x64(d.u64 + 64, 1);
            },
            {
                6,
                7,
                8,
                9,
                10,
                11,
                0,
                1,
                2,
                3,
                4,
                5,
                12,
            });

        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<13, W>(
            [](simd_bits<W> &d) {
                inplace_transpose_64x64(d.u64, 2);
                inplace_transpose_64x64(d.u64 + 1, 2);
            },
            {
                7,
                8,
                9,
                10,
                11,
                12,
                6,
                0,
                1,
                2,
                3,
                4,
                5,
            });
    } else if (W == 128) {
        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<14, W>(
            [](simd_bits<W> &d) {
                bitword<W>::inplace_transpose_square(d.ptr_simd, 1);
            },
            {
                7,
                8,
                9,
                10,
                11,
                12,
                13,
                0,
                1,
                2,
                3,
                4,
                5,
                6,
            });

        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<15, W>(
            [](simd_bits<W> &d) {
                bitword<W>::inplace_transpose_square(d.ptr_simd, 1);
                bitword<W>::inplace_transpose_square(d.ptr_simd + 128, 1);
            },
            {
                7,
                8,
                9,
                10,
                11,
                12,
                13,
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                14,
            });

        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<15, W>(
            [](simd_bits<W> &d) {
                bitword<W>::inplace_transpose_square(d.ptr_simd, 2);
                bitword<W>::inplace_transpose_square(d.ptr_simd + 1, 2);
            },
            {
                8,
                9,
                10,
                11,
                12,
                13,
                14,
                7,
                0,
                1,
                2,
                3,
                4,
                5,
                6,
            });
    } else if (W == 256) {
        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<16, W>(
            [](simd_bits<W> &d) {
                bitword<W>::inplace_transpose_square(d.ptr_simd, 1);
            },
            {
                8,
                9,
                10,
                11,
                12,
                13,
                14,
                15,
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
            });

        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<17, W>(
            [](simd_bits<W> &d) {
                bitword<W>::inplace_transpose_square(d.ptr_simd, 1);
                bitword<W>::inplace_transpose_square(d.ptr_simd + 256, 1);
            },
            {
                8,
                9,
                10,
                11,
                12,
                13,
                14,
                15,
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                16,
            });

        EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<17, W>(
            [](simd_bits<W> &d) {
                bitword<W>::inplace_transpose_square(d.ptr_simd, 2);
                bitword<W>::inplace_transpose_square(d.ptr_simd + 1, 2);
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
                8,
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
            });
    }
})

TEST_EACH_WORD_SIZE_W(simd_util, simd_bit_table_transpose, {
    EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<20, W>(
        [](simd_bits<W> &d) {
            d = reference_transpose_of(1024, d);
        },
        {
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        });
    EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<18, W>(
        [](simd_bits<W> &d) {
            simd_bit_table<W> t(512, 512);
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
        });
    EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<20, W>(
        [](simd_bits<W> &d) {
            simd_bit_table<W> t(1024, 1024);
            t.data = d;
            t.do_square_transpose();
            d = t.data;
        },
        {
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        });
    EXPECT_FUNCTION_PERFORMS_ADDRESS_BIT_PERMUTATION<19, W>(
        [](simd_bits<W> &d) {
            simd_bit_table<W> t(512, 1024);
            t.data = d;
            auto t2 = t.transposed();
            d = t2.data;
        },
        {
            9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 0, 1, 2, 3, 4, 5, 6, 7, 8,
        });
})

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
