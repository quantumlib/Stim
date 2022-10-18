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

#include "stim/main_namespaced.test.h"

#include "gtest/gtest.h"
#include "stim/test_util.test.h"

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
