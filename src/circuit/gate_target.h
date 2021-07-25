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

#ifndef STIM_GATE_TARGET_H
#define STIM_GATE_TARGET_H

#include <iostream>

#include "gate_data.h"

namespace stim_internal {

#define TARGET_VALUE_MASK ((uint32_t{1} << 24) - uint32_t{1})
#define TARGET_INVERTED_BIT (uint32_t{1} << 31)
#define TARGET_PAULI_X_BIT (uint32_t{1} << 30)
#define TARGET_PAULI_Z_BIT (uint32_t{1} << 29)
#define TARGET_RECORD_BIT (uint32_t{1} << 28)
#define TARGET_COMBINER (uint32_t{1} << 27)

struct GateTarget {
    uint32_t data;
    int32_t value() const;

    static GateTarget x(uint32_t qubit, bool inverted = false);
    static GateTarget y(uint32_t qubit, bool inverted = false);
    static GateTarget z(uint32_t qubit, bool inverted = false);
    static GateTarget qubit(uint32_t qubit, bool inverted = false);
    static GateTarget rec(int32_t lookback);
    static GateTarget combiner();

    bool is_combiner() const;
    bool is_x_target() const;
    bool is_y_target() const;
    bool is_z_target() const;
    bool is_inverted_result_target() const;
    bool is_measurement_record_target() const;
    bool is_qubit_target() const;
    uint32_t qubit_value() const;
    bool operator==(const GateTarget &other) const;
    bool operator!=(const GateTarget &other) const;
    bool operator<(const GateTarget &other) const;
    std::string str() const;
    std::string repr() const;
};

std::ostream &operator<<(std::ostream &out, const GateTarget &t);

}  // namespace stim_internal

#endif
