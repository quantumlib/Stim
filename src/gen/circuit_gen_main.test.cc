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

#include "surface_code.h"
#include "rep_code.h"
#include "../simulators/detection_simulator.h"
#include "../test_util.test.h"

#include <gtest/gtest.h>

using namespace stim_internal;

TEST(circuit_gen_main, no_noise_no_detections) {
    std::vector<uint32_t> distances{2, 3, 4, 5, 6, 7, 15};
    std::vector<uint32_t> rounds{1, 2, 3, 20};
    std::vector<std::string> bases{"X", "Z"};
    std::map<std::string, GeneratedCircuit(*)(const CircuitGenParameters &)> funcs{
        {"unrotated_surface", &generate_unrotated_surface_code_circuit},
        {"rotated_surface", &generate_rotated_surface_code_circuit},
        {"rep", &generate_rep_code_circuit},
    };
    for (const auto &func : funcs) {
        for (auto d : distances) {
            for (auto r : rounds) {
                for (auto basis : bases) {
                    CircuitGenParameters params(r, d);
                    params.basis = basis;
                    if (basis == "X" && func.first == "rep") {
                        continue;
                    }
                    auto samples = detector_samples(func.second(params).circuit, 256, false, true, SHARED_TEST_RNG());
                    ASSERT_FALSE(samples.data.not_zero()) << "d=" << d << ", r=" << r << ", basis=" << basis << ", func=" << func.first;
                }
            }
        }
    }
}
