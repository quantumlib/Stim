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

#include "stim/mem/simd_bits.h"

#include <cstring>

#include "stim/benchmark_util.perf.h"

using namespace stim;

BENCHMARK(simd_bits_randomize_10K) {
    size_t n = 10 * 1000;
    simd_bits data(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() {
        data.randomize(n, rng);
    })
        .goal_nanos(450)
        .show_rate("Bits", n);
}

BENCHMARK(simd_bits_xor_10K) {
    size_t n = 10 * 1000;
    simd_bits d1(n);
    simd_bits d2(n);
    benchmark_go([&]() {
        d2 ^= d1;
    })
        .goal_nanos(20)
        .show_rate("Bits", n);
}

BENCHMARK(simd_bits_not_zero_100K) {
    size_t n = 10 * 100;
    simd_bits d(n);
    benchmark_go([&]() {
        d.not_zero();
    })
        .goal_nanos(4)
        .show_rate("Bits", n);
}
