// Copyright 2023 Google LLC
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

TEST(command_convert, convert_measurements_with_circuit_to_dets) {
    RaiiTempNamedFile tmp(R"CIRCUIT(
        X 0
        M 0 1
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(2) rec[-1]
      )CIRCUIT");

    std::vector<std::tuple<std::string, std::string>> measurement_data{
        std::make_tuple("01", "00\n01\n10\n11\n"),
        std::make_tuple("b8", std::string({0x00, 0x02, 0x01, 0x03})),
        std::make_tuple("hits", "\n1\n0\n0,1\n"),
        std::make_tuple("r8", std::string({0x02, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}))};

    for (const auto& [in_format, in_data] : measurement_data) {
        ASSERT_EQ(
            run_captured_stim_main(
                {"convert",
                 "--in_format",
                 in_format.c_str(),
                 "--out_format",
                 "dets",
                 "--circuit",
                 tmp.path.c_str(),
                 "--types=M"},
                in_data),
            "shot\nshot M1\nshot M0\nshot M0 M1\n");
    }
}

TEST(command_convert, convert_detections_observables_with_circuit_to_dets) {
    RaiiTempNamedFile tmp(R"CIRCUIT(
        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1]
        TICK

        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1] rec[-2]
        TICK

        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1] rec[-2]
        TICK

        M 0 1
        DETECTOR rec[-1] rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");

    std::vector<std::tuple<std::string, std::string>> detection_data{
        std::make_tuple("01", "00000\n11000\n01100\n00110\n00010\n00011\n"),
        std::make_tuple("b8", std::string({0x00, 0x03, 0x06, 0x0c, 0x08, 0x18})),
        std::make_tuple("hits", "\n0,1\n1,2\n2,3\n3\n3,4\n"),
        std::make_tuple(
            "r8",
            std::string({0x05, 0x00, 0x00, 0x03, 0x01, 0x00, 0x02, 0x02, 0x00, 0x01, 0x03, 0x01, 0x03, 0x00, 0x00}))};

    for (const auto& [in_format, in_data] : detection_data) {
        ASSERT_EQ(
            run_captured_stim_main(
                {"convert",
                 "--in_format",
                 in_format.c_str(),
                 "--out_format",
                 "dets",
                 "--circuit",
                 tmp.path.c_str(),
                 "--types=DL"},
                in_data),
            "shot\nshot D0 D1\nshot D1 D2\nshot D2 D3\nshot D3\nshot D3 L0\n");
    }
}

TEST(command_convert, convert_detections_observables_with_circuit_to_dets_with_obs_out) {
    RaiiTempNamedFile tmp(R"CIRCUIT(
        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1]
        TICK

        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1] rec[-2]
        TICK

        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1] rec[-2]
        TICK

        M 0 1
        DETECTOR rec[-1] rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");
    RaiiTempNamedFile tmp_obs;

    std::vector<std::tuple<std::string, std::string>> detection_data{
        std::make_tuple("01", "00000\n11000\n01100\n00110\n00010\n00011\n"),
        std::make_tuple("b8", std::string({0x00, 0x03, 0x06, 0x0c, 0x08, 0x18})),
        std::make_tuple("hits", "\n0,1\n1,2\n2,3\n3\n3,4\n"),
        std::make_tuple(
            "r8",
            std::string({0x05, 0x00, 0x00, 0x03, 0x01, 0x00, 0x02, 0x02, 0x00, 0x01, 0x03, 0x01, 0x03, 0x00, 0x00}))};

    for (const auto& [in_format, in_data] : detection_data) {
        ASSERT_EQ(
            run_captured_stim_main(
                {"convert",
                 "--in_format",
                 in_format.c_str(),
                 "--out_format",
                 "dets",
                 "--obs_out_format",
                 "dets",
                 "--obs_out",
                 tmp_obs.path.c_str(),
                 "--circuit",
                 tmp.path.c_str(),
                 "--types=DL"},
                in_data),
            "shot\nshot D0 D1\nshot D1 D2\nshot D2 D3\nshot D3\nshot D3\n");
        ASSERT_EQ(tmp_obs.read_contents(), "shot\nshot\nshot\nshot\nshot\nshot L0\n");
    }
}

