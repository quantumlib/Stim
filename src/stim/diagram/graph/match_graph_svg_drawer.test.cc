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

#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/test_util.test.h"
#include "match_graph_3d_drawer.h"

using namespace stim;
using namespace stim_draw_internal;

void expect_graph_svg_diagram_is_identical_to_saved_file(const DetectorErrorModel &dem, std::string key) {
    std::stringstream actual_ss;
    dem_match_graph_to_svg_diagram_write_to(dem, actual_ss);
    auto actual = actual_ss.str();

    auto path = resolve_test_file(key);
    FILE *f = fopen(path.c_str(), "r");
    auto expected = rewind_read_close(f);

    if (expected != actual) {
        auto dot = key.rfind('.');
        std::string new_path;
        if (dot == std::string::npos) {
            new_path = path + ".new";
        } else {
            dot += path.size() - key.size();
            new_path = path.substr(0, dot) + ".new" + path.substr(dot);
        }
        std::ofstream out;
        out.open(new_path);
        out << actual;
        out.close();
        EXPECT_TRUE(false) << "Diagram didn't agree. key=" << key;
    }
}

TEST(match_graph_drawer_svg, repetition_code) {
    CircuitGenParameters params(10, 7, "memory");
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_rep_code_circuit(params).circuit;
    auto dem = ErrorAnalyzer::circuit_to_detector_error_model(
        circuit,
        true,
        true,
        false,
        false,
        false,
        false);
    expect_graph_svg_diagram_is_identical_to_saved_file(dem, "match_graph_repetition_code.svg");
}

TEST(match_graph_drawer_svg, surface_code) {
    CircuitGenParameters params(10, 3, "unrotated_memory_z");
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto dem = ErrorAnalyzer::circuit_to_detector_error_model(
        circuit,
        true,
        true,
        false,
        false,
        false,
        false);
    expect_graph_svg_diagram_is_identical_to_saved_file(dem, "match_graph_surface_code.svg");
}
