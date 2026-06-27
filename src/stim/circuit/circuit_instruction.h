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
#include <cassert>
#include <span>


namespace stim {

struct Circuit;

struct CircuitInstruction {
    /// The gate applied by the operation.
    uint32_t gate_type;
    std::span<const uint32_t> targets;

    bool can_fuse(const CircuitInstruction &other) const;
    bool operator==(const CircuitInstruction &other) const;
    bool operator!=(const CircuitInstruction &other) const;
    bool approx_equals(const CircuitInstruction &other, double atol) const;

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
};

}  // namespace stim

#endif
