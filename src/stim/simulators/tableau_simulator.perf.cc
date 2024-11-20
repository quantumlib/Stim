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

#include "stim/gen/circuit_gen_params.h"
#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(TableauSimulator_CX_10Kqubits) {
    size_t num_qubits = 10 * 1000;
    TableauSimulator<MAX_BITWORD_WIDTH> sim(std::mt19937_64(0), num_qubits);

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < (uint32_t)num_qubits; k++) {
        targets.push_back(GateTarget{k});
    }
    CircuitInstruction op_data{GateType::CX, {}, targets, ""};

    benchmark_go([&]() {
        sim.do_ZCX(op_data);
    })
        .goal_millis(5)
        .show_rate("OpQubits", targets.size());
}
