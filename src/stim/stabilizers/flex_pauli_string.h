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

#ifndef _STIM_STABILIZERS_FLEX_PAULI_STRING_H
#define _STIM_STABILIZERS_FLEX_PAULI_STRING_H

#include <complex>

#include "stim/mem/sparse_xor_vec.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

/// This is the backing class for the python stim.PauliString.
/// It's more flexible than the C++ stim::PauliString.
/// For example, it allows imaginary signs.
struct FlexPauliString {
    stim::PauliString<stim::MAX_BITWORD_WIDTH> value;
    bool imag;

    FlexPauliString(size_t num_qubits);
    FlexPauliString(const stim::PauliStringRef<stim::MAX_BITWORD_WIDTH> val, bool imag = false);
    FlexPauliString(stim::PauliString<stim::MAX_BITWORD_WIDTH> &&val, bool imag = false);

    static FlexPauliString from_text(std::string_view text);
    std::complex<float> get_phase() const;

    FlexPauliString operator+(const FlexPauliString &rhs) const;
    FlexPauliString &operator+=(const FlexPauliString &rhs);

    FlexPauliString operator*(size_t power) const;
    FlexPauliString operator*(std::complex<float> scale) const;
    FlexPauliString operator*(const FlexPauliString &rhs) const;
    FlexPauliString operator/(const std::complex<float> &divisor) const;

    FlexPauliString &operator*=(size_t power);
    FlexPauliString &operator*=(std::complex<float> scale);
    FlexPauliString &operator*=(const FlexPauliString &rhs);
    FlexPauliString &operator/=(const std::complex<float> &divisor);

    bool operator==(const FlexPauliString &other) const;
    bool operator!=(const FlexPauliString &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const FlexPauliString &v);

}  // namespace stim

#endif
