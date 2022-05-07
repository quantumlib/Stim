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

#include "stim/benchmark_util.perf.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/search/search.h"
#include "stim/simulators/error_analyzer.h"

using namespace stim;

BENCHMARK(find_graphlike_logical_error_surface_code_d25) {
    CircuitGenParameters params(25, 25, "rotated_memory_x");
    params.after_clifford_depolarization = 0.001;
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    params.before_round_data_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto model = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, 0.0, false, true);

    size_t total = 0;
    benchmark_go([&]() {
        total += stim::shortest_graphlike_undetectable_logical_error(model, false).instructions.size();
    }).goal_millis(35);
    if (total % 25 != 0 || total == 0) {
        std::cout << "bad";
    }
}
