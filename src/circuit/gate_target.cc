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

#include "gate_target.h"

using namespace stim_internal;

GateTarget GateTarget::x(uint32_t qubit, bool inverted) {
    if (qubit != (qubit & TARGET_VALUE_MASK)) {
        throw std::invalid_argument("qubit target larger than " + std::to_string(TARGET_VALUE_MASK));
    }
    return {qubit | (TARGET_INVERTED_BIT * inverted) | TARGET_PAULI_X_BIT};
}
GateTarget GateTarget::y(uint32_t qubit, bool inverted) {
    if (qubit != (qubit & TARGET_VALUE_MASK)) {
        throw std::invalid_argument("qubit target larger than " + std::to_string(TARGET_VALUE_MASK));
    }
    return {qubit | (TARGET_INVERTED_BIT * inverted) | TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT};
}
GateTarget GateTarget::z(uint32_t qubit, bool inverted) {
    if (qubit != (qubit & TARGET_VALUE_MASK)) {
        throw std::invalid_argument("qubit target larger than " + std::to_string(TARGET_VALUE_MASK));
    }
    return {qubit | (TARGET_INVERTED_BIT * inverted) | TARGET_PAULI_Z_BIT};
}
GateTarget GateTarget::qubit(uint32_t qubit, bool inverted) {
    if (qubit != (qubit & TARGET_VALUE_MASK)) {
        throw std::invalid_argument("qubit target larger than " + std::to_string(TARGET_VALUE_MASK));
    }
    return {qubit | (TARGET_INVERTED_BIT * inverted)};
}
GateTarget GateTarget::rec(int32_t lookback) {
    if (lookback >= 0 || lookback < -(int32_t)TARGET_VALUE_MASK) {
        throw std::invalid_argument("lookback further than " + std::to_string(-(int32_t)TARGET_VALUE_MASK));
    }
    return {((uint32_t)-lookback) | TARGET_RECORD_BIT};
}
GateTarget GateTarget::combiner() {
    return {TARGET_COMBINER};
}

uint32_t GateTarget::qubit_value() const {
    return data & TARGET_VALUE_MASK;
}

int32_t GateTarget::value() const {
    int32_t result = (int32_t)(data & TARGET_VALUE_MASK);
    if (is_measurement_record_target()) {
        return -result;
    }
    return result;
}
bool GateTarget::is_x_target() const {
    return (data & TARGET_PAULI_X_BIT) && !(data & TARGET_PAULI_Z_BIT);
}
bool GateTarget::is_y_target() const {
    return (data & TARGET_PAULI_X_BIT) && (data & TARGET_PAULI_Z_BIT);
}
bool GateTarget::is_z_target() const {
    return !(data & TARGET_PAULI_X_BIT) && (data & TARGET_PAULI_Z_BIT);
}
bool GateTarget::is_inverted_result_target() const {
    return data & TARGET_INVERTED_BIT;
}
bool GateTarget::is_measurement_record_target() const {
    return data & TARGET_RECORD_BIT;
}
bool GateTarget::is_qubit_target() const {
    return !(data & (TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT | TARGET_RECORD_BIT | TARGET_COMBINER));
}
bool GateTarget::is_combiner() const {
    return data == TARGET_COMBINER;
}
bool GateTarget::operator==(const GateTarget &other) const {
    return data == other.data;
}
bool GateTarget::operator<(const GateTarget &other) const {
    return data < other.data;
}
bool GateTarget::operator!=(const GateTarget &other) const {
    return data != other.data;
}

std::ostream &stim_internal::operator<<(std::ostream &out, const GateTarget &t) {
    if (t.is_combiner()) {
        return out << "stim.GateTarget.combiner()";
    }
    if (t.is_qubit_target()) {
        if (t.is_inverted_result_target()) {
            return out << "stim.target_inv(" << t.value() << ")";
        }
        return out << t.value();
    }
    if (t.is_measurement_record_target()) {
        return out << "stim.target_rec(" << t.value() << ")";
    }
    if (t.is_x_target()) {
        out << "stim.target_x(" << t.value();
        if (t.is_inverted_result_target()) {
            out << ", invert=True";
        }
        return out << ")";
    }
    if (t.is_y_target()) {
        out << "stim.target_y(" << t.value();
        if (t.is_inverted_result_target()) {
            out << ", invert=True";
        }
        return out << ")";
    }
    if (t.is_z_target()) {
        out << "stim.target_z(" << t.value();
        if (t.is_inverted_result_target()) {
            out << ", invert=True";
        }
        return out << ")";
    }
    throw std::invalid_argument("Malformed target.");
}

std::string GateTarget::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::string GateTarget::repr() const {
    std::stringstream ss;
    ss << "stim.GateTarget(";
    ss << *this;
    ss << ")";
    return ss.str();
}
