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

#include "common_circuits.h"

#include <gtest/gtest.h>

#include "circuit.h"

TEST(common_circuits, unrotated_surface_code_program_text) {
    auto circuit = Circuit::from_text(unrotated_surface_code_program_text(5, 4, 0.001));
    ASSERT_EQ(circuit.detectors.size(), 5 * 4 * 2 * 4);
}
