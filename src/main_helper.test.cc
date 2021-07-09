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

#include "main_helper.h"

#include <gtest/gtest.h>
#include <regex>

#include "test_util.test.h"

using namespace stim_internal;

std::string execute(std::vector<const char *> flags, const char *std_in_content) {
    // Setup input.
    RaiiTempNamedFile raii_temp_file;
    if (std_in_content != nullptr) {
        FILE *tmp_in = fdopen(raii_temp_file.descriptor, "w");
        if (tmp_in == nullptr) {
            throw std::runtime_error("Failed to open temporary stdin file.");
        }
        fprintf(tmp_in, "%s", std_in_content);
        fclose(tmp_in);
        flags.insert(flags.begin(), raii_temp_file.path.data());
        flags.insert(flags.begin(), "--in");
    }
    flags.insert(flags.begin(), "[PROGRAM_LOCATION_IGNORE]");

    // Setup output.
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    int result;
    try {
        result = main_helper(flags.size(), flags.data());
    } catch (const std::exception &e) {
        return "[exception=" + std::string(e.what()) + "]" + testing::internal::GetCapturedStdout() +
               testing::internal::GetCapturedStderr();
    }
    std::string out = testing::internal::GetCapturedStdout();
    std::string err = testing::internal::GetCapturedStderr();
    if (!err.empty()) {
        return "[stderr=" + err + "]";
    }
    if (result != EXIT_SUCCESS) {
        return "[exit code != EXIT_SUCCESS]";
    }

    return out;
}
