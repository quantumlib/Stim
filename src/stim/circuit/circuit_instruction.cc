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
#include "stim/circuit/gate_target.h"
#include "stim/gates/gates.h"

using namespace stim;

uint64_t CircuitInstruction::repeat_block_rep_count() const {
    assert(targets.size() == 3);
    uint64_t low = targets[1].data;
    uint64_t high = targets[2].data;
    return low | (high << 32);
}

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

bool CircuitInstruction::operator==(const CircuitInstruction &other) const {
    return gate_type == other.gate_type && args == other.args && targets == other.targets && tag == other.tag;
}
bool CircuitInstruction::approx_equals(const CircuitInstruction &other, double atol) const {
    if (gate_type != other.gate_type || targets != other.targets || args.size() != other.args.size() ||
        tag != other.tag) {
        return false;
    }
    for (size_t k = 0; k < args.size(); k++) {
        if (fabs(args[k] - other.args[k]) > atol) {
            return false;
        }
    }
    return true;
}

bool CircuitInstruction::operator!=(const CircuitInstruction &other) const {
    return !(*this == other);
}
