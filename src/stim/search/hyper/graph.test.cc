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

#include "stim/search/hyper/graph.h"

#include "gtest/gtest.h"

using namespace stim;
using namespace stim::impl_search_hyper;

static simd_bits<64> obs_mask(uint64_t v) {
    simd_bits<64> result(64);
    result.ptr_simd[0] = v;
    return result;
}

TEST(search_hyper_graph, equality) {
    ASSERT_TRUE(Graph(1, 64) == Graph(1, 64));
    ASSERT_TRUE(Graph(1, 64) != Graph(2, 64));
    ASSERT_TRUE(Graph(1, 64) != Graph(1, 32));
    ASSERT_FALSE(Graph(1, 64) == Graph(2, 64));
    ASSERT_FALSE(Graph(1, 64) != Graph(1, 64));

    Graph a(1, 64);
    Graph b(1, 64);
    ASSERT_EQ(a, b);
    b.distance_1_error_mask[0] = true;
    ASSERT_NE(a, b);
}

TEST(search_hyper_graph, add_edge_from_dem_targets) {
    Graph g(3, 64);
    g.add_edge_from_dem_targets(DetectorErrorModel("error(0.01) D0 D1 L3 ^ D0").instructions[0].target_data, SIZE_MAX);
    ASSERT_EQ(
        g,
        (Graph{
            std::vector<Node>{
                Node{},
                Node{{Edge{{{1}}, obs_mask(8)}}},
                Node{},
            },
            64,
            obs_mask(0)}));

    g.add_edge_from_dem_targets(DetectorErrorModel("error(0.01) D0 D1 D2 L0").instructions[0].target_data, SIZE_MAX);
    ASSERT_EQ(
        g,
        (Graph{
            std::vector<Node>{
                Node{{Edge{{{0, 1, 2}}, obs_mask(1)}}},
                Node{{Edge{{{1}}, obs_mask(8)}, Edge{{{0, 1, 2}}, obs_mask(1)}}},
                Node{{Edge{{{0, 1, 2}}, obs_mask(1)}}},
            },
            64,
            obs_mask(0)}));
}

TEST(search_hyper_graph, str) {
    Graph g{
        {
            Node{},
            Node{{Edge{{{1}}, obs_mask(0)}, Edge{{{1, 3}}, obs_mask(32)}}},
            Node{},
            Node{{Edge{{{1, 3}}, obs_mask(32)}}},
        },
        64,
        obs_mask(0)};
    ASSERT_EQ(
        g.str(),
        "0:\n"
        "1:\n"
        "    [boundary] D1\n"
        "    D1 D3 L5\n"
        "2:\n"
        "3:\n"
        "    D1 D3 L5\n");
}

TEST(search_hyper_graph, from_dem) {
    DetectorErrorModel dem(R"MODEL(
        error(0.1) D0
        repeat 3 {
            error(0.1) D0 D1
            shift_detectors 1
        }
        error(0.1) D0 L7
        error(0.1) D2 ^ D3 D4 L2
        detector D5
    )MODEL");

    ASSERT_EQ(
        Graph::from_dem(dem, SIZE_MAX),
        Graph(
            {
                Node{{Edge{{{0}}, obs_mask(0)}, Edge{{{0, 1}}, obs_mask(0)}}},
                Node{{Edge{{{0, 1}}, obs_mask(0)}, Edge{{{1, 2}}, obs_mask(0)}}},
                Node{{Edge{{{1, 2}}, obs_mask(0)}, Edge{{{2, 3}}, obs_mask(0)}}},
                Node{{Edge{{{2, 3}}, obs_mask(0)}, Edge{{{3}}, obs_mask(128)}}},
                Node{},
                Node{{Edge{{{5, 6, 7}}, obs_mask(4)}}},
                Node{{Edge{{{5, 6, 7}}, obs_mask(4)}}},
                Node{{Edge{{{5, 6, 7}}, obs_mask(4)}}},
                Node{},
            },
            8,
            obs_mask(0)));

    ASSERT_EQ(
        Graph::from_dem(dem, 2),
        Graph(
            {
                Node{{Edge{{{0}}, obs_mask(0)}, Edge{{{0, 1}}, obs_mask(0)}}},
                Node{{Edge{{{0, 1}}, obs_mask(0)}, Edge{{{1, 2}}, obs_mask(0)}}},
                Node{{Edge{{{1, 2}}, obs_mask(0)}, Edge{{{2, 3}}, obs_mask(0)}}},
                Node{{Edge{{{2, 3}}, obs_mask(0)}, Edge{{{3}}, obs_mask(128)}}},
                Node{},
                Node{},
                Node{},
                Node{},
                Node{},
            },
            8,
            obs_mask(0)));
    ASSERT_EQ(
        Graph::from_dem(dem, 1),
        Graph(
            {
                Node{{Edge{{{0}}, obs_mask(0)}}},
                Node{},
                Node{},
                Node{{Edge{{{3}}, obs_mask(128)}}},
                Node{},
                Node{},
                Node{},
                Node{},
                Node{},
            },
            8,
            obs_mask(0)));
}
