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

#include "gtest/gtest.h"

#include "stim/main_namespaced.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST(command_diagram, run_captured_stim_main) {
    ASSERT_EQ(
        trim(run_captured_stim_main(
            {
                "diagram",
                "--type",
                "timeline-text",
            },
            R"input(
                H 0
                CNOT 0 1
            )input")),
        trim(R"output(
q0: -H-@-
       |
q1: ---X-
            )output"));
}

TEST(command_diagram, run_captured_stim_main_detector_slice) {
    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"diagram", "--type", "detector-slice-text", "--tick", "1"},
            R"input(
                H 0
                CNOT 0 1 0 2
                TICK
                M 0 1 2
                DETECTOR(4,5) rec[-1] rec[-2]
                DETECTOR(6) rec[-2] rec[-3]
            )input")),
        trim(R"output(
q0: -------Z:D1-
           |
q1: -Z:D0--Z:D1-
     |
q2: -Z:D0-------
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"diagram", "--type", "detector-slice-text", "--tick", "1", "--filter_coords", "4"},
            R"input(
                H 0
                CNOT 0 1 0 2
                TICK
                M 0 1 2
                DETECTOR(4,5) rec[-1] rec[-2]
                DETECTOR(6) rec[-2] rec[-3]
            )input")),
        trim(R"output(
q0: ------

q1: -Z:D0-
     |
q2: -Z:D0-
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"diagram", "--type", "detector-slice-text", "--tick", "1", "--filter_coords", "5,6,7:6:7,8"},
            R"input(
                H 0
                CNOT 0 1 0 2
                TICK
                M 0 1 2
                DETECTOR(4,5) rec[-1] rec[-2]
                DETECTOR(6) rec[-2] rec[-3]
            )input")),
        trim(R"output(
q0: -Z:D1-
     |
q1: -Z:D1-

q2: ------
            )output"));
}

TEST(command_diagram, run_captured_stim_main_timeline_ticking) {
    auto circuit = R"input(
        R 0 1
        TICK
        H 0
        CNOT 0 1
        TICK
        S 0
        TICK
        H 0
        M 0 1
    )input";
    auto result_txt = run_captured_stim_main({"diagram", "--type", "timeline-text"}, circuit);
    ASSERT_EQ("\n" + result_txt, R"DIAGRAM(
       /-\   /--------\
q0: -R-H-@-S-H-M:rec[0]-
         |
q1: -R---X-----M:rec[1]-
       \-/   \--------/
)DIAGRAM");

    auto result = run_captured_stim_main({"diagram", "--type", "timeline-svg"}, circuit);
    expect_string_is_identical_to_saved_file(result, "command_diagram_timeline.svg");

    result = run_captured_stim_main({"diagram", "--type", "timeline-svg", "--tick", "0"}, circuit);
    expect_string_is_identical_to_saved_file(result, "command_diagram_timeline_tick0.svg");

    result = run_captured_stim_main({"diagram", "--type", "timeline-svg", "--tick", "1"}, circuit);
    expect_string_is_identical_to_saved_file(result, "command_diagram_timeline_tick1.svg");

    result = run_captured_stim_main({"diagram", "--type", "timeline-svg", "--tick", "2"}, circuit);
    expect_string_is_identical_to_saved_file(result, "command_diagram_timeline_tick2.svg");

    result = run_captured_stim_main({"diagram", "--type", "timeline-svg", "--tick", "1:3"}, circuit);
    expect_string_is_identical_to_saved_file(result, "command_diagram_timeline_tick1_3.svg");
}

TEST(command_diagram, run_captured_stim_main_works_various_arguments) {
    std::vector<std::string> diagram_types{
        "timeline-text",
        "timeline-svg",
        "timeline-3d",
        "timeline-3d-html",
        "match-graph-svg",
        "match-graph-3d",
        "match-graph-3d-html",
        "detector-slice-text",
        "detector-slice-svg",
        "time-slice-svg",
        "time+detector-slice-svg",
    };
    for (const auto &type : diagram_types) {
        auto actual = run_captured_stim_main(
            {
                "diagram",
                "--type",
                type.c_str(),
                "--tick",
                "1:2",
            },
            R"input(
            H 0
            CNOT 0 1
            X_ERROR(0.125) 0
            TICK
            M 0 1
            DETECTOR(1, 2, 3) rec[-1] rec[-2]
        )input");
        if (actual.find("[stderr") != std::string::npos) {
            EXPECT_TRUE(false) << actual;
        }
        EXPECT_NE(actual, "") << type;
    }
}

TEST(command_diagram, warn_about_noiseless_match_graphs) {
    auto result = run_captured_stim_main({"diagram", "--type", "matchgraph-3d", "--remove_noise"}, "H 0");
    ASSERT_NE(result.find("--remove_noise is incompatible"), std::string::npos);

    result = run_captured_stim_main({"diagram", "--type", "matchgraph-3d"}, "H 0");
    ASSERT_NE(
        result.find("[stderr=Warning: the detector error model derived from the circuit had no errors"),
        std::string::npos);
}
