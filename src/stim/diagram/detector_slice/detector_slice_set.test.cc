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

#include "stim/diagram/detector_slice/detector_slice_set.h"

#include <fstream>

#include "gtest/gtest.h"

#include "stim/diagram/timeline/timeline_svg_drawer.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;
using namespace stim_draw_internal;

TEST(detector_slice_set, from_circuit) {
    CoordFilter empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(
        stim::Circuit(R"CIRCUIT(
        QUBIT_COORDS(3, 5) 1
        R 0
        M 0
        DETECTOR rec[-1]
        TICK
        CX 2 1

        TICK  # Here
        H 0
        CX 1 0
        M 0
        DETECTOR rec[-1]
        M 1
        DETECTOR rec[-1]

        REPEAT 100 {
            TICK
            R 0
            M 0
            DETECTOR rec[-1]
        }
)CIRCUIT"),
        2,
        1,
        {&empty_filter});
    ASSERT_EQ(slice_set.coordinates, (std::map<uint64_t, std::vector<double>>{{0, {}}, {1, {3, 5}}}));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>>{
            {{2, DemTarget::relative_detector_id(1)}, {GateTarget::x(0), GateTarget::z(1)}},
            {{2, DemTarget::relative_detector_id(2)}, {GateTarget::z(1)}},
        }));
}

TEST(detector_slice_set, big_loop_seeking) {
    stim::Circuit circuit(R"CIRCUIT(
        REPEAT 100000 {
            REPEAT 10000 {
                REPEAT 1000 {
                    REPEAT 100 {
                        REPEAT 10 {
                            RY 0
                            TICK
                            MY 0
                            DETECTOR rec[-1]
                        }
                    }
                }
                RX 1
                TICK
                MX 1
                OBSERVABLE_INCLUDE(5) rec[-1]
                DETECTOR rec[-1]
            }
        }
    )CIRCUIT");

    uint64_t inner = 10 * 100 * 1000 + 1;
    CoordFilter empty_filter;
    CoordFilter obs5_filter;
    obs5_filter.use_target = true;
    obs5_filter.exact_target = DemTarget::observable_id(5);
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, inner * 10000ULL * 50ULL + 2ULL, 1, {&empty_filter});
    ASSERT_EQ(slice_set.coordinates, (std::map<uint64_t, std::vector<double>>{{0, {}}, {1, {}}}));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>>{
            {{inner * 10000ULL * 50ULL + 2ULL, DemTarget::relative_detector_id(inner * 10000ULL * 50ULL + 1ULL)},
             {GateTarget::y(0)}},
        }));

    slice_set = DetectorSliceSet::from_circuit_ticks(
        circuit,
        inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL,
        1,
        std::vector<CoordFilter>{empty_filter, obs5_filter});
    ASSERT_EQ(slice_set.coordinates, (std::map<uint64_t, std::vector<double>>{{0, {}}, {1, {}}}));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>>{
            {{inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL,
              DemTarget::relative_detector_id(inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL)},
             {GateTarget::x(1)}},
            {{inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL, DemTarget::observable_id(5)},
             {GateTarget::x(1)}},
        }));

    slice_set = DetectorSliceSet::from_circuit_ticks(
        circuit,
        inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL,
        2,
        std::vector<CoordFilter>{empty_filter, obs5_filter});
    ASSERT_EQ(slice_set.coordinates, (std::map<uint64_t, std::vector<double>>{{0, {}}, {1, {}}}));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>>{
            {{inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL,
              DemTarget::relative_detector_id(inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL)},
             {GateTarget::x(1)}},
            {{inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL, DemTarget::observable_id(5)},
             {GateTarget::x(1)}},
            {{inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 2ULL,
              DemTarget::relative_detector_id(inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL)},
             {GateTarget::y(0)}},
        }));
}

