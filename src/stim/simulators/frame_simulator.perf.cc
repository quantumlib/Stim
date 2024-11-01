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

#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(FrameSimulator_depolarize1_100Kqubits_1Ksamples_per1000) {
    CircuitStats stats;
    stats.num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    double probability = 0.001;
    FrameSimulator<MAX_BITWORD_WIDTH> sim(
        stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, num_samples, std::mt19937_64(0));

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < stats.num_qubits; k++) {
        targets.push_back(GateTarget{k});
    }
    CircuitInstruction op_data{GateType::DEPOLARIZE1, &probability, targets, ""};
    benchmark_go([&]() {
        sim.do_DEPOLARIZE1(op_data);
    })
        .goal_millis(5)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_depolarize2_100Kqubits_1Ksamples_per1000) {
    CircuitStats stats;
    stats.num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    double probability = 0.001;
    FrameSimulator<MAX_BITWORD_WIDTH> sim(
        stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, num_samples, std::mt19937_64(0));

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < stats.num_qubits; k++) {
        targets.push_back({k});
    }
    CircuitInstruction op_data{GateType::DEPOLARIZE2, &probability, targets, ""};

    benchmark_go([&]() {
        sim.do_DEPOLARIZE2(op_data);
    })
        .goal_millis(5)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_hadamard_100Kqubits_1Ksamples) {
    CircuitStats stats;
    stats.num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    FrameSimulator<MAX_BITWORD_WIDTH> sim(
        stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, num_samples, std::mt19937_64(0));

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < stats.num_qubits; k++) {
        targets.push_back({k});
    }
    CircuitInstruction op_data{GateType::H, {}, targets, ""};

    benchmark_go([&]() {
        sim.do_H_XZ(op_data);
    })
        .goal_millis(2)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_CX_100Kqubits_1Ksamples) {
    CircuitStats stats;
    stats.num_qubits = 100 * 1000;
    size_t num_samples = 1000;
    FrameSimulator<MAX_BITWORD_WIDTH> sim(
        stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, num_samples, std::mt19937_64(0));

    std::vector<GateTarget> targets;
    for (uint32_t k = 0; k < stats.num_qubits; k++) {
        targets.push_back({k});
    }
    CircuitInstruction op_data{GateType::CX, {}, targets, ""};

    benchmark_go([&]() {
        sim.do_ZCX(op_data);
    })
        .goal_millis(2)
        .show_rate("OpQubits", targets.size() * num_samples);
}

BENCHMARK(FrameSimulator_surface_code_rotated_memory_z_d11_r100_batch1024) {
    auto params = CircuitGenParameters(100, 11, "rotated_memory_z");
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;

    FrameSimulator<MAX_BITWORD_WIDTH> sim(
        circuit.compute_stats(), FrameSimulatorMode::STORE_MEASUREMENTS_TO_MEMORY, 1024, std::mt19937_64(0));

    benchmark_go([&]() {
        sim.reset_all();
        sim.do_circuit(circuit);
    })
        .goal_millis(5.1)
        .show_rate("Shots", 1024)
        .show_rate("Dets", circuit.count_detectors() * 1024);
    sim.reset_all();
    if (!sim.obs_record[0].not_zero()) {
        std::cerr << "data dependence";
    }
}
