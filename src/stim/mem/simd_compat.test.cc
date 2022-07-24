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

#include "stim/mem/simd_compat.h"

#include <algorithm>

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

union WordOr64 {
    simd_word w;
    uint64_t p[sizeof(simd_word) / sizeof(uint64_t)];

    WordOr64() : p() {
    }
};

TEST(simd_compat, popcount) {
    WordOr64 v;
    auto n = sizeof(simd_word) * 8;

    for (size_t expected = 0; expected <= n; expected++) {
        std::vector<uint64_t> bits{};
        for (size_t i = 0; i < n; i++) {
            bits.push_back(i < expected);
        }
        for (size_t reps = 0; reps < 100; reps++) {
            std::shuffle(bits.begin(), bits.end(), SHARED_TEST_RNG());
            for (size_t i = 0; i < n; i++) {
                v.p[i >> 6] = 0;
            }
            for (size_t i = 0; i < n; i++) {
                v.p[i >> 6] |= bits[i] << (i & 63);
            }
            ASSERT_EQ(v.w.popcount(), expected);
        }
    }
}

TEST(simd_compat, do_interleave8_tile128) {
    simd_word t1{};
    simd_word t2{};
    auto c1 = t1.u8;
    auto c2 = t2.u8;
    for (uint8_t k = 0; k < (uint8_t)sizeof(uint64_t) * 2; k++) {
        ASSERT_LT(k, sizeof(t1.u8));
        c1[k] = k + 1;
        c2[k] = k + 128;
    }
    t1.do_interleave8_tile128(t2);
    for (size_t k = 0; k < sizeof(uint64_t) * 2; k++) {
        ASSERT_EQ(c1[k], k % 2 == 0 ? (k / 2) + 1 : (k / 2) + 128);
        ASSERT_EQ(c2[k], k % 2 == 0 ? (k / 2) + 1 + 8 : (k / 2) + 128 + 8);
    }
}

TEST(simd_word, operator_bool) {
    simd_word w{};
    auto p = &w.u64[0];
    ASSERT_EQ((bool)w, false);
    p[0] = 5;
    ASSERT_EQ((bool)w, true);
    p[0] = 0;
    p[1] = 5;
    ASSERT_EQ((bool)w, true);
    p[1] = 0;
    ASSERT_EQ((bool)w, false);
}
