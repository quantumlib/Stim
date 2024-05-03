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

#include "stim/stabilizers/pauli_string_iter.pybind.h"

#include "pauli_string.pybind.h"
#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<PauliStringIterator<MAX_BITWORD_WIDTH>> stim_pybind::pybind_pauli_string_iter(pybind11::module &m) {
    auto c = pybind11::class_<PauliStringIterator<MAX_BITWORD_WIDTH>>(
        m,
        "PauliStringIterator",
        clean_doc_string(R"DOC(
            Iterates over all pauli strings matching specified patterns.

            Examples:
                >>> import stim
                >>> pauli_string_iterator = stim.PauliString.iter_all(
                ...     2,
                ...     min_weight=1,
                ...     max_weight=1,
                ...     allowed_paulis="XZ",
                ... )
                >>> for p in pauli_string_iterator:
                ...     print(p)
                +X_
                +Z_
                +_X
                +_Z
        )DOC")
            .data());
    return c;
}

void stim_pybind::pybind_pauli_string_iter_methods(
    pybind11::module &m, pybind11::class_<PauliStringIterator<MAX_BITWORD_WIDTH>> &c) {
    c.def(
        "__iter__",
        [](PauliStringIterator<MAX_BITWORD_WIDTH> &self) -> PauliStringIterator<MAX_BITWORD_WIDTH> {
            PauliStringIterator<MAX_BITWORD_WIDTH> copy = self;
            return copy;
        },
        clean_doc_string(R"DOC(
            Returns an independent copy of the pauli string iterator.

            Since for-loops and loop-comprehensions call `iter` on things they
            iterate, this effectively allows the iterator to be iterated
            multiple times.
        )DOC")
            .data());

    c.def(
        "__next__",
        [](PauliStringIterator<MAX_BITWORD_WIDTH> &self) -> FlexPauliString {
            if (!self.iter_next()) {
                throw pybind11::stop_iteration();
            }
            return FlexPauliString(self.result);
        },
        clean_doc_string(R"DOC(
            Returns the next iterated pauli string.
        )DOC")
            .data());
}
