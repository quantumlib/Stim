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

#include "stim/circuit/gate_target.h"

using namespace stim;

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
GateTarget GateTarget::combiner() {
    return {TARGET_COMBINER};
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
