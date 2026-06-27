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

#include <algorithm>
#include <string>
#include <utility>

#include "stim/circuit/gate_target.h"
#include "stim/gates/gates.h"

using namespace stim;

Circuit::Circuit() : target_buf(), arg_buf(), tag_buf(), operations(), blocks() {
}

Circuit::Circuit(const Circuit &circuit)
    : target_buf(circuit.target_buf.total_allocated()),
      arg_buf(circuit.arg_buf.total_allocated()),
      tag_buf(circuit.tag_buf.total_allocated()),
      operations(circuit.operations),
      blocks(circuit.blocks) {
    // Keep local copy of operation data.
    for (auto &op : operations) {
        op.targets = target_buf.take_copy(op.targets);
        op.args = arg_buf.take_copy(op.args);
        op.tag = tag_buf.take_copy(op.tag);
    }
}

Circuit::Circuit(Circuit &&circuit) noexcept
    : target_buf(std::move(circuit.target_buf)),
      arg_buf(std::move(circuit.arg_buf)),
      tag_buf(std::move(circuit.tag_buf)),
      operations(std::move(circuit.operations)),
      blocks(std::move(circuit.blocks)) {
}

Circuit &Circuit::operator=(const Circuit &circuit) {
    if (&circuit != this) {
        blocks = circuit.blocks;
        operations = circuit.operations;

        // Keep local copy of operation data.
        target_buf = MonotonicBuffer<GateTarget>(circuit.target_buf.total_allocated());
        arg_buf = MonotonicBuffer<double>(circuit.arg_buf.total_allocated());
        tag_buf = MonotonicBuffer<char>(circuit.tag_buf.total_allocated());
        for (auto &op : operations) {
            op.targets = target_buf.take_copy(op.targets);
            op.args = arg_buf.take_copy(op.args);
            op.tag = tag_buf.take_copy(op.tag);
        }
    }
    return *this;
}

Circuit &Circuit::operator=(Circuit &&circuit) noexcept {
    if (&circuit != this) {
        operations = std::move(circuit.operations);
        blocks = std::move(circuit.blocks);
        target_buf = std::move(circuit.target_buf);
        arg_buf = std::move(circuit.arg_buf);
        tag_buf = std::move(circuit.tag_buf);
    }
    return *this;
}

void Circuit::clear() {
    target_buf.clear();
    arg_buf.clear();
    operations.clear();
    blocks.clear();
}

uint64_t Circuit::count_detectors() const {
    return flat_count_operations([=](const CircuitInstruction &op) -> uint64_t {
        return op.gate_type == GateType::DETECTOR;
    });
}

uint64_t Circuit::count_ticks() const {
    return flat_count_operations([=](const CircuitInstruction &op) -> uint64_t {
        return op.gate_type == GateType::TICK;
    });
}

uint64_t Circuit::count_observables() const {
    return max_operation_property([=](const CircuitInstruction &op) -> uint64_t {
        return op.gate_type == GateType::OBSERVABLE_INCLUDE ? (size_t)op.args[0] + 1 : 0;
    });
}

size_t Circuit::count_sweep_bits() const {
    return 0;
}

void stim::vec_pad_add_mul(std::vector<double> &target, SpanRef<const double> offset, uint64_t mul) {
    while (target.size() < offset.size()) {
        target.push_back(0);
    }
    for (size_t k = 0; k < offset.size(); k++) {
        target[k] += offset[k] * mul;
    }
}
