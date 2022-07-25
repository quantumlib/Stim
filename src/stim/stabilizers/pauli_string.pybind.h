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

#ifndef _STIM_STABILIZERS_PAULI_STRING_PYBIND_H
#define _STIM_STABILIZERS_PAULI_STRING_PYBIND_H

#include <complex>
#include <pybind11/pybind11.h>

#include "stim/stabilizers/pauli_string.h"

namespace stim_pybind {

struct PyPauliString {
    stim::PauliString value;
    bool imag;

    PyPauliString(const stim::PauliStringRef val, bool imag = false);
    PyPauliString(stim::PauliString &&val, bool imag = false);

    static PyPauliString from_text(const char *c);
    std::complex<float> get_phase() const;

    PyPauliString operator+(const PyPauliString &rhs) const;
    PyPauliString &operator+=(const PyPauliString &rhs);

    PyPauliString operator*(const pybind11::object &rhs) const;
    PyPauliString operator*(size_t power) const;
    PyPauliString operator*(std::complex<float> scale) const;
    PyPauliString operator*(const PyPauliString &rhs) const;
    PyPauliString operator/(const std::complex<float> &divisor) const;

    PyPauliString &operator*=(const pybind11::object &rhs);
    PyPauliString &operator*=(size_t power);
    PyPauliString &operator*=(std::complex<float> scale);
    PyPauliString &operator*=(const PyPauliString &rhs);
    PyPauliString &operator/=(const std::complex<float> &divisor);

    bool operator==(const PyPauliString &other) const;
    bool operator!=(const PyPauliString &other) const;
    std::string str() const;
};

void pybind_pauli_string(pybind11::module &m);

}  // namespace stim_pybind

#endif
