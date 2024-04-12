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

#include "stim/simulators/error_analyzer.h"

#include "stim/gen/gen_surface_code.h"
#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(ErrorAnalyzer_surface_code_rotated_memory_z_d11_r100) {
    auto params = CircuitGenParameters(100, 11, "rotated_memory_z");
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    benchmark_go([&]() {
        ErrorAnalyzer analyzer(
            circuit.count_measurements(),
            circuit.count_detectors(),
            circuit.count_qubits(),
            circuit.count_ticks(),
            false,
            false,
            false,
            0.0,
            false,
            true);
        analyzer.undo_circuit(circuit);
    }).goal_millis(320);
}

BENCHMARK(ErrorAnalyzer_surface_code_rotated_memory_z_d11_r100_find_reducible_errors) {
    auto params = CircuitGenParameters(100, 11, "rotated_memory_z");
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    benchmark_go([&]() {
        ErrorAnalyzer analyzer(
            circuit.count_measurements(),
            circuit.count_detectors(),
            circuit.count_qubits(),
            circuit.count_ticks(),
            true,
            false,
            false,
            0.0,
            false,
            true);
        analyzer.undo_circuit(circuit);
    }).goal_millis(450);
}

BENCHMARK(ErrorAnalyzer_surface_code_rotated_memory_z_d11_r100000000_find_loops) {
    auto params = CircuitGenParameters(100000000, 11, "rotated_memory_z");
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    benchmark_go([&]() {
        ErrorAnalyzer analyzer(
            circuit.count_measurements(),
            circuit.count_detectors(),
            circuit.count_qubits(),
            circuit.count_ticks(),
            false,
            true,
            false,
            0.0,
            false,
            true);
        analyzer.undo_circuit(circuit);
    }).goal_millis(15);
}
