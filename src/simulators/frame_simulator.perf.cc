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

#include "frame_simulator.h"

#include "../benchmark_util.h"
#include "../circuit/common_circuits.h"

BENCHMARK(FrameSimulator_depolarize1_100Kqubits_1Ksamples_per1000) {
    size_t num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    float probability = 0.001f;
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    FrameSimulator sim(num_qubits, num_samples, 1, rng);

    std::vector<uint32_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    OperationData op_data{probability, {&targets, 0, targets.size()}};
    op_data.arg = probability;
    benchmark_go([&]() {
        sim.DEPOLARIZE1(op_data);
    })
        .goal_millis(5)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_depolarize2_100Kqubits_1Ksamples_per1000) {
    size_t num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    float probability = 0.001f;
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    FrameSimulator sim(num_qubits, num_samples, 1, rng);

    std::vector<uint32_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    OperationData op_data{probability, {&targets, 0, targets.size()}};
    op_data.arg = probability;

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
    FrameSimulator sim(num_qubits, num_samples, 1, rng);

    std::vector<uint32_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    OperationData op_data{0, {&targets, 0, targets.size()}};

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
    FrameSimulator sim(num_qubits, num_samples, 1, rng);

    std::vector<uint32_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    OperationData op_data{0, {&targets, 0, targets.size()}};

    benchmark_go([&]() {
        sim.ZCX(op_data);
    })
        .goal_millis(2)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_sample_unrotated_surface_code_d5_samples1024) {
    size_t num_samples = 1024;
    size_t distance = 5;
    auto circuit = Circuit::from_text(unrotated_surface_code_program_text(distance, distance, 0));
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits ref(circuit.num_measurements);
    benchmark_go([&]() {
        FrameSimulator::sample(circuit, ref, num_samples, rng);
    })
        .goal_micros(70)
        .show_rate("Layers", distance * num_samples)
        .show_rate("OutBits", circuit.num_measurements * num_samples);
}

BENCHMARK(FrameSimulator_sample_unrotated_noisy_surface_code_d5_samples1024) {
    size_t num_samples = 1024;
    size_t distance = 5;
    auto circuit = Circuit::from_text(unrotated_surface_code_program_text(distance, distance, 0.001));
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits ref(circuit.num_measurements);
    benchmark_go([&]() {
        FrameSimulator::sample(circuit, ref, num_samples, rng);
    })
        .goal_micros(170)
        .show_rate("Layers", distance * num_samples)
        .show_rate("OutBits", circuit.num_measurements * num_samples);
}

BENCHMARK(FrameSimulator_sample_unrotated_surface_code_d41_samples1024) {
    size_t num_samples = 1024;
    size_t distance = 41;
    auto circuit = Circuit::from_text(unrotated_surface_code_program_text(distance, distance, 0));
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits ref(circuit.num_measurements);
    benchmark_go([&]() {
        FrameSimulator::sample(circuit, ref, num_samples, rng);
    })
        .goal_millis(60)
        .show_rate("Layers", distance * num_samples)
        .show_rate("OutBits", circuit.num_measurements * num_samples);
}

BENCHMARK(FrameSimulator_sample_unrotated_noisy_surface_code_d41_samples1024) {
    size_t num_samples = 1024;
    size_t distance = 41;
    auto circuit = Circuit::from_text(unrotated_surface_code_program_text(distance, distance, 0.001));
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits ref(circuit.num_measurements);
    benchmark_go([&]() {
        FrameSimulator::sample(circuit, ref, num_samples, rng);
    })
        .goal_millis(130)
        .show_rate("Layers", distance * num_samples)
        .show_rate("OutBits", circuit.num_measurements * num_samples);
}
