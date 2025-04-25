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

#ifndef _STIM_STABILIZERS_FLOW_H
#define _STIM_STABILIZERS_FLOW_H

#include "stim/stabilizers/pauli_string.h"

namespace stim {

template <size_t W>
struct Flow {
    /// The stabilizer at the beginning of the circuit.
    stim::PauliString<W> input;
    /// The equivalent stabilizer at the end the circuit.
    stim::PauliString<W> output;
    /// The measurements mediating the equivalence.
    /// Indexing follows python convention: -1 is the last element, 0 is the first element.
    /// ENTRIES MUST BE SORTED AND UNIQUE.
    std::vector<int32_t> measurements;
    /// Indices of observables included in the flow.
    /// Determines which OBSERVABLE_INCLUDE instructions are included.
    /// ENTRIES MUST BE SORTED AND UNIQUE.
    std::vector<uint32_t> observables;

    /// Fixes non-unique non-sorted measurements and observables.
    void canonicalize();

    static Flow<W> from_str(std::string_view text);
    Flow<W> operator*(const Flow<W> &rhs) const;
    bool operator<(const Flow<W> &other) const;
    bool operator==(const Flow<W> &other) const;
    bool operator!=(const Flow<W> &other) const;
    std::string str() const;
};

template <size_t W>
std::ostream &operator<<(std::ostream &out, const Flow<W> &flow);

}  // namespace stim

#include "stim/stabilizers/flow.inl"

#endif
