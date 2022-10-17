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

#include <regex>

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(command_gen, execute) {
    ASSERT_TRUE(matches(
        trim(run_captured_stim_main({"--gen=repetition_code", "--rounds=3", "--distance=4", "--task=memory"}, "")),
        ".+Generated repetition_code.+"));
    ASSERT_TRUE(matches(
        trim(run_captured_stim_main({"--gen=surface_code", "--rounds=3", "--distance=2", "--task=unrotated_memory_z"}, "")),
        ".+Generated surface_code.+"));
    ASSERT_TRUE(matches(
        trim(run_captured_stim_main({"gen", "--code=surface_code", "--rounds=3", "--distance=2", "--task=unrotated_memory_z"}, "")),
        ".+Generated surface_code.+"));
    ASSERT_TRUE(matches(
        trim(run_captured_stim_main({"--gen=surface_code", "--rounds=3", "--distance=2", "--task=rotated_memory_x"}, "")),
        ".+Generated surface_code.+"));
    ASSERT_TRUE(matches(
        trim(run_captured_stim_main({"--gen=color_code", "--rounds=3", "--distance=3", "--task=memory_xyz"}, "")),
        ".+Generated color_code.+"));
}
