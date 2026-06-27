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

#include "stim/search/graphlike/graph.h"

#include "gtest/gtest.h"

using namespace stim;
using namespace stim::impl_search_graphlike;

static simd_bits<64> obs_mask(uint64_t v) {
    simd_bits<64> result(64);
    result.ptr_simd[0] = v;
    return result;
}

TEST(search_graphlike, DemAdjGraph_equality) {
    ASSERT_TRUE(Graph(1, 5) == Graph(1, 5));
    ASSERT_TRUE(Graph(1, 5) != Graph(2, 5));
    ASSERT_TRUE(Graph(1, 5) != Graph(1, 4));
    ASSERT_FALSE(Graph(1, 5) == Graph(2, 5));
    ASSERT_FALSE(Graph(1, 5) != Graph(1, 5));

    Graph a(1, 5);
    Graph b(1, 5);
    ASSERT_EQ(a, b);
    b.distance_1_error_mask = obs_mask(1);
    ASSERT_NE(a, b);
}

TEST(search_graphlike, DemAdjGraph_add_outward_edge) {
    Graph g(3, 64);

    g.add_outward_edge(1, 2, obs_mask(3));
    ASSERT_EQ(
        g,
        (Graph{
            std::vector<Node>{
                Node{},
                Node{{Edge{2, obs_mask(3)}}},
                Node{},
            },
            64,
            obs_mask(0)}));

    g.add_outward_edge(1, 2, obs_mask(3));
    ASSERT_EQ(
        g,
        (Graph{
            std::vector<Node>{
                Node{},
                Node{{Edge{2, obs_mask(3)}}},
                Node{},
            },
            64,
            obs_mask(0)}));

    g.add_outward_edge(1, 2, obs_mask(4));
    ASSERT_EQ(
        g,
        (Graph{
            std::vector<Node>{
                Node{},
                Node{{Edge{2, obs_mask(3)}, Edge{2, obs_mask(4)}}},
                Node{},
            },
            64,
            obs_mask(0)}));

    g.add_outward_edge(2, 1, obs_mask(3));
    ASSERT_EQ(
        g,
        (Graph{
            std::vector<Node>{
                Node{},
                Node{{Edge{2, obs_mask(3)}, Edge{2, obs_mask(4)}}},
                Node{{Edge{1, obs_mask(3)}}},
            },
            64,
            obs_mask(0)}));

    g.add_outward_edge(2, NO_NODE_INDEX, obs_mask(3));
    ASSERT_EQ(
        g,
        (Graph{
            std::vector<Node>{
                Node{},
                Node{{Edge{2, obs_mask(3)}, Edge{2, obs_mask(4)}}},
                Node{{Edge{1, obs_mask(3)}, Edge{NO_NODE_INDEX, obs_mask(3)}}},
            },
            64,
            obs_mask(0)}));
}

TEST(search_graphlike, DemAdjGraph_add_edges_from_targets_with_no_separators) {
    Graph g(4, 64);

    g.add_edges_from_targets_with_no_separators(std::vector<DemTarget>{DemTarget::relative_detector_id(1)}, false);

    ASSERT_EQ(
        g,
        (Graph{
            {
                Node{},
                Node{{Edge{NO_NODE_INDEX, obs_mask(0)}}},
                Node{},
                Node{},
            },
            64,
            obs_mask(0)}));

    g.add_edges_from_targets_with_no_separators(
        std::vector<DemTarget>{
            DemTarget::relative_detector_id(1),
            DemTarget::relative_detector_id(3),
            DemTarget::observable_id(5),
        },
        false);

    ASSERT_EQ(
        g,
        (Graph{
            {
                Node{},
                Node{{Edge{NO_NODE_INDEX, obs_mask(0)}, Edge{3, obs_mask(32)}}},
                Node{},
                Node{{Edge{1, obs_mask(32)}}},
            },
            64,
            obs_mask(0)}));

    g.add_edges_from_targets_with_no_separators(
        std::vector<DemTarget>{
            DemTarget::observable_id(3),
            DemTarget::observable_id(7),
        },
        false);

    ASSERT_EQ(
        g,
        (Graph{
            {
                Node{},
                Node{{Edge{NO_NODE_INDEX, obs_mask(0)}, Edge{3, obs_mask(32)}}},
                Node{},
                Node{{Edge{1, obs_mask(32)}}},
            },
            64,
            obs_mask((1 << 3) + (1 << 7))}));

    Graph same_g = g;
    std::vector<DemTarget> too_big{
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
        DemTarget::relative_detector_id(3),
    };
    ASSERT_THROW({ same_g.add_edges_from_targets_with_no_separators(too_big, false); }, std::invalid_argument);
    ASSERT_EQ(g, same_g);
    same_g.add_edges_from_targets_with_no_separators(too_big, true);
    ASSERT_EQ(g, same_g);
}

