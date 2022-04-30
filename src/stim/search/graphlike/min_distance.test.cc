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

#include "stim/search/graphlike/min_distance.h"

#include <gtest/gtest.h>

#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/simulators/error_analyzer.h"

constexpr uint64_t NO_NODE_INDEX = UINT64_MAX;

using namespace stim;
using namespace stim::impl_min_distance_graphlike;

TEST(shortest_graphlike_undetectable_logical_error, no_error) {
    // No error.
    ASSERT_THROW(
        { stim::shortest_graphlike_undetectable_logical_error(DetectorErrorModel(), false); }, std::invalid_argument);

    // No undetectable error.
    ASSERT_THROW(
        {
            stim::shortest_graphlike_undetectable_logical_error(
                DetectorErrorModel(R"MODEL(
            error(0.1) D0 L0
        )MODEL"),
                false);
        },
        std::invalid_argument);

    // No logical flips.
    ASSERT_THROW(
        {
            stim::shortest_graphlike_undetectable_logical_error(
                DetectorErrorModel(R"MODEL(
            error(0.1) D0
            error(0.1) D0 D1
            error(0.1) D1
        )MODEL"),
                false);
        },
        std::invalid_argument);
}

TEST(shortest_graphlike_undetectable_logical_error, distance_1) {
    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) L0
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) L0
        )MODEL"));
}

TEST(shortest_graphlike_undetectable_logical_error, distance_2) {
    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0
                error(0.1) D0 L0
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0
            error(1) D0 L0
        )MODEL"));

    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0 L0
                error(0.1) D0 L1
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0 L0
            error(1) D0 L1
        )MODEL"));

    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0 D1 L0
                error(0.1) D0 D1 L1
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0 D1 L0
            error(1) D0 D1 L1
        )MODEL"));

    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0 D1 L1
                error(0.1) D0 D1 L0
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0 D1 L0
            error(1) D0 D1 L1
        )MODEL"));
}

TEST(shortest_graphlike_undetectable_logical_error, distance_3) {
    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0
                error(0.1) D0 D1 L0
                error(0.1) D1
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0
            error(1) D0 D1 L0
            error(1) D1
        )MODEL"));

    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D1
                error(0.1) D1 D0 L0
                error(0.1) D0
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0
            error(1) D0 D1 L0
            error(1) D1
        )MODEL"));
}

TEST(shortest_graphlike_undetectable_logical_error, surface_code) {
    CircuitGenParameters params(5, 5, "rotated_memory_x");
    params.after_clifford_depolarization = 0.001;
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    params.before_round_data_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto graphlike_model = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, false);
    auto ungraphlike_model = ErrorAnalyzer::circuit_to_detector_error_model(circuit, false, true, false, false);

    ASSERT_EQ(stim::shortest_graphlike_undetectable_logical_error(graphlike_model, false).instructions.size(), 5);

    ASSERT_EQ(stim::shortest_graphlike_undetectable_logical_error(graphlike_model, true).instructions.size(), 5);

    ASSERT_EQ(stim::shortest_graphlike_undetectable_logical_error(ungraphlike_model, true).instructions.size(), 5);

    // Throw due to ungraphlike errors.
    ASSERT_THROW(
        { stim::shortest_graphlike_undetectable_logical_error(ungraphlike_model, false); }, std::invalid_argument);
}

TEST(shortest_graphlike_undetectable_logical_error, repetition_code) {
    CircuitGenParameters params(10, 7, "memory");
    params.before_round_data_depolarization = 0.01;
    auto circuit = generate_rep_code_circuit(params).circuit;
    auto graphlike_model = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, false);

    ASSERT_EQ(stim::shortest_graphlike_undetectable_logical_error(graphlike_model, false).instructions.size(), 7);
}

TEST(impl_min_distance_graphlike, DemAdjEdge) {
    DemAdjEdge e1{NO_NODE_INDEX, 0};
    DemAdjEdge e2{1, 0};
    DemAdjEdge e3{NO_NODE_INDEX, 1};
    DemAdjEdge e4{NO_NODE_INDEX, 5};
    ASSERT_EQ(e1.str(), "[boundary]");
    ASSERT_EQ(e2.str(), "D1");
    ASSERT_EQ(e3.str(), "[boundary] L0");
    ASSERT_EQ(e4.str(), "[boundary] L0 L2");

    ASSERT_TRUE(e1 == e1);
    ASSERT_TRUE(!(e1 == e2));
    ASSERT_FALSE(e1 == e2);
    ASSERT_FALSE(!(e1 == e1));

    ASSERT_EQ(e1, (DemAdjEdge{NO_NODE_INDEX, 0}));
    ASSERT_EQ(e2, e2);
    ASSERT_EQ(e3, e3);
    ASSERT_NE(e1, e3);
}

