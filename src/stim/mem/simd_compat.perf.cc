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

#include "stim/mem/simd_compat.h"

#include "stim/benchmark_util.perf.h"
#include "stim/mem/simd_bits.h"
#include "stim/mem/simd_compat.h"

using namespace stim;

BENCHMARK(simd_compat_popcnt) {
    simd_bits d(1024 * 256);
    std::mt19937_64 rng(0);
    d.randomize(d.num_bits_padded(), rng);

    uint64_t optimization_blocker = 0;
    benchmark_go([&]() {
        d[300] ^= true;
        for (size_t k = 0; k < d.num_simd_words; k++) {
            optimization_blocker += d.ptr_simd[k].popcount();
        }
    })
        .goal_micros(1.5)
        .show_rate("Bits", d.num_bits_padded());
    if (optimization_blocker == 0) {
        std::cout << '!';
    }
}
