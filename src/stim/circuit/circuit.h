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

#ifndef _STIM_CIRCUIT_CIRCUIT_H
#define _STIM_CIRCUIT_CIRCUIT_H

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "stim/circuit/circuit_instruction.h"
#include "stim/circuit/gate_target.h"
#include "stim/gates/gates.h"
#include "stim/mem/monotonic_buffer.h"
#include "stim/mem/span_ref.h"

namespace stim {

uint64_t add_saturate(uint64_t a, uint64_t b);
uint64_t mul_saturate(uint64_t a, uint64_t b);

/// A description of a quantum computation.
struct Circuit {
    /// Backing data stores for variable-sized target data referenced by operations.
    MonotonicBuffer<GateTarget> target_buf;
    MonotonicBuffer<double> arg_buf;
    MonotonicBuffer<char> tag_buf;
    /// Operations in the circuit, from earliest to latest.
    std::vector<CircuitInstruction> operations;
    std::vector<Circuit> blocks;

    // Returns one more than the largest `k` from any qubit target `k` or `!k` or `{X,Y,Z}k`.
    size_t count_qubits() const;
    // Returns the number of measurement bits produced by the circuit.
    uint64_t count_measurements() const;
    // Returns the number of detection event bits produced by the circuit.
    uint64_t count_detectors() const;
    // Returns one more than the largest `k` from any `OBSERVABLE_INCLUDE(k)` instruction.
    uint64_t count_observables() const;
    // Returns the number of ticks performed when running the circuit.
    uint64_t count_ticks() const;
    // Returns the largest `k` from any `rec[-k]` target.
    size_t max_lookback() const;
    // Returns one more than the largest `k` from any `sweep[k]` target.
    size_t count_sweep_bits() const;

};

}  // namespace stim

#endif
