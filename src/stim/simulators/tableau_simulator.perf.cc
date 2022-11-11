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

#include "stim/simulators/tableau_simulator.h"

#include "stim/benchmark_util.perf.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_surface_code.h"

using namespace stim;

BENCHMARK(TableauSimulator_CX_10Kqubits) {
    size_t num_qubits = 10 * 1000;
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    TableauSimulator sim(rng, num_qubits);

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < (uint32_t)num_qubits; k++) {
        targets.push_back(GateTarget{k});
    }
    OperationData op_data{{}, targets};

    benchmark_go([&]() {
        sim.ZCX(op_data);
    })
        .goal_millis(5)
        .show_rate("OpQubits", targets.size());
}

BENCHMARK(TableauSimulator_reference_sample_surface_code_d31_r1000) {
    CircuitGenParameters params(1000, 31, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    simd_bits<MAX_BITWORD_WIDTH> ref(0);
    auto total = 0;
    benchmark_go([&]() {
        auto result = TableauSimulator::reference_sample_circuit(circuit);
        total += result.not_zero();
    })
        .goal_millis(25)
        .show_rate("Samples", circuit.count_measurements());
    if (total) {
        std::cerr << "data dependence";
    }
}
