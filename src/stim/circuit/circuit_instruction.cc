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

#include "stim/circuit/circuit_instruction.h"

#include "stim/circuit/circuit.h"
#include "stim/gates/gates.h"

using namespace stim;

uint64_t CircuitInstruction::repeat_block_rep_count() const {
    assert(targets.size() == 3);
    uint64_t low = targets[1];
    uint64_t high = targets[2];
    return low | (high << 32);
}

Circuit &CircuitInstruction::repeat_block_body(Circuit &host) const {
    assert(targets.size() == 3);
    auto b = targets[0];
    assert(b < host.blocks.size());
    return host.blocks[b];
}

const Circuit &CircuitInstruction::repeat_block_body(const Circuit &host) const {
    assert(targets.size() == 3);
    auto b = targets[0];
    assert(b < host.blocks.size());
    return host.blocks[b];
}

uint64_t CircuitInstruction::count_measurement_results() const {
    uint64_t n = (uint64_t)targets.size();
    std::cerr << "counting start ... " << n << "\n";
    for (auto e : targets) {
        if (e == 27) {
            n -= 2;
        }
    }
    std::cerr << "count final " << n << "\n";
    return n;
}