TEST(impl_min_distance_graphlike, DemAdjNode) {
    DemAdjNode n1{};
    DemAdjNode n2{{DemAdjEdge{NO_NODE_INDEX, 0}}};
    DemAdjNode n3{{DemAdjEdge{1, 5}, DemAdjEdge{NO_NODE_INDEX, 8}}};
    ASSERT_EQ(n1.str(), "");
    ASSERT_EQ(n2.str(), "    [boundary]\n");
    ASSERT_EQ(n3.str(), "    D1 L0 L2\n    [boundary] L3\n");

    ASSERT_TRUE(n1 == n1);
    ASSERT_TRUE(!(n1 == n2));
    ASSERT_FALSE(n1 == n2);
    ASSERT_FALSE(!(n1 == n1));

    ASSERT_EQ(n1, (DemAdjNode{}));
    ASSERT_EQ(n2, n2);
    ASSERT_EQ(n3, n3);
    ASSERT_NE(n1, n3);
}

TEST(impl_min_distance_graphlike, DemAdjGraphSearchState_construct) {
    DemAdjGraphSearchState r;
    ASSERT_EQ(r.det_active, NO_NODE_INDEX);
    ASSERT_EQ(r.det_held, NO_NODE_INDEX);
    ASSERT_EQ(r.obs_mask, 0);

    DemAdjGraphSearchState r2(2, 1, 3);
    ASSERT_EQ(r2.det_active, 2);
    ASSERT_EQ(r2.det_held, 1);
    ASSERT_EQ(r2.obs_mask, 3);
}

TEST(impl_min_distance_graphlike, DemAdjGraphSearchState_is_undetected) {
    ASSERT_FALSE(DemAdjGraphSearchState(1, 2, 3).is_undetected());
    ASSERT_FALSE(DemAdjGraphSearchState(1, 2, 2).is_undetected());
    ASSERT_FALSE(DemAdjGraphSearchState(1, 2, 0).is_undetected());
    ASSERT_TRUE(DemAdjGraphSearchState(1, 1, 3).is_undetected());
    ASSERT_TRUE(DemAdjGraphSearchState(NO_NODE_INDEX, NO_NODE_INDEX, 32).is_undetected());
    ASSERT_TRUE(DemAdjGraphSearchState(NO_NODE_INDEX, NO_NODE_INDEX, 0).is_undetected());
}

TEST(impl_min_distance_graphlike, DemAdjGraphSearchState_canonical) {
    DemAdjGraphSearchState a = DemAdjGraphSearchState(1, 2, 3).canonical();
    ASSERT_EQ(a.det_active, 1);
    ASSERT_EQ(a.det_held, 2);
    ASSERT_EQ(a.obs_mask, 3);

    a = DemAdjGraphSearchState(2, 1, 3).canonical();
    ASSERT_EQ(a.det_active, 1);
    ASSERT_EQ(a.det_held, 2);
    ASSERT_EQ(a.obs_mask, 3);

    a = DemAdjGraphSearchState(1, 1, 3).canonical();
    ASSERT_EQ(a.det_active, NO_NODE_INDEX);
    ASSERT_EQ(a.det_held, NO_NODE_INDEX);
    ASSERT_EQ(a.obs_mask, 3);

    a = DemAdjGraphSearchState(1, 1, 1).canonical();
    ASSERT_EQ(a.det_active, NO_NODE_INDEX);
    ASSERT_EQ(a.det_held, NO_NODE_INDEX);
    ASSERT_EQ(a.obs_mask, 1);

    a = DemAdjGraphSearchState(1, NO_NODE_INDEX, 1).canonical();
    ASSERT_EQ(a.det_active, 1);
    ASSERT_EQ(a.det_held, NO_NODE_INDEX);
    ASSERT_EQ(a.obs_mask, 1);
}

