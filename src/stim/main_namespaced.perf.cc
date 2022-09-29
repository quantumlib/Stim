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

#include "stim/main_namespaced.h"

#include "stim/benchmark_util.perf.h"
#include "stim/simulators/detection_simulator.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

Circuit make_rep_code(uint32_t distance, uint32_t rounds) {
    Circuit round_ops;
    for (uint32_t k = 0; k < distance - 1; k++) {
        round_ops.append_op("CNOT", {2 * k, 2 * k + 1});
    }
    for (uint32_t k = 0; k < distance - 1; k++) {
        round_ops.append_op("DEPOLARIZE2", {2 * k, 2 * k + 1}, 0.001);
    }
    for (uint32_t k = 1; k < distance; k++) {
        round_ops.append_op("CNOT", {2 * k, 2 * k - 1});
    }
    for (uint32_t k = 1; k < distance; k++) {
        round_ops.append_op("DEPOLARIZE2", {2 * k, 2 * k - 1}, 0.001);
    }
    for (uint32_t k = 0; k < distance - 1; k++) {
        round_ops.append_op("X_ERROR", {2 * k + 1}, 0.001);
    }
    for (uint32_t k = 0; k < distance - 1; k++) {
        round_ops.append_op("MR", {2 * k + 1});
    }
    Circuit detectors;
    for (uint32_t k = 1; k < distance; k++) {
        detectors.append_op("DETECTOR", {k | TARGET_RECORD_BIT, (k + distance - 1) | TARGET_RECORD_BIT});
    }

    Circuit result = round_ops + (round_ops + detectors) * (rounds - 1);
    for (uint32_t k = 0; k < distance; k++) {
        result.append_op("X_ERROR", {2 * k}, 0.001);
    }
    for (uint32_t k = 0; k < distance; k++) {
        result.append_op("M", {2 * k});
    }
    for (uint32_t k = 1; k < distance; k++) {
        result.append_op(
            "DETECTOR", {k | TARGET_RECORD_BIT, (k + 1) | TARGET_RECORD_BIT, (k + distance) | TARGET_RECORD_BIT});
    }
    result.append_op("OBSERVABLE_INCLUDE", {1 | TARGET_RECORD_BIT}, 0);
    return result;
}

BENCHMARK(main_sample1_tableau_rep_d1000_r100) {
    size_t distance = 1000;
    size_t rounds = 100;
    auto circuit = make_rep_code(distance, rounds);
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    fprintf(in, "%s", circuit.str().data());
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    benchmark_go([&]() {
        rewind(in);
        rewind(out);
        TableauSimulator::sample_stream(in, out, SAMPLE_FORMAT_B8, false, rng);
    })
        .goal_millis(30)
        .show_rate("Samples", circuit.count_measurements());
}

BENCHMARK(main_sample1_pauliframe_b8_rep_d1000_r100) {
    size_t distance = 1000;
    size_t rounds = 100;
    auto circuit = make_rep_code(distance, rounds);
    FILE *out = tmpfile();
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits<MAX_BITWORD_WIDTH> ref(0);
    benchmark_go([&]() {
        rewind(out);
        FrameSimulator::sample_out(circuit, ref, 1, out, SAMPLE_FORMAT_B8, rng);
    })
        .goal_millis(16)
        .show_rate("Samples", circuit.count_measurements());
}

BENCHMARK(main_sample1_detectors_b8_rep_d1000_r100) {
    size_t distance = 1000;
    size_t rounds = 100;
    auto circuit = make_rep_code(distance, rounds);
    FILE *out = tmpfile();
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits<MAX_BITWORD_WIDTH> ref(circuit.count_measurements());
    benchmark_go([&]() {
        rewind(out);
        detector_samples_out(circuit, 1, false, true, out, SAMPLE_FORMAT_B8, rng, nullptr, SAMPLE_FORMAT_01);
    })
        .goal_millis(20)
        .show_rate("Samples", circuit.count_measurements());
}

BENCHMARK(main_sample256_pauliframe_b8_rep_d1000_r100) {
    size_t distance = 1000;
    size_t rounds = 100;
    auto circuit = make_rep_code(distance, rounds);
    FILE *out = tmpfile();
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits<MAX_BITWORD_WIDTH> ref(0);
    benchmark_go([&]() {
        rewind(out);
        FrameSimulator::sample_out(circuit, ref, 256, out, SAMPLE_FORMAT_B8, rng);
    })
        .goal_millis(20)
        .show_rate("Samples", circuit.count_measurements());
}

BENCHMARK(main_sample256_pauliframe_b8_rep_d1000_r1000_stream) {
    size_t distance = 1000;
    size_t rounds = 1000;
    auto circuit = make_rep_code(distance, rounds);
    FILE *out = tmpfile();
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits<MAX_BITWORD_WIDTH> ref(0);
    benchmark_go([&]() {
        rewind(out);
        FrameSimulator::sample_out(circuit, ref, 256, out, SAMPLE_FORMAT_B8, rng);
    })
        .goal_millis(360)
        .show_rate("Samples", circuit.count_measurements());
}

BENCHMARK(main_sample256_detectors_b8_rep_d1000_r100) {
    size_t distance = 1000;
    size_t rounds = 100;
    auto circuit = make_rep_code(distance, rounds);
    FILE *out = tmpfile();
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits<MAX_BITWORD_WIDTH> ref(0);
    benchmark_go([&]() {
        rewind(out);
        detector_samples_out(circuit, 256, false, true, out, SAMPLE_FORMAT_B8, rng, nullptr, SAMPLE_FORMAT_01);
    })
        .goal_millis(25)
        .show_rate("Samples", circuit.count_measurements());
}

BENCHMARK(main_sample256_detectors_b8_rep_d1000_r1000_stream) {
    size_t distance = 1000;
    size_t rounds = 1000;
    auto circuit = make_rep_code(distance, rounds);
    FILE *out = tmpfile();
    std::mt19937_64 rng(0);  // NOLINT(cert-msc51-cpp)
    simd_bits<MAX_BITWORD_WIDTH> ref(0);
    benchmark_go([&]() {
        rewind(out);
        detector_samples_out(circuit, 256, false, true, out, SAMPLE_FORMAT_B8, rng, nullptr, SAMPLE_FORMAT_01);
    })
        .goal_millis(360)
        .show_rate("Samples", circuit.count_measurements());
}
