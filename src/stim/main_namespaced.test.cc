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

#include "stim/main_namespaced.h"

#include <regex>

#include "gtest/gtest.h"

#include "stim/main_namespaced.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

std::string stim::run_captured_stim_main(std::vector<const char *> flags, std::string_view std_in_content) {
    // Setup input.
    RaiiTempNamedFile raii_temp_file(std_in_content);
    flags.push_back("--in");
    flags.push_back(raii_temp_file.path.data());

    return run_captured_stim_main(flags);
}

std::string stim::run_captured_stim_main(std::vector<const char *> flags) {
    flags.insert(flags.begin(), "[PROGRAM_LOCATION_IGNORE]");

    // Setup output.
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    int result;
    try {
        result = stim::main(flags.size(), flags.data());
    } catch (const std::exception &e) {
        return "[exception=" + std::string(e.what()) + "]" + testing::internal::GetCapturedStdout() +
               testing::internal::GetCapturedStderr();
    }
    std::string out = testing::internal::GetCapturedStdout();
    std::string err = testing::internal::GetCapturedStderr();
    if (!err.empty()) {
        return out + "[stderr=" + err + "]";
    }
    if (result != EXIT_SUCCESS) {
        return "[exit code != EXIT_SUCCESS]";
    }

    return out;
}

std::string_view stim::trim(std::string_view text) {
    size_t s = 0;
    size_t e = text.size();
    while (s < e && std::isspace(text[s])) {
        s++;
    }
    while (s < e && std::isspace(text[e - 1])) {
        e--;
    }
    return text.substr(s, e - s);
}

bool stim::matches(std::string actual, std::string pattern) {
    // Hackily work around C++ regex not supporting multiline matching.
    std::replace(actual.begin(), actual.end(), '\n', 'X');
    std::replace(pattern.begin(), pattern.end(), '\n', 'X');
    return std::regex_match(actual, std::regex("^" + pattern + "$"));
}

TEST(main, help_modes) {
    ASSERT_TRUE(matches(run_captured_stim_main({"--help"}), ".*Available stim commands.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"help"}), ".*Available stim commands.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({}), ".+stderr.+No mode.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--sample", "--repl"}), ".+stderr.+More than one mode.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--sample", "--repl", "--detect"}), ".+stderr.+More than one mode.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--help", "dhnsahddjoidsa"}), ".*Unrecognized.*"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--help", "H"}), ".+Hadamard.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--help", "sample"}), ".*Samples measurements from a circuit.+"));
}

TEST(main, bad_flag) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--gen", "--unknown"})),
        trim(
            "[stderr=\033[31mUnrecognized command line argument --unknown for `stim gen`.\n"
            "Recognized command line arguments for `stim gen`:\n"
            "    --after_clifford_depolarization\n"
            "    --after_reset_flip_probability\n"
            "    --before_measure_flip_probability\n"
            "    --before_round_data_depolarization\n"
            "    --code\n"
            "    --distance\n"
            "    --in\n"
            "    --out\n"
            "    --rounds\n"
            "    --task\n"
            "\033[0m]\n"));
}
