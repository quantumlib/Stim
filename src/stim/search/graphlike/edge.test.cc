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

#include "stim/search/graphlike/edge.h"

#include "gtest/gtest.h"

#include "stim/search/graphlike/node.h"

using namespace stim;
using namespace stim::impl_search_graphlike;

static simd_bits<64> obs_mask(uint64_t v) {
    simd_bits<64> result(64);
    result.ptr_simd[0] = v;
    return result;
}

TEST(search_graphlike, Edge) {
    Edge e1{NO_NODE_INDEX, obs_mask(0)};
    Edge e2{1, obs_mask(0)};
    Edge e3{NO_NODE_INDEX, obs_mask(1)};
    Edge e4{NO_NODE_INDEX, obs_mask(5)};
    ASSERT_EQ(e1.str(), "[boundary]");
    ASSERT_EQ(e2.str(), "D1");
    ASSERT_EQ(e3.str(), "[boundary] L0");
    ASSERT_EQ(e4.str(), "[boundary] L0 L2");

    ASSERT_TRUE(e1 == e1);
    ASSERT_TRUE(!(e1 == e2));
    ASSERT_FALSE(e1 == e2);
    ASSERT_FALSE(!(e1 == e1));

    ASSERT_EQ(e1, (Edge{NO_NODE_INDEX, obs_mask(0)}));
    ASSERT_EQ(e2, e2);
    ASSERT_EQ(e3, e3);
    ASSERT_NE(e1, e3);
}
