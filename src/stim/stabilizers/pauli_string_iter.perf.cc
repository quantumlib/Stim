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

#include "stim/stabilizers/pauli_string_iter.h"

#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(pauli_iter_xz_2_to_5_of_5) {
    size_t c = 0;
    size_t n = 0;
    benchmark_go([&]() {
        PauliStringIterator<MAX_BITWORD_WIDTH> iter(5, 2, 5, true, false, true);
        n = 0;
        while (iter.iter_next()) {
            c += iter.result.num_qubits;
            n += 1;
        }
    })
        .goal_micros(8)
        .show_rate("PauliStrings", n);
    if (c == 0) {
        std::cerr << "use the output\n";
    }
}

BENCHMARK(pauli_iter_xyz_1_of_1000) {
    size_t c = 0;
    size_t n = 0;
    benchmark_go([&]() {
        PauliStringIterator<MAX_BITWORD_WIDTH> iter(1000, 1, 1, true, true, true);
        n = 0;
        while (iter.iter_next()) {
            c += iter.result.num_qubits;
            n += 1;
        }
    })
        .goal_micros(55)
        .show_rate("PauliStrings", n);
    if (c == 0) {
        std::cerr << "use the output\n";
    }
}
