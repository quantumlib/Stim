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

#include "stim/stabilizers/tableau_iter.h"

#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(tableau_iter_unsigned_3q) {
    size_t c = 0;
    benchmark_go([&]() {
        TableauIterator<MAX_BITWORD_WIDTH> iter(3, false);
        while (iter.iter_next()) {
            c += iter.result.num_qubits;
        }
    })
        .goal_millis(200)
        .show_rate("Tableaus", 1451520);
    if (c == 0) {
        std::cerr << "use the output\n";
    }
}

BENCHMARK(tableau_iter_all_3q) {
    size_t c = 0;
    benchmark_go([&]() {
        TableauIterator<MAX_BITWORD_WIDTH> iter(3, true);
        while (iter.iter_next()) {
            c += iter.result.num_qubits;
        }
    })
        .goal_millis(420)
        .show_rate("Tableaus", 92897280);
    if (c == 0) {
        std::cerr << "use the output\n";
    }
}
