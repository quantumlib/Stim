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
#include "stim/test_util.test.h"

using namespace stim;

TEST(command_m2d, m2d) {
    RaiiTempNamedFile tmp;
    tmp.write_contents(R"CIRCUIT(
        X 0
        M 0 1
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(2) rec[-1]
    )CIRCUIT");

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data(), "--append_observables"},
            "00\n01\n10\n11\n")),
        trim(R"output(
shot D0
shot D0 D1 L2
shot
shot D1 L2
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data()}, "00\n01\n10\n11\n")),
        trim(R"output(
shot D0
shot D0 D1
shot
shot D1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data(), "--skip_reference_sample"},
            "00\n01\n10\n11\n")),
        trim(R"output(
shot
shot D1
shot D0
shot D0 D1
            )output"));
}