TEST(search_graphlike, DemAdjGraph_str) {
    Graph g{
        {
            Node{},
            Node{{Edge{NO_NODE_INDEX, obs_mask(0)}, Edge{3, obs_mask(32)}}},
            Node{},
            Node{{Edge{1, obs_mask(32)}}},
        },
        64,
        obs_mask(0)};
    ASSERT_EQ(
        g.str(),
        "0:\n"
        "1:\n"
        "    [boundary]\n"
        "    D3 L5\n"
        "2:\n"
        "3:\n"
        "    D1 L5\n");
}

TEST(search_graphlike, DemAdjGraph_add_edges_from_separable_targets) {
    Graph g(4, 64);

    g.add_edges_from_separable_targets(
        std::vector<DemTarget>{
            DemTarget::relative_detector_id(1),
            DemTarget::separator(),
            DemTarget::relative_detector_id(1),
            DemTarget::relative_detector_id(2),
            DemTarget::observable_id(4),
        },
        false);

    ASSERT_EQ(
        g,
        (Graph{
            {
                Node{},
                Node{{Edge{NO_NODE_INDEX, obs_mask(0)}, Edge{2, obs_mask(16)}}},
                Node{{Edge{1, obs_mask(16)}}},
                Node{},
            },
            64,
            obs_mask(0)}));
}

TEST(search_graphlike, DemAdjGraph_from_dem) {
    ASSERT_EQ(
        Graph::from_dem(
            DetectorErrorModel(R"MODEL(
        error(0.1) D0
        repeat 3 {
            error(0.1) D0 D1
            shift_detectors 1
        }
        error(0.1) D0 L7
        error(0.1) D2 ^ D3 D4 L2
        detector D5
    )MODEL"),
            false),
        Graph(
            {
                Node{{Edge{NO_NODE_INDEX, obs_mask(0)}, Edge{1, obs_mask(0)}}},
                Node{{Edge{0, obs_mask(0)}, Edge{2, obs_mask(0)}}},
                Node{{Edge{1, obs_mask(0)}, Edge{3, obs_mask(0)}}},
                Node{{Edge{2, obs_mask(0)}, Edge{NO_NODE_INDEX, obs_mask(128)}}},
                Node{},
                Node{{Edge{NO_NODE_INDEX, obs_mask(0)}}},
                Node{{Edge{7, obs_mask(4)}}},
                Node{{Edge{6, obs_mask(4)}}},
                Node{},
            },
            8,
            obs_mask(0)));
}

TEST(search_graphlike, add_edges_from_separable_targets_ignore) {
    Graph actual(10, 64);
    Graph expected(10, 64);
    DetectorErrorModel d(R"DEM(
        error(0.125) D0 ^ D4 D6
    )DEM");
    ASSERT_EQ(actual, expected);
    actual.add_edges_from_separable_targets(d.instructions[0].target_data, true);
    ASSERT_EQ(actual, expected);
    actual.add_edges_from_separable_targets(d.instructions[0].target_data, false);
    expected.add_outward_edge(4, 6, obs_mask(0));
    expected.add_outward_edge(6, 4, obs_mask(0));
    expected.add_outward_edge(0, NO_NODE_INDEX, obs_mask(0));
    ASSERT_EQ(actual, expected);
}
