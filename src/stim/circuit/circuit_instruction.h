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
    uint64_t count_measurement_results() const;
};

}  // namespace stim

#endif
