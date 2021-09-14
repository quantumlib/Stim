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

#include "stim/simulators/frame_simulator.h"

#include "stim/benchmark_util.perf.h"

using namespace stim;

BENCHMARK(FrameSimulator_depolarize1_100Kqubits_1Ksamples_per1000) {
    size_t num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    double probability = 0.001;
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    FrameSimulator sim(num_qubits, num_samples, SIZE_MAX, rng);

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < (uint32_t)num_qubits; k++) {
        targets.push_back(GateTarget{k});
    }
    OperationData op_data{{&probability}, targets};
    benchmark_go([&]() {
        sim.DEPOLARIZE1(op_data);
    })
        .goal_millis(5)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_depolarize2_100Kqubits_1Ksamples_per1000) {
    size_t num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    double probability = 0.001;
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    FrameSimulator sim(num_qubits, num_samples, SIZE_MAX, rng);

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < (uint32_t)num_qubits; k++) {
        targets.push_back({k});
    }
    OperationData op_data{{&probability}, targets};

    benchmark_go([&]() {
        sim.DEPOLARIZE2(op_data);
    })
        .goal_millis(5)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_hadamard_100Kqubits_1Ksamples) {
    size_t num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    FrameSimulator sim(num_qubits, num_samples, SIZE_MAX, rng);

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < (uint32_t)num_qubits; k++) {
        targets.push_back({k});
    }
    OperationData op_data{{}, targets};

    benchmark_go([&]() {
        sim.H_XZ(op_data);
    })
        .goal_millis(2)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_CX_100Kqubits_1Ksamples) {
    size_t num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    FrameSimulator sim(num_qubits, num_samples, SIZE_MAX, rng);

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < (uint32_t)num_qubits; k++) {
        targets.push_back({k});
    }
    OperationData op_data{{}, targets};

    benchmark_go([&]() {
        sim.ZCX(op_data);
    })
        .goal_millis(2)
        .show_rate("OpQubits", targets.size() * num_samples);
}
