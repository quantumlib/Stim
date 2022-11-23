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

#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/test_util.test.h"

using namespace stim;
using namespace stim_draw_internal;

TEST(detector_slice_set, from_circuit) {
    std::vector<double> empty_filter;
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
    std::vector<double> empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, inner * 10000ULL * 50ULL + 2ULL, 1, {&empty_filter});
    ASSERT_EQ(slice_set.coordinates, (std::map<uint64_t, std::vector<double>>{{0, {}}, {1, {}}}));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<std::pair<uint64_t, stim::DemTarget>, std::vector<stim::GateTarget>>{
            {{inner * 10000ULL * 50ULL + 2ULL, DemTarget::relative_detector_id(inner * 10000ULL * 50ULL + 1ULL)},
             {GateTarget::y(0)}},
        }));

    slice_set = DetectorSliceSet::from_circuit_ticks(
        circuit, inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL, 1, {&empty_filter});
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
        circuit, inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL, 2, {&empty_filter});
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
    std::vector<double> empty_filter;
    CircuitGenParameters params(10, 5, "memory");
    auto circuit = generate_rep_code_circuit(params).circuit;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, 9, 1, {&empty_filter});
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

    ASSERT_EQ("\n" + DetectorSliceSet::from_circuit_ticks(circuit, 11, 1, {&empty_filter}).str() + "\n", R"DIAGRAM(
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
    std::vector<double> empty_filter;
    CircuitGenParameters params(10, 2, "unrotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, 11, 1, {&empty_filter});
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
    std::vector<double> empty_filter;
    CircuitGenParameters params(10, 2, "rotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, 8, 1, {&empty_filter});
    std::stringstream ss;
    slice_set.write_svg_diagram_to(ss);
    expect_string_is_identical_to_saved_file(ss.str(), "rotated_memory_z_detector_slice.svg");
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
    ASSERT_TRUE(is_colinear({0, 0}, {0, 0}, {1, 2}));
    ASSERT_TRUE(is_colinear({3, 6}, {1, 2}, {2, 4}));
    ASSERT_FALSE(is_colinear({3, 7}, {1, 2}, {2, 4}));
    ASSERT_FALSE(is_colinear({4, 6}, {1, 2}, {2, 4}));
    ASSERT_FALSE(is_colinear({3, 6}, {1, 3}, {2, 4}));
    ASSERT_FALSE(is_colinear({3, 6}, {2, 2}, {2, 4}));
    ASSERT_FALSE(is_colinear({3, 6}, {1, 2}, {1, 4}));
    ASSERT_FALSE(is_colinear({3, 6}, {1, 2}, {2, -4}));
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
    std::vector<double> empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_ticks(circuit, 1, 1, {&empty_filter});
    std::stringstream ss;
    slice_set.write_svg_diagram_to(ss);
    expect_string_is_identical_to_saved_file(ss.str(), "colinear_detector_slice.svg");
}
