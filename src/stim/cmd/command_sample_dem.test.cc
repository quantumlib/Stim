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

TEST(main, sample_dem) {
    ASSERT_EQ(run_captured_stim_main({"sample_dem"}, ""), "\n");

    RaiiTempNamedFile obs_out;

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {
                "sample_dem",
                "--obs_out",
                obs_out.path.c_str(),
                "--out_format",
                "01",
                "--obs_out_format",
                "01",
                "--shots",
                "5",
                "--seed",
                "0",
            },
            R"input(
                error(0) D0
                error(1) D1 L2
            )input")),
        trim(R"output(
01
01
01
01
01
            )output"));
    ASSERT_EQ(obs_out.read_contents(), "001\n001\n001\n001\n001\n");
}
