/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_CIRCUIT_INSTRUCTION_H
#define _STIM_CIRCUIT_INSTRUCTION_H

#include <cstdint>

#include "stim/circuit/gate_target.h"
#include "stim/mem/span_ref.h"

namespace stim {

struct Circuit;

/// Stores a variety of circuit quantities relevant for sizing memory.
struct CircuitStats {
    uint64_t num_detectors = 0;
    uint64_t num_observables = 0;
    uint64_t num_measurements = 0;
    uint32_t num_qubits = 0;
    uint64_t num_ticks = 0;
    uint32_t max_lookback = 0;
    uint32_t num_sweep_bits = 0;

    inline CircuitStats repeated(uint64_t repetitions) const {
        return CircuitStats{
            num_detectors * repetitions,
            num_observables,
            num_measurements * repetitions,
            num_qubits,
            (uint32_t)(num_ticks * repetitions),
            max_lookback,
            num_sweep_bits,
        };
    }
};

/// The data that describes how a gate is being applied to qubits (or other targets).
///
/// A gate applied to targets.
///
/// This struct is not self-sufficient. It points into data stored elsewhere (e.g. in a Circuit's jagged_data).
struct CircuitInstruction {
    /// The gate applied by the operation.
    GateType gate_type;

    /// Numeric arguments varying the functionality of the gate.
    ///
    /// The meaning of the numbers varies from gate to gate.
    /// Examples:
    ///     X_ERROR(p) has a single argument: probability of X.
    ///     PAULI_CHANNEL_1(px,py,pz) has multiple probability arguments.
    ///     DETECTOR(c1,c2) has variable arguments: coordinate data.
    ///     OBSERVABLE_INCLUDE(k) has a single argument: the observable index.
    SpanRef<const double> args;

    /// Encoded data indicating the qubits and other targets acted on by the gate.
    SpanRef<const GateTarget> targets;

    /// Arbitrary string associated with the instruction.
    /// No effect on simulations or analysis steps within stim, but user code may use it.
    std::string_view tag;

    CircuitInstruction() = delete;
    CircuitInstruction(
        GateType gate_type, SpanRef<const double> args, SpanRef<const GateTarget> targets, std::string_view tag);

    /// Computes number of qubits, number of measurements, etc.
    CircuitStats compute_stats(const Circuit *host) const;
    /// Computes number of qubits, number of measurements, etc and adds them into a target.
    void add_stats_to(CircuitStats &out, const Circuit *host) const;

    /// Determines if two operations can be combined into one operation (with combined targeting data).
    ///
    /// For example, `H 1` then `H 2 1` is equivalent to `H 1 2 1` so those instructions are fusable.
    bool can_fuse(const CircuitInstruction &other) const;
    /// Equality.
    bool operator==(const CircuitInstruction &other) const;
    /// Inequality.
    bool operator!=(const CircuitInstruction &other) const;
    /// Approximate equality.
    bool approx_equals(const CircuitInstruction &other, double atol) const;
    /// Returns a text description of the instruction, as would appear in a STIM circuit file.
    std::string str() const;

    /// Determines the number of entries added to the measurement record by the operation.
    ///
    /// Note: invalid to use this on REPEAT blocks.
    uint64_t count_measurement_results() const;

    uint64_t repeat_block_rep_count() const;
    Circuit &repeat_block_body(Circuit &host) const;
    const Circuit &repeat_block_body(const Circuit &host) const;

    /// Verifies complex invariants that circuit instructions are supposed to follow.
    ///
    /// For example: CNOT gates should have an even number of targets.
    /// For example: X_ERROR should have a single float argument between 0 and 1 inclusive.
    ///
    /// Raises:
    ///     std::invalid_argument: Validation failed.
    void validate() const;

    template <typename CALLBACK>
    inline void for_combined_target_groups(CALLBACK callback) const {
        auto flags = GATE_DATA[gate_type].flags;
        size_t start = 0;
        while (start < targets.size()) {
            size_t end;
            if (flags & stim::GateFlags::GATE_TARGETS_COMBINERS) {
                end = start + 1;
                while (end < targets.size() && targets[end].is_combiner()) {
                    end += 2;
                }
            } else if (flags & stim::GateFlags::GATE_IS_SINGLE_QUBIT_GATE) {
                end = start + 1;
            } else if (flags & stim::GateFlags::GATE_TARGETS_PAIRS) {
                end = start + 2;
            } else if (
                (flags & stim::GateFlags::GATE_TARGETS_PAULI_STRING) &&
                !(flags & stim::GateFlags::GATE_TARGETS_COMBINERS)) {
                // like CORRELATED_ERROR
                end = targets.size();
            } else if (flags & stim::GateFlags::GATE_ONLY_TARGETS_MEASUREMENT_RECORD) {
                // like DETECTOR
                end = start + 1;
            } else if (gate_type == GateType::MPAD || gate_type == GateType::QUBIT_COORDS) {
                end = start + 1;
            } else {
                throw std::invalid_argument("Not implemented: splitting " + str());
            }
            std::span<const GateTarget> group = targets.sub(start, end);
            callback(group);
            start = end;
        }
    }
};

void write_tag_escaped_string_to(std::string_view tag, std::ostream &out);

std::ostream &operator<<(std::ostream &out, const CircuitInstruction &op);

}  // namespace stim

#endif
