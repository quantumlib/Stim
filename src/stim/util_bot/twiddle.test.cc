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

#include "stim/util_bot/twiddle.h"

#include <bit>

#include "gtest/gtest.h"

using namespace stim;

TEST(twiddle, is_power_of_2) {
    ASSERT_EQ(is_power_of_2(0), 0);
    ASSERT_EQ(is_power_of_2(1), 1);
    ASSERT_EQ(is_power_of_2(2), 1);
    ASSERT_EQ(is_power_of_2(3), 0);
    ASSERT_EQ(is_power_of_2(4), 1);
    ASSERT_EQ(is_power_of_2(5), 0);
    ASSERT_EQ(is_power_of_2(6), 0);
    ASSERT_EQ(is_power_of_2(7), 0);
    ASSERT_EQ(is_power_of_2(8), 1);
    ASSERT_EQ(is_power_of_2(9), 0);
}

TEST(twiddle, floor_lg2) {
    ASSERT_EQ(floor_lg2(1), 0);
    ASSERT_EQ(floor_lg2(2), 1);
    ASSERT_EQ(floor_lg2(3), 1);
    ASSERT_EQ(floor_lg2(4), 2);
    ASSERT_EQ(floor_lg2(5), 2);
    ASSERT_EQ(floor_lg2(6), 2);
    ASSERT_EQ(floor_lg2(7), 2);
    ASSERT_EQ(floor_lg2(8), 3);
    ASSERT_EQ(floor_lg2(9), 3);
}

TEST(twiddle, first_set_bit) {
    ASSERT_EQ(first_set_bit(0b0000111001000, 0), 3);
    ASSERT_EQ(first_set_bit(0b0000111001000, 1), 3);
    ASSERT_EQ(first_set_bit(0b0000111001000, 2), 3);
    ASSERT_EQ(first_set_bit(0b0000111001000, 3), 3);
    ASSERT_EQ(first_set_bit(0b0000111001000, 4), 6);
    ASSERT_EQ(first_set_bit(0b0000111001000, 5), 6);
    ASSERT_EQ(first_set_bit(0b0000111001000, 6), 6);
    ASSERT_EQ(first_set_bit(0b0000111001000, 7), 7);
    ASSERT_EQ(first_set_bit(0b0000111001000, 8), 8);

    ASSERT_EQ(first_set_bit(0b0000111001001, 0), 0);
    ASSERT_EQ(first_set_bit(1 << 20, 0), 20);
}
