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

#include "dem_sampler.h"

#include "stim/benchmark_util.perf.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/simulators/error_analyzer.h"

using namespace stim;

BENCHMARK(DemSampler_surface_code_rotated_memory_z_distance11_100rounds_1024stripes) {
    auto params = CircuitGenParameters(100, 11, "rotated_memory_z");
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, false, false, false);
    std::mt19937_64 rng(0);
    DemSampler sampler(dem, std::mt19937_64(0), 1024);
    size_t count = 0;
    benchmark_go([&]() {
        sampler.resample(false);
        count += sampler.det_buffer[0].popcnt();
        count += sampler.obs_buffer[0].popcnt();
    }).goal_millis(35);
    if (count == 0) {
        std::cerr << "Data dependence.";
    }
}
