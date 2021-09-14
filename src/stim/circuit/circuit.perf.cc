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

#include "stim/circuit/circuit.h"

#include "stim/benchmark_util.perf.h"

using namespace stim;

BENCHMARK(circuit_parse) {
    Circuit c;
    benchmark_go([&]() {
        c = Circuit(R"input(
H 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
CNOT 4 5 6 7
M 1 2 3 4 5 6 7 8 9 10 11
            )input");
    }).goal_nanos(950);
    if (c.count_qubits() == 0) {
        std::cerr << "impossible";
    }
}

BENCHMARK(circuit_parse_sparse) {
    Circuit c;
    for (auto k = 0; k < 1000; k++) {
        c.append_op("H", {0});
        c.append_op("CNOT", {1, 2});
        c.append_op("M", {0});
    }
    auto text = c.str();
    benchmark_go([&]() {
        c = Circuit(text.data());
    }).goal_micros(150);
    if (c.count_qubits() == 0) {
        std::cerr << "impossible";
    }
}
