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

#include "stim/diagram/json_obj.h"

#include "gtest/gtest.h"

using namespace stim;
using namespace stim_draw_internal;

TEST(json_obj, str) {
    EXPECT_EQ(JsonObj(1).str(), "1");
    EXPECT_EQ(JsonObj(2.5f).str(), "2.5");
    EXPECT_EQ(JsonObj("test").str(), "\"test\"");

    EXPECT_EQ(JsonObj(std::vector<JsonObj>{}).str(), "[]");
    EXPECT_EQ(JsonObj(std::vector<JsonObj>{1}).str(), "[1]");
    EXPECT_EQ(JsonObj(std::vector<JsonObj>{1, 2.5f, "5"}).str(), "[1,2.5,\"5\"]");

    EXPECT_EQ(JsonObj(std::map<std::string, JsonObj>{}).str(), "{}");
    EXPECT_EQ(JsonObj(std::map<std::string, JsonObj>{{"a", 1}}).str(), "{\"a\":1}");
    EXPECT_EQ(JsonObj(std::map<std::string, JsonObj>{{"a", 1}, {"b", 2}}).str(), "{\"a\":1,\"b\":2}");
}
