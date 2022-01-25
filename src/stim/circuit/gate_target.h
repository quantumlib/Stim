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

#ifndef _STIM_CIRCUIT_GATE_TARGET_H
#define _STIM_CIRCUIT_GATE_TARGET_H

#include <iostream>

#include "stim/circuit/gate_data.h"

namespace stim {

constexpr uint32_t TARGET_VALUE_MASK = (uint32_t{1} << 24) - uint32_t{1};
constexpr uint32_t TARGET_INVERTED_BIT = uint32_t{1} << 31;
constexpr uint32_t TARGET_PAULI_X_BIT = uint32_t{1} << 30;
constexpr uint32_t TARGET_PAULI_Z_BIT = uint32_t{1} << 29;
constexpr uint32_t TARGET_RECORD_BIT = uint32_t{1} << 28;
constexpr uint32_t TARGET_COMBINER = uint32_t{1} << 27;
constexpr uint32_t TARGET_SWEEP_BIT = uint32_t{1} << 26;

struct GateTarget {
    uint32_t data;
    int32_t value() const;

    static GateTarget x(uint32_t qubit, bool inverted = false);
    static GateTarget y(uint32_t qubit, bool inverted = false);
    static GateTarget z(uint32_t qubit, bool inverted = false);
    static GateTarget qubit(uint32_t qubit, bool inverted = false);
    static GateTarget rec(int32_t lookback);
    static GateTarget sweep_bit(uint32_t index);
    static GateTarget combiner();

    bool is_combiner() const;
    bool is_x_target() const;
    bool is_y_target() const;
    bool is_z_target() const;
    bool is_inverted_result_target() const;
    bool is_measurement_record_target() const;
    bool is_qubit_target() const;
    bool is_sweep_bit_target() const;
    uint32_t qubit_value() const;
    bool operator==(const GateTarget &other) const;
    bool operator!=(const GateTarget &other) const;
    bool operator<(const GateTarget &other) const;
    std::string str() const;
    std::string repr() const;

    void write_succinct(std::ostream &out) const;
};

std::ostream &operator<<(std::ostream &out, const GateTarget &t);

}  // namespace stim

#endif
