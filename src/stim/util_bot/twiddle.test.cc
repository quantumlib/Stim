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

#include "gtest/gtest.h"

using namespace stim;

TEST(conversions, is_power_of_2) {
    ASSERT_FALSE(is_power_of_2(0));
    ASSERT_TRUE(is_power_of_2(1));
    ASSERT_TRUE(is_power_of_2(2));
    ASSERT_FALSE(is_power_of_2(3));
    ASSERT_TRUE(is_power_of_2(4));
    ASSERT_FALSE(is_power_of_2(5));
    ASSERT_FALSE(is_power_of_2(6));
    ASSERT_FALSE(is_power_of_2(7));
    ASSERT_TRUE(is_power_of_2(8));
    ASSERT_FALSE(is_power_of_2(9));
}

TEST(conversions, floor_lg2) {
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
