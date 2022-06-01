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

#include "stim/search/graphlike/node.h"

#include "gtest/gtest.h"

using namespace stim;
using namespace stim::impl_search_graphlike;

TEST(search_graphlike, Node) {
    Node n1{};
    Node n2{{Edge{NO_NODE_INDEX, 0}}};
    Node n3{{Edge{1, 5}, Edge{NO_NODE_INDEX, 8}}};
    ASSERT_EQ(n1.str(), "");
    ASSERT_EQ(n2.str(), "    [boundary]\n");
    ASSERT_EQ(n3.str(), "    D1 L0 L2\n    [boundary] L3\n");

    ASSERT_TRUE(n1 == n1);
    ASSERT_TRUE(!(n1 == n2));
    ASSERT_FALSE(n1 == n2);
    ASSERT_FALSE(!(n1 == n1));

    ASSERT_EQ(n1, (Node{}));
    ASSERT_EQ(n2, n2);
    ASSERT_EQ(n3, n3);
    ASSERT_NE(n1, n3);
}
