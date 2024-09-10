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

#include "stim/mem/simd_word.h"

#include <algorithm>

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

union WordOr64 {
    simd_word<MAX_BITWORD_WIDTH> w;
    uint64_t p[sizeof(simd_word<MAX_BITWORD_WIDTH>) / sizeof(uint64_t)];

    WordOr64() : p() {
    }
};

TEST_EACH_WORD_SIZE_W(simd_word_pick, popcount, {
    WordOr64 v;
    auto n = sizeof(simd_word<W>) * 2;

    for (size_t expected = 0; expected <= n; expected++) {
        std::vector<uint64_t> bits{};
        for (size_t i = 0; i < n; i++) {
            bits.push_back(i < expected);
        }
        for (size_t reps = 0; reps < 10; reps++) {
            std::shuffle(bits.begin(), bits.end(), INDEPENDENT_TEST_RNG());
            for (size_t i = 0; i < n; i++) {
                v.p[i >> 6] = 0;
            }
            for (size_t i = 0; i < n; i++) {
                v.p[i >> 6] |= bits[i] << (i & 63);
            }
            ASSERT_EQ(v.w.popcount(), expected);
        }
    }
})

TEST_EACH_WORD_SIZE_W(simd_word_pick, operator_bool, {
    bitword<W> w{};
    auto p = &w.u8[0];
    ASSERT_EQ((bool)w, false);
    p[0] = 5;
    ASSERT_EQ((bool)w, true);
    p[0] = 0;
    if (bitword<W>::BIT_SIZE > 64) {
        p[1] = 5;
        ASSERT_EQ((bool)w, true);
        p[1] = 0;
        ASSERT_EQ((bool)w, false);
    }
})

TEST_EACH_WORD_SIZE_W(simd_word, integer_conversions, {
    ASSERT_EQ((uint64_t)(simd_word<W>{23}), 23);
    ASSERT_EQ((uint64_t)(simd_word<W>{(uint64_t)23}), 23);
    ASSERT_EQ((int64_t)(simd_word<W>{(uint64_t)23}), 23);
    ASSERT_EQ((uint64_t)(simd_word<W>{(int64_t)23}), 23);
    ASSERT_EQ((int64_t)(simd_word<W>{(int64_t)23}), 23);
    ASSERT_EQ((int64_t)(simd_word<W>{(int64_t)-23}), -23);
    if (W > 64) {
        ASSERT_THROW(
            {
                uint64_t u = (uint64_t)(simd_word<W>{(int64_t)-23});
                std::cerr << u;
            },
            std::invalid_argument);
    }

    simd_word<W> w0{(uint64_t)0};
    simd_word<W> w1{(uint64_t)1};
    simd_word<W> w2{(uint64_t)2};
    ASSERT_EQ((uint64_t)(w1 | w2), 3);
    ASSERT_EQ((uint64_t)w1, 1);
    ASSERT_EQ((uint64_t)w2, 2);
    ASSERT_EQ((int)(w1 | w2), 3);
    ASSERT_EQ((int)w0, 0);
    ASSERT_EQ((int)w1, 1);
    ASSERT_EQ((int)w2, 2);
    ASSERT_EQ((bool)w0, false);
    ASSERT_EQ((bool)w1, true);
    ASSERT_EQ((bool)w2, true);
})

TEST_EACH_WORD_SIZE_W(simd_word, equality, {
    ASSERT_TRUE((simd_word<W>{1}) == (simd_word<W>{1}));
    ASSERT_FALSE((simd_word<W>{1}) == (simd_word<W>{2}));
    ASSERT_TRUE((simd_word<W>{1}) != (simd_word<W>{2}));
    ASSERT_FALSE((simd_word<W>{1}) != (simd_word<W>{1}));
})

TEST_EACH_WORD_SIZE_W(simd_word, shifting, {
    simd_word<W> w{1};

    for (size_t k = 0; k < W; k++) {
        std::array<uint64_t, W / 64> expected{};
        expected[k / 64] = uint64_t{1} << (k % 64);
        EXPECT_EQ((w << static_cast<uint64_t>(k)).to_u64_array(), expected) << k;
        if (k > 0) {
            EXPECT_EQ(((w << (static_cast<uint64_t>(k) - 1)) << 1).to_u64_array(), expected) << k;
        }
        EXPECT_EQ(w, (w << static_cast<uint64_t>(k)) >> static_cast<uint64_t>(k)) << k;
    }

    ASSERT_EQ(w << 0, 1);
    ASSERT_EQ(w >> 0, 1);
    ASSERT_EQ(w << 1, 2);
    ASSERT_EQ(w >> 1, 0);
    ASSERT_EQ(w << 2, 4);
    ASSERT_EQ(w >> 2, 0);
    ASSERT_EQ((w << 5) >> 5, 1);
    ASSERT_EQ((w >> 5) << 5, 0);
    ASSERT_EQ((w << static_cast<uint64_t>(W - 1)) << 1, 0);
    ASSERT_EQ((w << static_cast<uint64_t>(W - 1)) >> static_cast<uint64_t>(W - 1), 1);
})

TEST_EACH_WORD_SIZE_W(simd_word, masking, {
    simd_word<W> w{0b10011};
    ASSERT_EQ((w & 1), 1);
    ASSERT_EQ((w & 2), 2);
    ASSERT_EQ((w & 4), 0);
    ASSERT_EQ((w ^ 1), 0b10010);
    ASSERT_EQ((w ^ 2), 0b10001);
    ASSERT_EQ((w ^ 4), 0b10111);
    ASSERT_EQ((w | 1), 0b10011);
    ASSERT_EQ((w | 2), 0b10011);
    ASSERT_EQ((w | 4), 0b10111);
})

TEST_EACH_WORD_SIZE_W(simd_word, ordering, {
    ASSERT_TRUE(simd_word<W>(1) < simd_word<W>(2));
    ASSERT_TRUE(!(simd_word<W>(2) < simd_word<W>(2)));
    ASSERT_TRUE(!(simd_word<W>(3) < simd_word<W>(2)));
})

TEST_EACH_WORD_SIZE_W(simd_word, from_u64_array, {
    std::array<uint64_t, W / 64> expected;
    for (size_t k = 0; k < expected.size(); k++) {
        expected[k] = k * 3 + 1;
    }
    simd_word<W> w(expected);
    std::array<uint64_t, W / 64> actual = w.to_u64_array();
    ASSERT_EQ(actual, expected);
})
