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
#include "stim/test_util.test.h"

using namespace stim;

TEST(command_convert, convert_measurements_with_circuit) {
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
        std::make_tuple("dets", "shot\nshot M1\nshot M0\nshot M0 M1\n"),
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
                     "--circuit",
                     tmp.path.data(),
                     "--types=M"},
                    in_data),
                out_data);
        }
    }
}

TEST(command_convert, convert_detections_with_circuit) {
    RaiiTempNamedFile tmp(R"CIRCUIT(
        X 0
        M 0 1
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(2) rec[-1]
      )CIRCUIT");

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
                     "--circuit",
                     tmp.path.data(),
                     "--types=DL"},
                    in_data),
                out_data);
        }
    }
}

TEST(command_convert, convert_invalid_types) {
    RaiiTempNamedFile tmp("");

    ASSERT_TRUE(matches(
        run_captured_stim_main(
            {"convert", "--in_format=dets", "--out_format=dets", "--circuit", tmp.path.data(), "--types=N"}, ""),
        ".*Unknown type passed to --types.*"));

    ASSERT_TRUE(matches(
        run_captured_stim_main(
            {"convert", "--in_format=dets", "--out_format=dets", "--circuit", tmp.path.data(), "--types=MM"}, ""),
        ".*Each type in types should only be specified once.*"));
}
