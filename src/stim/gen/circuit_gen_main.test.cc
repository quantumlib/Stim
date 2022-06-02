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

#include "stim/gen/gen_color_code.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/simulators/detection_simulator.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST(circuit_gen_main, no_noise_no_detections) {
    std::vector<uint32_t> distances{2, 3, 4, 5, 6, 7, 15};
    std::vector<uint32_t> rounds{1, 2, 3, 4, 5, 6, 20};
    std::map<std::string, std::pair<std::string, GeneratedCircuit (*)(const CircuitGenParameters &)>> funcs{
        {"color", {"memory_xyz", &generate_color_code_circuit}},
        {"surface", {"unrotated_memory_x", &generate_surface_code_circuit}},
        {"surface", {"unrotated_memory_z", &generate_surface_code_circuit}},
        {"surface", {"rotated_memory_x", &generate_surface_code_circuit}},
        {"surface", {"rotated_memory_z", &generate_surface_code_circuit}},
        {"rep", {"memory", &generate_rep_code_circuit}},
    };
    for (const auto &func : funcs) {
        for (auto d : distances) {
            for (auto r : rounds) {
                if (func.first == "color" && (r < 2 || d % 2 == 0 || d < 3)) {
                    continue;
                }
                CircuitGenParameters params(r, d, func.second.first);
                auto samples =
                    detector_samples(func.second.second(params).circuit, 256, false, true, SHARED_TEST_RNG());
                EXPECT_FALSE(samples.data.not_zero())
                    << "d=" << d << ", r=" << r << ", task=" << func.second.first << ", func=" << func.first;
            }
        }
    }
}
