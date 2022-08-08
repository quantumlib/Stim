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

#include "stim/mem/simd_bit_table.h"

#include "stim/benchmark_util.perf.h"

using namespace stim;

BENCHMARK(simd_bit_table_inplace_square_transpose_diam10K) {
    size_t n = 10 * 1000;
    simd_bit_table<MAX_BITWORD_WIDTH> table(n, n);
    benchmark_go([&]() {
        table.do_square_transpose();
    })
        .goal_millis(6)
        .show_rate("Bits", n * n);
}

BENCHMARK(simd_bit_table_out_of_place_transpose_diam10K) {
    size_t n = 10 * 1000;
    simd_bit_table<MAX_BITWORD_WIDTH> table(n, n);
    simd_bit_table<MAX_BITWORD_WIDTH> out(n, n);
    benchmark_go([&]() {
        table.transpose_into(out);
    })
        .goal_millis(12)
        .show_rate("Bits", n * n);
}