TEST(detector_slice_set_text_diagram, repetition_code) {
    CoordFilter empty_filter;
    CoordFilter obs_filter;
    obs_filter.use_target = true;
    obs_filter.exact_target = DemTarget::observable_id(0);
    CircuitGenParameters params(10, 5, "memory");
    auto circuit = generate_rep_code_circuit(params).circuit;
    auto slice_set =
        DetectorSliceSet::from_circuit_ticks(circuit, 9, 1, std::vector<CoordFilter>{empty_filter, obs_filter});
    ASSERT_EQ(slice_set.slices.size(), circuit.count_qubits());
    ASSERT_EQ("\n" + slice_set.str() + "\n", R"DIAGRAM(
q0: --------Z:D12----------------------------
            |
q1: -Z:D8---Z:D12----------------------------
            |
q2: --------Z:D12--Z:D13---------------------
                   |
q3: -Z:D9----------Z:D13---------------------
                   |
q4: ---------------Z:D13--Z:D14--------------
                          |
q5: -Z:D10----------------Z:D14--------------
                          |
q6: ----------------------Z:D14--Z:D15-------
                                 |
q7: -Z:D11-----------------------Z:D15-------
                                 |
q8: -----------------------------Z:D15--Z:L0-
)DIAGRAM");

    ASSERT_EQ(
        "\n" +
            DetectorSliceSet::from_circuit_ticks(circuit, 11, 1, std::vector<CoordFilter>{empty_filter, obs_filter})
                .str() +
            "\n",
        R"DIAGRAM(
q0: --------Z:D16-
            |
q1: -Z:D12--Z:D16-
     |
q2: -Z:D12--Z:D17-
            |
q3: -Z:D13--Z:D17-
     |
q4: -Z:D13--Z:D18-
            |
q5: -Z:D14--Z:D18-
     |
q6: -Z:D14--Z:D19-
            |
q7: -Z:D15--Z:D19-
     |
q8: -Z:D15--Z:L0--
)DIAGRAM");
}

TEST(detector_slice_set_text_diagram, surface_code) {
    CoordFilter empty_filter;
    CoordFilter obs_filter;
    obs_filter.use_target = true;
    obs_filter.exact_target = DemTarget::observable_id(0);
    CircuitGenParameters params(10, 2, "unrotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto slice_set =
        DetectorSliceSet::from_circuit_ticks(circuit, 11, 1, std::vector<CoordFilter>{empty_filter, obs_filter});
    ASSERT_EQ(slice_set.slices.size(), circuit.count_qubits());
    ASSERT_EQ("\n" + slice_set.str() + "\n", R"DIAGRAM(
q0:(0, 0) -X:D2--Z:D3--------------------------------Z:L0-
           |     |                                   |
q1:(1, 0) -X:D2--|-----------------X:D6--Z:D7--------Z:L0-
           |     |                 |     |           |
q2:(2, 0) -|-----|-----Z:D4--------X:D6--|-----------Z:L0-
           |     |     |           |     |
q3:(0, 1) -X:D2--Z:D3--|-----------|-----Z:D7-------------
                       |           |     |
q4:(1, 1) -------------Z:D4--X:D5--X:D6--Z:D7-------------
                       |     |           |
q5:(2, 1) -------------Z:D4--|-----------|-----Z:D8--X:D9-
                       |     |           |     |     |
q6:(0, 2) -------------|-----X:D5--------Z:D7--|-----|----
                       |     |                 |     |
q7:(1, 2) -------------Z:D4--X:D5--------------|-----X:D9-
                                               |     |
q8:(2, 2) -------------------------------------Z:D8--X:D9-
)DIAGRAM");
}

TEST(detector_slice_set_svg_diagram, surface_code) {
    CoordFilter empty_filter;
    CircuitGenParameters params(10, 2, "rotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, 8, 1, {&empty_filter});
    std::stringstream ss;
    slice_set.write_svg_diagram_to(ss);
    expect_string_is_identical_to_saved_file(ss.str(), "rotated_memory_z_detector_slice.svg");
}

TEST(detector_slice_set_svg_diagram, long_range_detector) {
    Circuit circuit(R"CIRCUIT(
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 1) 1
        QUBIT_COORDS(2, 2) 2
        QUBIT_COORDS(3, 3) 3
        QUBIT_COORDS(4, 4) 4
        QUBIT_COORDS(5, 5) 5
        QUBIT_COORDS(6, 6) 6
        QUBIT_COORDS(7, 7) 7
        QUBIT_COORDS(8, 8) 8
        QUBIT_COORDS(9, 9) 9
        H 0 1 2 3 4 5 6 7 8 9
        TICK
        MX 0 9
        DETECTOR(0) rec[-1] rec[-2]
    )CIRCUIT");
    CoordFilter empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, 1, 1, {&empty_filter});
    std::stringstream ss;
    slice_set.write_svg_diagram_to(ss);
    expect_string_is_identical_to_saved_file(ss.str(), "long_range_detector.svg");
}

