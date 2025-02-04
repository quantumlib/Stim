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

#include "stim/util_bot/str_util.h"

#include "gtest/gtest.h"

#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST(str_util, comma_sep) {
    std::vector<int> v{1, 2, 3};
    std::stringstream out;
    out << comma_sep(v);
    ASSERT_EQ(out.str(), "1, 2, 3");
    ASSERT_EQ(comma_sep(v).str(), "1, 2, 3");
    ASSERT_EQ(comma_sep(std::vector<int>{}).str(), "");
    ASSERT_EQ(comma_sep(std::vector<int>{4}).str(), "4");
    ASSERT_EQ(comma_sep(std::vector<int>{5, 6}).str(), "5, 6");
}
