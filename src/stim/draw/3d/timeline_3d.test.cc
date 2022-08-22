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

#include "stim/draw/3d/timeline_3d.h"

#include "gtest/gtest.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_surface_code.h"

using namespace stim;

TEST(circuit_diagram_timeline_3d, XXXXXXXXXXX) {
    CircuitGenParameters params(999999, 5, "unrotated_memory_z");
    Circuit circuit = generate_surface_code_circuit(params).circuit;

    auto s = circuit_diagram_timeline_3d(circuit);

    FILE *f = fopen("/home/craiggidney/tmp/test.gltf", "w");
    fprintf(f, "%s", s.data());
    fclose(f);
}
