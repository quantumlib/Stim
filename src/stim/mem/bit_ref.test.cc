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

#include "stim/mem/bit_ref.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(bit_ref, get) {
    uint64_t word = 0;
    bit_ref b(&word, 5);
    ASSERT_EQ(b, false);
    word = 1;
    ASSERT_EQ(b, false);
    word = 32;
    ASSERT_EQ(b, true);
}

TEST(bit_ref, set) {
    uint64_t word = 0;
    bit_ref b(&word, 5);
    word = UINT64_MAX;
    b = false;
    ASSERT_EQ(word, UINT64_MAX - 32);
}

TEST(bit_ref, bit_xor) {
    uint64_t word = 0;
    bit_ref b2(&word, 2);
    bit_ref b5(&word, 5);
    b5 ^= 1;
    ASSERT_EQ(word, 32);
    b5 ^= 1;
    ASSERT_EQ(word, 0);
    b5 ^= 1;
    ASSERT_EQ(word, 32);
    b5 ^= 0;
    ASSERT_EQ(word, 32);
    b2 ^= 1;
    ASSERT_EQ(word, 36);
    b5 ^= 0;
    ASSERT_EQ(word, 36);
    b2 ^= 1;
    ASSERT_EQ(word, 32);
    b5 ^= 1;
    ASSERT_EQ(word, 0);
}

TEST(bit_ref, bit_or) {
    uint64_t word = 0;
    bit_ref b2(&word, 2);
    bit_ref b3(&word, 3);
    b2 |= 0;
    ASSERT_EQ(word, 0);
    b2 |= 1;
    ASSERT_EQ(word, 4);
    b3 |= 1;
    ASSERT_EQ(word, 12);
    word = 0;
    b3 |= 0;
    ASSERT_EQ(word, 0);
    b3 |= 1;
    ASSERT_EQ(word, 8);
    b3 |= 1;
    ASSERT_EQ(word, 8);
    b3 |= 0;
    ASSERT_EQ(word, 8);
}

TEST(bit_ref, bit_andr) {
    uint64_t word = 8;
    bit_ref b2(&word, 2);
    b2 &= 0;
    ASSERT_EQ(word, 8);
    b2 &= 1;
    ASSERT_EQ(word, 8);
    bit_ref b3(&word, 3);
    b3 &= 1;
    ASSERT_EQ(word, 8);
    b3 &= 0;
    ASSERT_EQ(word, 0);
    b3 &= 0;
    ASSERT_EQ(word, 0);
    b3 &= 1;
    ASSERT_EQ(word, 0);
}

TEST(bit_ref, swap_with) {
    uint64_t word = 8;
    bit_ref b3(&word, 3);
    bit_ref b5(&word, 5);
    b3.swap_with(b5);
    ASSERT_EQ(word, 32);
    b3.swap_with(b5);
    ASSERT_EQ(word, 8);
    word = 0;
    b3.swap_with(b5);
    ASSERT_EQ(word, 0);
    word = UINT64_MAX;
    b3.swap_with(b5);
    ASSERT_EQ(word, UINT64_MAX);
}
