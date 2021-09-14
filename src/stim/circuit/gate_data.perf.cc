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

#include "stim/circuit/gate_data.h"

#include <iostream>

#include "stim/benchmark_util.perf.h"

using namespace stim;

BENCHMARK(gate_data_hash_all_gate_names) {
    std::vector<std::string> names;
    for (const auto &gate : GATE_DATA.gates()) {
        names.emplace_back(gate.name);
    }
    size_t result = 0;
    benchmark_go([&]() {
        for (const auto &s : names) {
            result += gate_name_to_id(s.data(), s.size());
        }
    })
        .goal_nanos(2.7 * names.size())
        .show_rate("GateHashes", names.size());
    if (result == 0) {
        std::cerr << "impossible";
    }
}