TEST(detector_slice_set, pick_polygon_center) {
    std::vector<Coord<2>> coords;
    coords.push_back({2, 1});
    coords.push_back({4.5, 3.5});
    coords.push_back({6, 0});
    coords.push_back({1, 5});
    ASSERT_EQ(pick_polygon_center(coords), (Coord<2>{3.25, 2.25}));

    coords.pop_back();
    coords.push_back({1, 6});
    ASSERT_EQ(pick_polygon_center(coords), (Coord<2>{(2 + 4.5 + 6 + 1) / 4, (1 + 3.5 + 0 + 6) / 4}));

    coords.push_back({7, 0});
    coords.push_back({1, 5});
    ASSERT_EQ(pick_polygon_center(coords), (Coord<2>{3.25, 2.25}));
}

TEST(detector_slice_set_svg_diagram, is_colinear) {
    ASSERT_TRUE(is_colinear({0, 0}, {0, 0}, {1, 2}, 1e-4f));
    ASSERT_TRUE(is_colinear({3, 6}, {1, 2}, {2, 4}, 1e-4f));

    ASSERT_FALSE(is_colinear({3, 7}, {1, 2}, {2, 4}, 1e-4f));
    ASSERT_FALSE(is_colinear({4, 6}, {1, 2}, {2, 4}, 1e-4f));
    ASSERT_FALSE(is_colinear({3, 6}, {1, 3}, {2, 4}, 1e-4f));
    ASSERT_FALSE(is_colinear({3, 6}, {2, 2}, {2, 4}, 1e-4f));
    ASSERT_FALSE(is_colinear({3, 6}, {1, 2}, {1, 4}, 1e-4f));
    ASSERT_FALSE(is_colinear({3, 6}, {1, 2}, {2, -4}, 1e-4f));

    ASSERT_FALSE(is_colinear({0, 1e-3f}, {0, 0}, {1, 2}, 1e-4f));
    ASSERT_TRUE(is_colinear({0, 1e-3f}, {0, 0}, {1, 2}, 1e-2f));
}

TEST(detector_slice_set_svg_diagram, colinear_polygon) {
    Circuit circuit(R"CIRCUIT(
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 1) 1
        QUBIT_COORDS(2, 2) 2
        QUBIT_COORDS(0, 3) 3
        QUBIT_COORDS(4, 0) 4
        QUBIT_COORDS(5, 1) 5
        QUBIT_COORDS(6, 2) 6
        R 0 1 2 3 4 5 6
        H 3
        TICK
        H 3
        M 0 1 2 3 4 5 6
        DETECTOR rec[-1] rec[-2] rec[-3]
        DETECTOR rec[-4] rec[-5] rec[-6] rec[-7]
    )CIRCUIT");
    CoordFilter empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, 1, 1, {&empty_filter});
    std::stringstream ss;
    slice_set.write_svg_diagram_to(ss);
    expect_string_is_identical_to_saved_file(ss.str(), "colinear_detector_slice.svg");
}

TEST(detector_slice_set_svg_diagram, observable) {
    Circuit circuit(R"CIRCUIT(
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(2, 0) 1
        QUBIT_COORDS(1, 1) 2
        REPEAT 3 {
            C_XYZ 0 1
            TICK
            CX 0 2
            TICK
            CX 1 2
            TICK
            M 2
            TICK
            R 2
        }
        DETECTOR rec[-1] rec[-2] rec[-3]
        M 0 1
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
    )CIRCUIT");
    CoordFilter obs_filter;
    obs_filter.use_target = true;
    obs_filter.exact_target = DemTarget::observable_id(0);
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit,
        ss,
        0,
        circuit.count_ticks() + 1,
        DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE,
        {&obs_filter});
    expect_string_is_identical_to_saved_file(ss.str(), "observable_slices.svg");
}

