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

#include "stim/stabilizers/tableau_iter.pybind.h"

#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<TableauIterator> stim_pybind::pybind_tableau_iter(pybind11::module &m) {
    auto c = pybind11::class_<TableauIterator>(
        m,
        "TableauIterator",
        clean_doc_string(u8R"DOC(
            Iterates over all stabilizer tableaus of a specified size.

            Examples:
                >>> import stim
                >>> tableau_iterator = stim.Tableau.iter_all(1)
                >>> n = 0
                >>> for single_qubit_clifford in tableau_iterator:
                ...     n += 1
                >>> n
                24
        )DOC")
            .data());
    return c;
}

void stim_pybind::pybind_tableau_iter_methods(
    pybind11::module &m, pybind11::class_<TableauIterator> &c) {
    c.def(
        "__iter__",
        [](TableauIterator &self) -> TableauIterator {
            TableauIterator copy = self;
            return copy;
        },
        clean_doc_string(u8R"DOC(
            Returns an independent copy of the tableau iterator.

            Since for-loops and loop-comprehensions call `iter` on things they
            iterate, this effectively allows the iterator to be iterated
            multiple times.
        )DOC")
            .data());

    c.def(
        "__next__",
        [](TableauIterator &self) -> Tableau {
            if (!self.iter_next()) {
                throw pybind11::stop_iteration();
            }
            return self.result;
        },
        clean_doc_string(u8R"DOC(
            Returns the next iterated tableau.
        )DOC")
            .data());
}