TEST(command_convert, convert_detections_observables_with_circuit_no_dets) {
    RaiiTempNamedFile tmp(R"CIRCUIT(
      R 0 1 2 3 4
      TICK
      CX 0 1 2 3
      DEPOLARIZE2(0.3) 0 1 2 3
      TICK
      CX 2 1 4 3
      DEPOLARIZE2(0.3) 2 1 4 3
      TICK
      MR 1 3
      DETECTOR(1, 0) rec[-2]
      DETECTOR(3, 0) rec[-1]
      M 0 2 4
      DETECTOR(1, 1) rec[-2] rec[-3] rec[-5]
      DETECTOR(3, 1) rec[-1] rec[-2] rec[-4]
      OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");

    std::vector<std::tuple<std::string, std::string>> detection_data{
        std::make_tuple("01", "10100\n00011\n00000\n00100\n00000\n10000\n"),
        std::make_tuple("b8", std::string({0x05, 0x18, 0x00, 0x04, 0x00, 0x01})),
        std::make_tuple("hits", "0,2\n3,4\n\n2\n\n0\n"),
        std::make_tuple("r8", std::string({0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x05, 0x02, 0x02, 0x05, 0x00, 0x04}))};

    for (const auto& [in_format, in_data] : detection_data) {
        for (const auto& [out_format, out_data] : detection_data) {
            ASSERT_EQ(
                run_captured_stim_main(
                    {"convert",
                     "--in_format",
                     in_format.c_str(),
                     "--out_format",
                     out_format.c_str(),
                     "--circuit",
                     tmp.path.c_str(),
                     "--types=DL"},
                    in_data),
                out_data);
        }
    }
}

TEST(command_convert, convert_detections_observables_with_dem) {
    RaiiTempNamedFile tmp(R"DEM(
        detector D0
        detector D1
        logical_observable L2
      )DEM");

    std::vector<std::tuple<std::string, std::string>> detection_data{
        std::make_tuple("01", "10000\n11001\n00000\n01001\n"),
        std::make_tuple("b8", std::string({0x01, 0x13, 0x00, 0x12})),
        std::make_tuple("dets", "shot D0\nshot D0 D1 L2\nshot\nshot D1 L2\n"),
        std::make_tuple("hits", "0\n0,1,4\n\n1,4\n"),
        std::make_tuple("r8", std::string({0x00, 0x04, 0x00, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x00}))};

    for (const auto& [in_format, in_data] : detection_data) {
        for (const auto& [out_format, out_data] : detection_data) {
            ASSERT_EQ(
                run_captured_stim_main(
                    {"convert",
                     "--in_format",
                     in_format.c_str(),
                     "--out_format",
                     out_format.c_str(),
                     "--dem",
                     tmp.path.c_str()},
                    in_data),
                out_data);
        }
    }
}

TEST(command_convert, convert_measurements_no_circuit_or_dem) {
    std::vector<std::tuple<std::string, std::string>> measurement_data{
        std::make_tuple("01", "100\n010\n110\n001\n010\n111\n"),
        std::make_tuple("b8", std::string({0x01, 0x02, 0x03, 0x04, 0x02, 0x07})),
        std::make_tuple("hits", "0\n1\n0,1\n2\n1\n0,1,2\n"),
        std::make_tuple("dets", "shot M0\nshot M1\nshot M0 M1\nshot M2\nshot M1\nshot M0 M1 M2\n"),
        std::make_tuple(
            "r8",
            std::string({0x00, 0x02, 0x01, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}))};

    for (const auto& [in_format, in_data] : measurement_data) {
        for (const auto& [out_format, out_data] : measurement_data) {
            ASSERT_EQ(
                run_captured_stim_main(
                    {"convert",
                     "--in_format",
                     in_format.c_str(),
                     "--out_format",
                     out_format.c_str(),
                     "--num_measurements",
                     "3"},
                    in_data),
                out_data);
        }
    }
}

TEST(command_convert, convert_detections_observables_no_circuit_or_dem) {
    std::vector<std::tuple<std::string, std::string>> detection_data{
        std::make_tuple("01", "10000\n11001\n00000\n01001\n"),
        std::make_tuple("b8", std::string({0x01, 0x13, 0x00, 0x12})),
        std::make_tuple("dets", "shot D0\nshot D0 D1 L2\nshot\nshot D1 L2\n"),
        std::make_tuple("hits", "0\n0,1,4\n\n1,4\n"),
        std::make_tuple("r8", std::string({0x00, 0x04, 0x00, 0x00, 0x02, 0x00, 0x05, 0x01, 0x02, 0x00}))};

    for (const auto& [in_format, in_data] : detection_data) {
        for (const auto& [out_format, out_data] : detection_data) {
            ASSERT_EQ(
                run_captured_stim_main(
                    {"convert",
                     "--in_format",
                     in_format.c_str(),
                     "--out_format",
                     out_format.c_str(),
                     "--num_detectors",
                     "2",
                     "--num_observables",
                     "3"},
                    in_data),
                out_data);
        }
    }
}

TEST(command_convert, convert_bits_per_shot_no_dets) {
    std::vector<std::tuple<std::string, std::string>> measurement_data{
        std::make_tuple("01", "00\n01\n10\n11\n"),
        std::make_tuple("b8", std::string({0x00, 0x02, 0x01, 0x03})),
        std::make_tuple("hits", "\n1\n0\n0,1\n"),
        std::make_tuple("r8", std::string({0x02, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}))};

    for (const auto& [in_format, in_data] : measurement_data) {
        for (const auto& [out_format, out_data] : measurement_data) {
            ASSERT_EQ(
                run_captured_stim_main(
                    {"convert",
                     "--in_format",
                     in_format.c_str(),
                     "--out_format",
                     out_format.c_str(),
                     "--bits_per_shot=2"},
                    in_data),
                out_data);
        }
    }
}

TEST(command_convert, convert_multiple_bitword_sized_records) {
    ASSERT_EQ(
        run_captured_stim_main(
            {"convert", "--in_format=b8", "--out_format=b8", "--bits_per_shot=2048"}, std::string(256, 0x6b)),
        std::string(256, 0x6b));
}

TEST(command_convert, convert_circuit_fail_without_types) {
    RaiiTempNamedFile tmp(R"CIRCUIT(
        X 0
        M 0 1
      )CIRCUIT");

    ASSERT_TRUE(matches(
        run_captured_stim_main(
            {"convert",
             "--in_format=01",
             "--out_format",
             "dets",
             "--circuit",
             tmp.path.c_str(),
             "--num_measurements=2"},
            ""),
        ".*--types required when passing circuit.*"));
}

TEST(command_convert, convert_fail_without_any_information) {
    ASSERT_TRUE(matches(
        run_captured_stim_main({"convert", "--in_format=r8", "--out_format=b8"}, ""),
        ".*Not enough information given to parse input file.*"));
}

TEST(command_convert, convert_fail_with_bits_per_shot_to_dets) {
    ASSERT_TRUE(matches(
        run_captured_stim_main({"convert", "--in_format=01", "--out_format", "dets", "--bits_per_shot=2"}, ""),
        ".*Not enough information given to parse input file to write to dets.*"));
}

TEST(command_convert, convert_invalid_types) {
    RaiiTempNamedFile tmp("");

    ASSERT_TRUE(matches(
        run_captured_stim_main(
            {"convert", "--in_format=dets", "--out_format=dets", "--circuit", tmp.path.c_str(), "--types=N"}, ""),
        ".*Unknown type passed to --types.*"));

    ASSERT_TRUE(matches(
        run_captured_stim_main(
            {"convert", "--in_format=dets", "--out_format=dets", "--circuit", tmp.path.c_str(), "--types=MM"}, ""),
        ".*Each type in types should only be specified once.*"));
}
