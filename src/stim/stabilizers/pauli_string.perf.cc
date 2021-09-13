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

#include "stim/stabilizers/pauli_string.h"

#include "stim/benchmark_util.perf.h"

using namespace stim;

BENCHMARK(PauliString_multiplication_1M) {
    size_t n = 1000 * 1000;
    PauliString p1(n);
    PauliString p2(n);
    benchmark_go([&]() {
        p1.ref().inplace_right_mul_returning_log_i_scalar(p2);
    })
        .goal_micros(8.5)
        .show_rate("Paulis", n);
}

BENCHMARK(PauliString_multiplication_100K) {
    size_t n = 100 * 1000;
    PauliString p1(n);
    PauliString p2(n);
    benchmark_go([&]() {
        p1.ref().inplace_right_mul_returning_log_i_scalar(p2);
    })
        .goal_nanos(700)
        .show_rate("Paulis", n);
}

BENCHMARK(PauliString_multiplication_10K) {
    size_t n = 10 * 1000;
    PauliString p1(n);
    PauliString p2(n);
    benchmark_go([&]() {
        p1.ref().inplace_right_mul_returning_log_i_scalar(p2);
    })
        .goal_nanos(90)
        .show_rate("Paulis", n);
}
