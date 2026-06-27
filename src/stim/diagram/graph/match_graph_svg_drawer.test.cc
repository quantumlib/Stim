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

#include "stim/diagram/graph/match_graph_svg_drawer.h"

#include <fstream>

#include "gtest/gtest.h"

#include "match_graph_3d_drawer.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;
using namespace stim_draw_internal;

void expect_graph_svg_diagram_is_identical_to_saved_file(const DetectorErrorModel &dem, std::string_view key) {
    std::stringstream actual_ss;
    dem_match_graph_to_svg_diagram_write_to(dem, actual_ss);
    auto actual = actual_ss.str();
    expect_string_is_identical_to_saved_file(actual_ss.str(), key);
}

TEST(match_graph_drawer_svg, repetition_code) {
    CircuitGenParameters params(10, 7, "memory");
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_rep_code_circuit(params).circuit;
    auto dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, false, false, false);
    expect_graph_svg_diagram_is_identical_to_saved_file(dem, "match_graph_repetition_code.svg");
}

TEST(match_graph_drawer_svg, surface_code) {
    CircuitGenParameters params(10, 3, "unrotated_memory_z");
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, false, false, false);
    expect_graph_svg_diagram_is_identical_to_saved_file(dem, "match_graph_surface_code.svg");
}

TEST(match_graph_drawer_svg, missing_coordinates) {
    Circuit circuit(R"CIRCUIT(
        R 0 1 2 3 4 5 6 7 8 9 10
        X_ERROR(0.125) 0 1 2 3 4 5 6 7 8 9 10
        M 0 1 2 3 4 5 6 7 8 9 10
        DETECTOR rec[-1] rec[-2]
        DETECTOR rec[-2] rec[-3]
        DETECTOR rec[-3] rec[-4]
        DETECTOR rec[-4] rec[-5]
        DETECTOR rec[-5] rec[-6]
        DETECTOR rec[-6] rec[-7]
        DETECTOR rec[-7] rec[-8]
        DETECTOR rec[-8] rec[-9]
        DETECTOR rec[-9] rec[-10]
        OBSERVABLE_INCLUDE(1) rec[-1]
    )CIRCUIT");

    auto dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, false, false, false);
    std::stringstream ss;

    testing::internal::CaptureStderr();
    dem_match_graph_to_svg_diagram_write_to(dem, ss);
    std::string err = testing::internal::GetCapturedStderr();
    ASSERT_NE(err, "");

    expect_string_is_identical_to_saved_file(ss.str(), "match_graph_no_coords.svg");
}
