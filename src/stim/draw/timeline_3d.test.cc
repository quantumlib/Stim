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

#include "stim/draw/timeline_3d.h"

#include "gtest/gtest.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_surface_code.h"

using namespace stim;

TEST(circuit_diagram_timeline_3d, json_obj) {
    EXPECT_EQ(JsonObj(1).str(), "1");
    EXPECT_EQ(JsonObj(2.5).str(), "2.5");
    EXPECT_EQ(JsonObj("test").str(), "\"test\"");

    EXPECT_EQ(JsonObj(std::vector<JsonObj>{}).str(), "[]");
    EXPECT_EQ(JsonObj(std::vector<JsonObj>{1}).str(), "[1]");
    EXPECT_EQ(JsonObj(std::vector<JsonObj>{1, 2.5, "5"}).str(), "[1,2.5,\"5\"]");

    EXPECT_EQ(JsonObj(std::map<std::string, JsonObj>{}).str(), "{}");
    EXPECT_EQ(JsonObj(std::map<std::string, JsonObj>{{"a", 1}}).str(), "{\"a\":1}");
    EXPECT_EQ(JsonObj(std::map<std::string, JsonObj>{{"a", 1}, {"b", 2}}).str(), "{\"a\":1,\"b\":2}");
}

TEST(circuit_diagram_timeline_3d, write_base64) {
    auto f = [](const char *c){
        std::stringstream ss;
        write_base64(c, strlen(c), ss);
        return ss.str();
    };

    EXPECT_EQ(f("light work."), "bGlnaHQgd29yay4=");
    EXPECT_EQ(f("light work"), "bGlnaHQgd29yaw==");
    EXPECT_EQ(f("light wor"), "bGlnaHQgd29y");
    EXPECT_EQ(f("light wo"), "bGlnaHQgd28=");
    EXPECT_EQ(f("light w"), "bGlnaHQgdw==");
    EXPECT_EQ(f(""), "");
    EXPECT_EQ(f("f"), "Zg==");
    EXPECT_EQ(f("fo"), "Zm8=");
    EXPECT_EQ(f("foo"), "Zm9v");
    EXPECT_EQ(f("foob"), "Zm9vYg==");
    EXPECT_EQ(f("fooba"), "Zm9vYmE=");
    EXPECT_EQ(f("foobar"), "Zm9vYmFy");
}

TEST(circuit_diagram_timeline_3d, XXXXXXXXXXX) {
    FILE *f = fopen("/home/craiggidney/w/stim/model.gltf", "w");
    auto s = circuit_diagram_timeline_3d(Circuit());
    fprintf(f, "%s", s.data());
    fclose(f);
    ASSERT_TRUE(false);
}
