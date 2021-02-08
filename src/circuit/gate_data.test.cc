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

#include "gate_data.h"

#include <gtest/gtest.h>

#include "../test_util.test.h"

TEST(gate_data, lookup) {
    ASSERT_TRUE(GATE_DATA.has("H"));
    ASSERT_FALSE(GATE_DATA.has("H2345"));
    ASSERT_EQ(GATE_DATA.at("H").id, GATE_DATA.at("H_XZ").id);
    ASSERT_NE(GATE_DATA.at("H").id, GATE_DATA.at("H_XY").id);
    ASSERT_THROW(GATE_DATA.at("MISSING"), std::out_of_range);

    ASSERT_TRUE(GATE_DATA.has("h"));
    ASSERT_TRUE(GATE_DATA.has("Cnot"));

    ASSERT_TRUE(GATE_DATA.at("h").id == GATE_DATA.at("H").id);
    ASSERT_TRUE(GATE_DATA.at("H_xz").id == GATE_DATA.at("H").id);
}
