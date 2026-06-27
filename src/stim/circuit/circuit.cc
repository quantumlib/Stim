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

enum class READ_CONDITION {
    READ_AS_LITTLE_AS_POSSIBLE,
    READ_UNTIL_END_OF_BLOCK,
    READ_UNTIL_END_OF_FILE,
};

/// Concatenates the second pointer range's data into the first.
/// Typically, the two ranges are contiguous and so this only requires advancing the end of the destination region.
/// In cases where that doesn't occur, space is created in the given monotonic buffer to store the result and both
/// the start and end of the destination range move.
void fuse_data(SpanRef<const GateTarget> &dst, SpanRef<const GateTarget> src, MonotonicBuffer<GateTarget> &buf) {
    if (dst.ptr_end != src.ptr_start) {
        buf.ensure_available(src.size() + dst.size());
        dst = buf.take_copy(dst);
        src = buf.take_copy(src);
    }
    assert(dst.ptr_end == src.ptr_start);
    dst.ptr_end = src.ptr_end;
}

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

bool Circuit::operator==(const Circuit &other) const {
    return true;
}
bool Circuit::operator!=(const Circuit &other) const {
    return !(*this == other);
}
bool Circuit::approx_equals(const Circuit &other, double atol) const {
    return true;
}

inline bool is_name_char(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
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

    if (!block_fusion && !operations.empty() && operations.back().can_fuse(operation)) {
        // Extend targets of last gate.
        fuse_data(operations.back().targets, operation.targets, target_buf);
    } else {
        // Add a fresh new operation with its own target data.
        operations.push_back(operation);
    }
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
}

void Circuit::safe_insert_repeat_block(
    size_t index, uint64_t repeat_count, const Circuit &block, std::string_view tag) {
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

    if (!operations.empty() && operations.back().can_fuse(to_add)) {
        // Extend targets of last gate.
        fuse_data(operations.back().targets, to_add.targets, target_buf);
    } else {
        // Add a fresh new operation with its own target data.
        operations.push_back(to_add);
    }
}

void Circuit::clear() {
    target_buf.clear();
    arg_buf.clear();
    operations.clear();
    blocks.clear();
}

Circuit Circuit::operator+(const Circuit &other) const {
    Circuit result = *this;
    result += other;
    return result;
}
Circuit Circuit::operator*(uint64_t repetitions) const {
    Circuit result;
    return result;
}

/// Helper method for fusing during concatenation. If the data being extended is at the end of
/// the monotonic buffer and there's space for the additional data, put it there in place.
/// Otherwise it needs to be copied to the new location.
///
/// CAUTION: This violates the usual guarantee that once data is committed to a monotonic
/// buffer it cannot be moved. The old data is still readable in its original location, but
/// the caller is responsible for guaranteeing that no dangling writeable pointers remain
/// that point to the old location (since they will write data that is no longer read by
/// other parts of the code).
template <typename T>
SpanRef<const T> mono_extend(MonotonicBuffer<T> &cur, SpanRef<const T> original, SpanRef<const T> additional) {
    if (original.ptr_end == cur.tail.ptr_start) {
        // Try to append new data right after the original data.
        cur.ensure_available(additional.size());
        if (original.ptr_end == cur.tail.ptr_start) {
            cur.append_tail(additional);
            auto added = cur.commit_tail();
            return {original.ptr_start, added.ptr_end};
        }
    }

    // Ensure necessary space is available, plus some padding to avoid quadratic behavior when repeatedly extending.
    cur.ensure_available((int)(1.1 * (original.size() + additional.size())) + 10);
    cur.append_tail(original);
    cur.append_tail(additional);
    return cur.commit_tail();
}

Circuit &Circuit::operator+=(const Circuit &other) {
    SpanRef<const CircuitInstruction> ops_to_add = other.operations;
    if (!operations.empty() && !ops_to_add.empty() && operations.back().can_fuse(ops_to_add[0])) {
        operations.back().targets = mono_extend(target_buf, operations.back().targets, ops_to_add[0].targets);
        ops_to_add.ptr_start++;
    }

    if (&other == this) {
        operations.insert(operations.end(), ops_to_add.begin(), ops_to_add.end());
        return *this;
    }

    uint32_t block_offset = (uint32_t)blocks.size();
    blocks.insert(blocks.end(), other.blocks.begin(), other.blocks.end());
    for (const auto &op : ops_to_add) {
        SpanRef<stim::GateTarget> target_data = target_buf.take_copy(op.targets);
        if (op.gate_type == GateType::REPEAT) {
            assert(op.targets.size() == 3);
            target_data[0].data += block_offset;
        }
        SpanRef<double> arg_data = arg_buf.take_copy(op.args);
        std::string_view tag_data = tag_buf.take_copy(op.tag);
        operations.push_back(CircuitInstruction(op.gate_type, arg_data, target_data, tag_data));
    }

    return *this;
}
Circuit &Circuit::operator*=(uint64_t repetitions) {
    if (repetitions == 0) {
        clear();
    } else {
        *this = *this * repetitions;
    }
    return *this;
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
    return 0;
}

uint64_t Circuit::count_detectors() const {
    return 0;
}

uint64_t Circuit::count_ticks() const {
    return 0;
}

uint64_t Circuit::count_observables() const {
    return 0;
}

size_t Circuit::count_sweep_bits() const {
    return 0;
}

Circuit Circuit::py_get_slice(int64_t start, int64_t step, int64_t slice_length) const {
    assert(slice_length >= 0);
    assert(slice_length == 0 || start >= 0);
    Circuit result;
    for (size_t k = 0; k < (size_t)slice_length; k++) {
        const auto &op = operations[start + step * k];
        if (op.gate_type == GateType::REPEAT) {
            result.target_buf.append_tail(GateTarget{(uint32_t)result.blocks.size()});
            result.target_buf.append_tail(op.targets[1]);
            result.target_buf.append_tail(op.targets[2]);
            auto targets = result.target_buf.commit_tail();
            auto tag = result.tag_buf.take_copy(op.tag);
            result.blocks.push_back(op.repeat_block_body(*this));
            result.operations.push_back(CircuitInstruction(op.gate_type, {}, targets, tag));
        } else {
            auto args = result.arg_buf.take_copy(op.args);
            auto targets = result.target_buf.take_copy(op.targets);
            auto tag = result.tag_buf.take_copy(op.tag);
            result.operations.push_back({op.gate_type, args, targets, tag});
        }
    }
    return result;
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
