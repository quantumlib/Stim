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

#include "tableau_simulator.h"

#include "../benchmark_util.h"

BENCHMARK(TableauSimulator_CX_10Kqubits) {
    size_t num_qubits = 10 * 1000;
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    TableauSimulator sim(num_qubits, rng);

    std::vector<uint32_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    OperationData op_data{0, targets};

    benchmark_go([&]() {
        sim.ZCX(op_data);
    })
        .goal_millis(5)
        .show_rate("OpQubits", targets.size());
}