TEST(impl_min_distance_graphlike, DemAdjGraphSearchState_append_transition_as_error_instruction_to) {
    DetectorErrorModel out;

    DemAdjGraphSearchState(1, 2, 9).append_transition_as_error_instruction_to(DemAdjGraphSearchState(1, 2, 16), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
    )MODEL"));

    DemAdjGraphSearchState(1, 2, 9).append_transition_as_error_instruction_to(DemAdjGraphSearchState(3, 2, 9), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D3
    )MODEL"));

    DemAdjGraphSearchState(1, 2, 9).append_transition_as_error_instruction_to(
        DemAdjGraphSearchState(1, NO_NODE_INDEX, 9), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D3
        error(1) D2
    )MODEL"));

    DemAdjGraphSearchState(NO_NODE_INDEX, NO_NODE_INDEX, 0)
        .append_transition_as_error_instruction_to(DemAdjGraphSearchState(1, NO_NODE_INDEX, 9), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D3
        error(1) D2
        error(1) D1 L0 L3
    )MODEL"));

    DemAdjGraphSearchState(1, 1, 0).append_transition_as_error_instruction_to(DemAdjGraphSearchState(2, 2, 4), out);
    ASSERT_EQ(out, DetectorErrorModel(R"MODEL(
        error(1) L0 L3 L4
        error(1) D1 D3
        error(1) D2
        error(1) D1 L0 L3
        error(1) L2
    )MODEL"));
}

TEST(impl_min_distance_graphlike, DemAdjGraphSearchState_canonical_equality) {
    DemAdjGraphSearchState v1{1, 2, 3};
    DemAdjGraphSearchState v2{1, 4, 3};
    ASSERT_TRUE(v1 == v1);
    ASSERT_FALSE(v1 == v2);
    ASSERT_FALSE(v1 != v1);
    ASSERT_TRUE(v1 != v2);

    ASSERT_EQ(v1, DemAdjGraphSearchState(2, 1, 3));
    ASSERT_NE(v1, DemAdjGraphSearchState(1, NO_NODE_INDEX, 3));
    ASSERT_EQ(DemAdjGraphSearchState(NO_NODE_INDEX, NO_NODE_INDEX, 0), DemAdjGraphSearchState(1, 1, 0));
    ASSERT_EQ(DemAdjGraphSearchState(3, 3, 0), DemAdjGraphSearchState(1, 1, 0));
    ASSERT_NE(DemAdjGraphSearchState(3, 3, 1), DemAdjGraphSearchState(1, 1, 0));
    ASSERT_EQ(DemAdjGraphSearchState(3, 3, 1), DemAdjGraphSearchState(1, 1, 1));
    ASSERT_EQ(DemAdjGraphSearchState(2, NO_NODE_INDEX, 3), DemAdjGraphSearchState(NO_NODE_INDEX, 2, 3));
}

TEST(impl_min_distance_graphlike, DemAdjGraphSearchState_canonical_ordering) {
    ASSERT_TRUE(DemAdjGraphSearchState(1, 999, 999) < DemAdjGraphSearchState(101, 102, 103));
    ASSERT_TRUE(DemAdjGraphSearchState(999, 1, 999) < DemAdjGraphSearchState(101, 102, 103));
    ASSERT_TRUE(DemAdjGraphSearchState(101, 1, 999) < DemAdjGraphSearchState(101, 102, 103));
    ASSERT_TRUE(DemAdjGraphSearchState(102, 1, 999) < DemAdjGraphSearchState(101, 102, 103));
    ASSERT_TRUE(DemAdjGraphSearchState(101, 102, 3) < DemAdjGraphSearchState(101, 102, 103));

    ASSERT_FALSE(DemAdjGraphSearchState(101, 102, 103) < DemAdjGraphSearchState(101, 102, 103));
    ASSERT_FALSE(DemAdjGraphSearchState(101, 104, 103) < DemAdjGraphSearchState(101, 102, 103));
    ASSERT_FALSE(DemAdjGraphSearchState(101, 102, 104) < DemAdjGraphSearchState(101, 102, 103));
}

TEST(impl_min_distance_graphlike, DemAdjGraphSearchState_str) {
    ASSERT_EQ(DemAdjGraphSearchState(1, 2, 3).str(), "D1 D2 L0 L1 ");
}

TEST(impl_min_distance_graphlike, DemAdjGraph_equality) {
    ASSERT_TRUE(DemAdjGraph(1) == DemAdjGraph(1));
    ASSERT_TRUE(DemAdjGraph(1) != DemAdjGraph(2));
    ASSERT_FALSE(DemAdjGraph(1) == DemAdjGraph(2));
    ASSERT_FALSE(DemAdjGraph(1) != DemAdjGraph(1));

    DemAdjGraph a(1);
    DemAdjGraph b(1);
    ASSERT_EQ(a, b);
    b.distance_1_error_mask = 1;
    ASSERT_NE(a, b);
}

TEST(impl_min_distance_graphlike, DemAdjGraph_add_outward_edge) {
    DemAdjGraph g(3);

    g.add_outward_edge(1, 2, 3);
    ASSERT_EQ(
        g,
        (DemAdjGraph{
            std::vector<DemAdjNode>{
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{2, 3}}},
                DemAdjNode{},
            },
            0}));

    g.add_outward_edge(1, 2, 3);
    ASSERT_EQ(
        g,
        (DemAdjGraph{
            std::vector<DemAdjNode>{
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{2, 3}}},
                DemAdjNode{},
            },
            0}));

    g.add_outward_edge(1, 2, 4);
    ASSERT_EQ(
        g,
        (DemAdjGraph{
            std::vector<DemAdjNode>{
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{2, 3}, DemAdjEdge{2, 4}}},
                DemAdjNode{},
            },
            0}));

    g.add_outward_edge(2, 1, 3);
    ASSERT_EQ(
        g,
        (DemAdjGraph{
            std::vector<DemAdjNode>{
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{2, 3}, DemAdjEdge{2, 4}}},
                DemAdjNode{{DemAdjEdge{1, 3}}},
            },
            0}));

    g.add_outward_edge(2, NO_NODE_INDEX, 3);
    ASSERT_EQ(
        g,
        (DemAdjGraph{
            std::vector<DemAdjNode>{
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{2, 3}, DemAdjEdge{2, 4}}},
                DemAdjNode{{DemAdjEdge{1, 3}, DemAdjEdge{NO_NODE_INDEX, 3}}},
            },
            0}));
}

