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

#include "stim/util_bot/probability_util.h"

#include "stim/mem/simd_bits.h"
#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(biased_random_1024_0point1percent) {
    std::mt19937_64 rng(0);
    float p = 0.001;
    size_t n = 1024;
    simd_bits<MAX_BITWORD_WIDTH> data(n);
    benchmark_go([&]() {
        biased_randomize_bits(p, data.u64, data.u64 + data.num_u64_padded(), rng);
    })
        .goal_nanos(70)
        .show_rate("bits", n);
}

BENCHMARK(biased_random_1024_0point01percent) {
    std::mt19937_64 rng(0);
    float p = 0.0001;
    size_t n = 1024;
    simd_bits<MAX_BITWORD_WIDTH> data(n);
    benchmark_go([&]() {
        biased_randomize_bits(p, data.u64, data.u64 + data.num_u64_padded(), rng);
    })
        .goal_nanos(35)
        .show_rate("bits", n);
}

BENCHMARK(biased_random_1024_1percent) {
    std::mt19937_64 rng(0);
    float p = 0.01;
    size_t n = 1024;
    simd_bits<MAX_BITWORD_WIDTH> data(n);
    benchmark_go([&]() {
        biased_randomize_bits(p, data.u64, data.u64 + data.num_u64_padded(), rng);
    })
        .goal_nanos(250)
        .show_rate("bits", n);
}

BENCHMARK(biased_random_1024_40percent) {
    std::mt19937_64 rng(0);
    float p = 0.4;
    size_t n = 1024;
    simd_bits<MAX_BITWORD_WIDTH> data(n);
    benchmark_go([&]() {
        biased_randomize_bits(p, data.u64, data.u64 + data.num_u64_padded(), rng);
    })
        .goal_nanos(420)
        .show_rate("bits", n);
}

BENCHMARK(biased_random_1024_50percent) {
    std::mt19937_64 rng(0);
    float p = 0.5;
    size_t n = 1024;
    simd_bits<MAX_BITWORD_WIDTH> data(n);
    benchmark_go([&]() {
        biased_randomize_bits(p, data.u64, data.u64 + data.num_u64_padded(), rng);
    })
        .goal_nanos(40)
        .show_rate("bits", n);
}

BENCHMARK(biased_random_1024_90percent) {
    std::mt19937_64 rng(0);
    float p = 0.9;
    size_t n = 1024;
    simd_bits<MAX_BITWORD_WIDTH> data(n);
    benchmark_go([&]() {
        biased_randomize_bits(p, data.u64, data.u64 + data.num_u64_padded(), rng);
    })
        .goal_nanos(450)
        .show_rate("bits", n);
}

BENCHMARK(biased_random_1024_99percent) {
    std::mt19937_64 rng(0);
    float p = 0.99;
    size_t n = 1024;
    simd_bits<MAX_BITWORD_WIDTH> data(n);
    benchmark_go([&]() {
        biased_randomize_bits(p, data.u64, data.u64 + data.num_u64_padded(), rng);
    })
        .goal_nanos(260)
        .show_rate("bits", n);
}
