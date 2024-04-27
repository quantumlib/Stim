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

TEST(command_explain_errors, explain_errors) {
    ASSERT_EQ(run_captured_stim_main({"explain_errors"}, ""), "");

    RaiiTempNamedFile tmp("error(1) D0\n");

    ASSERT_EQ(
        trim(run_captured_stim_main({"explain_errors", "--dem_filter", tmp.path.c_str()}, R"input(
X_ERROR(0.25) 0 1
M 0 1
DETECTOR rec[-1]
DETECTOR rec[-2]
            )input")),
        trim(R"output(
ExplainedError {
    dem_error_terms: D0
    CircuitErrorLocation {
        flipped_pauli_product: X1
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #1 (X_ERROR) in the circuit
            at target #2 of the instruction
            resolving to X_ERROR(0.25) 1
    }
}
            )output"));
}