TEST(impl_min_distance_graphlike, DemAdjGraph_add_edges_from_targets_with_no_separators) {
    DemAdjGraph g(4);

    g.add_edges_from_targets_with_no_separators(std::vector<DemTarget>{DemTarget::relative_detector_id(1)}, false);

    ASSERT_EQ(
        g,
        (DemAdjGraph{
            {
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{NO_NODE_INDEX, 0}}},
                DemAdjNode{},
                DemAdjNode{},
            },
            0}));

    g.add_edges_from_targets_with_no_separators(
        std::vector<DemTarget>{
            DemTarget::relative_detector_id(1),
            DemTarget::relative_detector_id(3),
            DemTarget::observable_id(5),
        },
        false);

    ASSERT_EQ(
        g,
        (DemAdjGraph{
            {
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{NO_NODE_INDEX, 0}, DemAdjEdge{3, 32}}},
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{1, 32}}},
            },
            0}));

    g.add_edges_from_targets_with_no_separators(
        std::vector<DemTarget>{
            DemTarget::observable_id(3),
            DemTarget::observable_id(7),
        },
        false);

    ASSERT_EQ(
        g,
        (DemAdjGraph{
            {
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{NO_NODE_INDEX, 0}, DemAdjEdge{3, 32}}},
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{1, 32}}},
            },
            (1 << 3) + (1 << 7)}));

    DemAdjGraph same_g = g;
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

TEST(impl_min_distance_graphlike, DemAdjGraph_str) {
    DemAdjGraph g{
        {
            DemAdjNode{},
            DemAdjNode{{DemAdjEdge{NO_NODE_INDEX, 0}, DemAdjEdge{3, 32}}},
            DemAdjNode{},
            DemAdjNode{{DemAdjEdge{1, 32}}},
        },
        0};
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

TEST(impl_min_distance_graphlike, DemAdjGraph_add_edges_from_separable_targets) {
    DemAdjGraph g(4);

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
        (DemAdjGraph{
            {
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{NO_NODE_INDEX, 0}, DemAdjEdge{2, 16}}},
                DemAdjNode{{DemAdjEdge{1, 16}}},
                DemAdjNode{},
            },
            0}));
}

TEST(impl_min_distance_graphlike, DemAdjGraph_from_dem) {
    ASSERT_EQ(
        DemAdjGraph::from_dem(
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
        DemAdjGraph(
            {
                DemAdjNode{{DemAdjEdge{NO_NODE_INDEX, 0}, DemAdjEdge{1, 0}}},
                DemAdjNode{{DemAdjEdge{0, 0}, DemAdjEdge{2, 0}}},
                DemAdjNode{{DemAdjEdge{1, 0}, DemAdjEdge{3, 0}}},
                DemAdjNode{{DemAdjEdge{2, 0}, DemAdjEdge{NO_NODE_INDEX, 128}}},
                DemAdjNode{},
                DemAdjNode{{DemAdjEdge{NO_NODE_INDEX, 0}}},
                DemAdjNode{{DemAdjEdge{7, 4}}},
                DemAdjNode{{DemAdjEdge{6, 4}}},
                DemAdjNode{},
            },
            0));
}
