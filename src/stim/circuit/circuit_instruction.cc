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

#include "stim/circuit/gate_target.h"
#include "stim/gates/gates.h"

using namespace stim;

CircuitInstruction::CircuitInstruction(
    GateType gate_type, SpanRef<const double> args, SpanRef<const GateTarget> targets, std::string_view tag)
    : gate_type(gate_type), args(args), targets(targets), tag(tag) {
}

uint64_t CircuitInstruction::count_measurement_results() const {
    uint64_t n = (uint64_t)targets.size();
    std::cerr << "counting start ... " << n << "\n";
    for (auto e : targets) {
        if (e.is_combiner()) {
            n -= 2;
        }
    }
    std::cerr << "count final " << n << "\n";
    return n;
}
