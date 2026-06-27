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

void Circuit::safe_append(CircuitInstruction operation, bool block_fusion) {
    auto flags = GATE_DATA[operation.gate_type].flags;
    if (flags & GATE_IS_BLOCK) {
        throw std::invalid_argument("Can't append a block like a normal operation.");
    }

    operation.validate();

    // Ensure arg/target data is backed by coping it into this circuit's buffers.
    operation.args = arg_buf.take_copy(operation.args);
    operation.targets = target_buf.take_copy(operation.targets);
    operation.tag = tag_buf.take_copy(operation.tag);
}

void Circuit::safe_append_ua(
    std::string_view gate_name, const std::vector<uint32_t> &targets, double singleton_arg, std::string_view tag) {
    const auto &gate = GATE_DATA.at(gate_name);

    std::vector<GateTarget> converted;
    converted.reserve(targets.size());
    for (auto e : targets) {
        converted.push_back({e});
    }

    safe_append(CircuitInstruction(gate.id, &singleton_arg, converted, tag));
}

void Circuit::safe_append_u(
    std::string_view gate_name,
    const std::vector<uint32_t> &targets,
    const std::vector<double> &args,
    std::string_view tag) {
    const auto &gate = GATE_DATA.at(gate_name);

    std::vector<GateTarget> converted;
    converted.reserve(targets.size());
    for (auto e : targets) {
        converted.push_back({e});
    }

    safe_append(CircuitInstruction(gate.id, args, converted, tag));
}

void Circuit::safe_insert(size_t index, const CircuitInstruction &instruction) {
    if (index > operations.size()) {
        throw std::invalid_argument("index > operations.size()");
    }
    auto flags = GATE_DATA[instruction.gate_type].flags;
    if (flags & GATE_IS_BLOCK) {
        throw std::invalid_argument("Can't insert a block like a normal operation.");
    }
    instruction.validate();

    // Copy arg/target data into this circuit's buffers.
    CircuitInstruction copy = instruction;
    copy.args = arg_buf.take_copy(copy.args);
    copy.targets = target_buf.take_copy(copy.targets);
    copy.tag = tag_buf.take_copy(copy.tag);
    operations.insert(operations.begin() + index, copy);

}

void Circuit::safe_insert(size_t index, const Circuit &circuit) {
    if (index > operations.size()) {
        throw std::invalid_argument("index > operations.size()");
    }

    operations.insert(operations.begin() + index, circuit.operations.begin(), circuit.operations.end());

    // Copy backing data over into this circuit.
    for (size_t k = index; k < index + circuit.operations.size(); k++) {
        if (operations[k].gate_type == GateType::REPEAT) {
            blocks.push_back(operations[k].repeat_block_body(circuit));
            auto repeat_count = operations[k].repeat_block_rep_count();
            target_buf.append_tail(GateTarget{(uint32_t)(blocks.size() - 1)});
            target_buf.append_tail(GateTarget{(uint32_t)(repeat_count & 0xFFFFFFFFULL)});
            target_buf.append_tail(GateTarget{(uint32_t)(repeat_count >> 32)});
            operations[k].targets = target_buf.commit_tail();
        } else {
            operations[k].targets = target_buf.take_copy(operations[k].targets);
            operations[k].args = arg_buf.take_copy(operations[k].args);
            operations[k].tag = tag_buf.take_copy(operations[k].tag);
        }
    }

}

void Circuit::safe_insert_repeat_block(
    size_t index, uint64_t repeat_count, const Circuit &block, std::string_view tag) {
    if (repeat_count == 0) {
        throw std::invalid_argument("Can't repeat 0 times.");
    }
    if (index > operations.size()) {
        throw std::invalid_argument("index > operations.size()");
    }
    target_buf.append_tail(GateTarget{(uint32_t)blocks.size()});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count & 0xFFFFFFFFULL)});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count >> 32)});
    blocks.push_back(block);
    auto targets = target_buf.commit_tail();
    operations.insert(operations.begin() + index, CircuitInstruction(GateType::REPEAT, {}, targets, tag));
}

void Circuit::safe_append_reversed_targets(CircuitInstruction instruction, bool reverse_in_pairs) {
    if (reverse_in_pairs) {
        if (instruction.targets.size() % 2 != 0) {
            throw std::invalid_argument("targets.size() % 2 != 0");
        }
        for (size_t k = instruction.targets.size(); k;) {
            k -= 2;
            target_buf.append_tail(instruction.targets[k]);
            target_buf.append_tail(instruction.targets[k + 1]);
        }
    } else {
        for (size_t k = instruction.targets.size(); k-- > 0;) {
            target_buf.append_tail(instruction.targets[k]);
        }
    }

    CircuitInstruction to_add = instruction;
    try {
        to_add.validate();
    } catch (const std::invalid_argument &ex) {
        target_buf.discard_tail();
        throw;
    }

    // Commit reversed tail data.
    to_add.targets = target_buf.commit_tail();

    // Ensure arg/tag data is backed by copying it into this circuit's buffers.
    to_add.args = arg_buf.take_copy(to_add.args);
    to_add.tag = tag_buf.take_copy(to_add.tag);
}

void Circuit::clear() {
    target_buf.clear();
    arg_buf.clear();
    operations.clear();
    blocks.clear();
}

size_t Circuit::count_qubits() const {
    return 0;
}

size_t Circuit::max_lookback() const {
    return 0;
}

uint64_t stim::add_saturate(uint64_t a, uint64_t b) {
    uint64_t r = a + b;
    if (r < a) {
        return UINT64_MAX;
    }
    return r;
}

uint64_t stim::mul_saturate(uint64_t a, uint64_t b) {
    if (b && a > UINT64_MAX / b) {
        return UINT64_MAX;
    }
    return a * b;
}

uint64_t Circuit::count_measurements() const {
    return flat_count_operations([=](const CircuitInstruction &op) -> uint64_t {
        return op.count_measurement_results();
    });
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

void Circuit::append_repeat_block(uint64_t repeat_count, Circuit &&body, std::string_view tag) {
    if (repeat_count == 0) {
        throw std::invalid_argument("Can't repeat 0 times.");
    }
    target_buf.append_tail(GateTarget{(uint32_t)blocks.size()});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count & 0xFFFFFFFFULL)});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count >> 32)});
    blocks.push_back(std::move(body));
    auto targets = target_buf.commit_tail();
    operations.push_back(CircuitInstruction(GateType::REPEAT, {}, targets, tag_buf.take_copy(tag)));
}

void Circuit::append_repeat_block(uint64_t repeat_count, const Circuit &body, std::string_view tag) {
    if (repeat_count == 0) {
        throw std::invalid_argument("Can't repeat 0 times.");
    }
    target_buf.append_tail(GateTarget{(uint32_t)blocks.size()});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count & 0xFFFFFFFFULL)});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count >> 32)});
    blocks.push_back(body);
    auto targets = target_buf.commit_tail();
    operations.push_back(CircuitInstruction(GateType::REPEAT, {}, targets, tag_buf.take_copy(tag)));
}

void stim::vec_pad_add_mul(std::vector<double> &target, SpanRef<const double> offset, uint64_t mul) {
    while (target.size() < offset.size()) {
        target.push_back(0);
    }
    for (size_t k = 0; k < offset.size(); k++) {
        target[k] += offset[k] * mul;
    }
}
