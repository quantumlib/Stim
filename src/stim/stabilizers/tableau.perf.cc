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

#include "stim/stabilizers/tableau.h"

#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(tableau_random_10) {
    size_t n = 10;
    Tableau<MAX_BITWORD_WIDTH> t(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() {
        t = Tableau<MAX_BITWORD_WIDTH>::random(n, rng);
    }).goal_micros(30);
}

BENCHMARK(tableau_random_100) {
    size_t n = 100;
    Tableau<MAX_BITWORD_WIDTH> t(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() {
        t = Tableau<MAX_BITWORD_WIDTH>::random(n, rng);
    }).goal_millis(1.1);
}

BENCHMARK(tableau_random_256) {
    size_t n = 256;
    Tableau<MAX_BITWORD_WIDTH> t(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() {
        t = Tableau<MAX_BITWORD_WIDTH>::random(n, rng);
    }).goal_millis(7.5);
}

BENCHMARK(tableau_random_1000) {
    size_t n = 1000;
    Tableau<MAX_BITWORD_WIDTH> t(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() {
        t = Tableau<MAX_BITWORD_WIDTH>::random(n, rng);
    }).goal_millis(130);
}

BENCHMARK(tableau_cnot_10Kqubits) {
    size_t n = 10 * 1000;
    Tableau<MAX_BITWORD_WIDTH> t(n);
    benchmark_go([&]() {
        t.prepend_ZCX(5, 20);
    }).goal_nanos(170);
}