TEST(detector_slice_set_svg_diagram, svg_ids) {
    Circuit circuit(R"CIRCUIT(
        QUBIT_COORDS 0
        QUBIT_COORDS(1) 1
        QUBIT_COORDS(2, 2) 2
        QUBIT_COORDS(3, 3, 3) 3
        R 0 1 2 3
        TICK
        M 0 1 2 3
        DETECTOR rec[-1]
        DETECTOR(1) rec[-1] rec[-2]
        DETECTOR(2, 2) rec[-1] rec[-2] rec[-3]
        DETECTOR(3, 3, 3) rec[-1] rec[-2] rec[-3] rec[-4]
    )CIRCUIT");
    CoordFilter empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, 1, 1, {&empty_filter});
    std::stringstream ss;
    slice_set.write_svg_diagram_to(ss);
    expect_string_is_identical_to_saved_file(ss.str(), "svg_ids.svg");
}

TEST(coord_filter, parse_from) {
    auto c = CoordFilter::parse_from("");
    ASSERT_TRUE(c.coordinates.empty());
    ASSERT_FALSE(c.use_target);

    c = CoordFilter::parse_from("D5");
    ASSERT_TRUE(c.coordinates.empty());
    ASSERT_TRUE(c.use_target);
    ASSERT_EQ(c.exact_target, DemTarget::relative_detector_id(5));

    c = CoordFilter::parse_from("L7");
    ASSERT_TRUE(c.coordinates.empty());
    ASSERT_TRUE(c.use_target);
    ASSERT_EQ(c.exact_target, DemTarget::observable_id(7));

    c = CoordFilter::parse_from("2,3,*,5");
    ASSERT_TRUE(c.coordinates.size() == 4);
    ASSERT_TRUE(c.coordinates[0] == 2);
    ASSERT_TRUE(c.coordinates[1] == 3);
    ASSERT_TRUE(std::isnan(c.coordinates[2]));
    ASSERT_TRUE(c.coordinates[3] == 5);
    ASSERT_FALSE(c.use_target);
}

TEST(inv_space_fill_transform, inv_space_fill_transform) {
    ASSERT_EQ(inv_space_fill_transform({0, 0}), 0);
    ASSERT_EQ(inv_space_fill_transform({4, 55.5}), 339946);
}

TEST(detector_slice_set, from_circuit_with_errors) {
    CoordFilter empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(
        stim::Circuit(R"CIRCUIT(
            TICK
            R 0
            TICK
            R 0
            TICK
            MXX 0 1
            DETECTOR rec[-1]
        )CIRCUIT"),
        0,
        5,
        {&empty_filter});
    ASSERT_EQ(
        slice_set.anticommutations,
        (std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>>{
            {{2, DemTarget::relative_detector_id(0)}, {GateTarget::x(0)}},
        }));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>>{
            {{3, DemTarget::relative_detector_id(0)}, {GateTarget::x(0), GateTarget::x(1)}},
        }));
}

TEST(circuit_diagram_timeline_text, anticommuting_detector_circuit) {
    auto circuit = Circuit(R"CIRCUIT(
        TICK
        R 0
        TICK
        R 0
        TICK
        MXX 0 1
        M 2
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    )CIRCUIT");
    CoordFilter empty_filter;
    std::stringstream ss;
    ss << DetectorSliceSet::from_circuit_ticks(circuit, 0, 10, &empty_filter);
    ASSERT_EQ("\n" + ss.str() + "\n", R"DIAGRAM(
q0: -------ANTICOMMUTED:D1--X:D1-
                            |
q1: ------------------------X:D1-

q2: -Z:D0--Z:D0-------------Z:D0-
)DIAGRAM");
}
