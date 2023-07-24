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

#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/stabilizers/pauli_string.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<PauliStringIterator<MAX_BITWORD_WIDTH>> stim_pybind::pybind_pauli_string_iter(pybind11::module &m) {
    auto c = pybind11::class_<PauliStringIterator<MAX_BITWORD_WIDTH>>(
        m,
        "PauliStringIterator",
        clean_doc_string(R"DOC(
            Iterates over all possible pauli_strings of weight specfied by
            min_weight and max_weight.

            Returns:
                An Iterable[stim.PauliString] that yields the requested Pauli
                    string.

            Examples:
                >>> import stim
                >>> pauli_iter = stim.PauliString.iter_all(10, min_weight=2, max_weight=3)
                >>> n = 0
                >>> for pauli_string in pauli_iter:
                ...     n += 1
                >>> n
                3645
        )DOC")
            .data());
    return c;
}

void stim_pybind::pybind_pauli_string_iter_methods(
    pybind11::module &m, pybind11::class_<PauliStringIterator<stim::MAX_BITWORD_WIDTH>> &c) {
    c.def(
        "__iter__",
        [](PauliStringIterator<stim::MAX_BITWORD_WIDTH> &self) -> PauliStringIterator<stim::MAX_BITWORD_WIDTH> {
            PauliStringIterator<stim::MAX_BITWORD_WIDTH> copy = self;
            return copy;
        },
        clean_doc_string(R"DOC(
            Returns an independent copy of the pauli string iterator.
        )DOC")
            .data());

    c.def(
        "__next__",
        [](PauliStringIterator<stim::MAX_BITWORD_WIDTH> &self) -> PyPauliString {
            if (!self.iter_next()) {
                throw pybind11::stop_iteration();
            }
            return {self.result, false};
        },
        clean_doc_string(R"DOC(
            Returns the next iterated pauli string.
        )DOC")
            .data());

    c.def(
        "__internal_next_qubit_permutation",
        [](PauliStringIterator<stim::MAX_BITWORD_WIDTH> &self) {
            self.next_qubit_permutation(self.cur_perm);
            return simd_bits_to_numpy(self.cur_perm, self.result.num_qubits, false);
        },
        clean_doc_string(R"DOC(
            [DEPRECATED] Get the next permutation of qubit labels.

            It's alot easier to test more complicated edge cases in python
            which largely arise due to the algorithm for generating the next
            permutation.

            The user should not interact with this function.

            Returns:
                next_perm: numpy boolean array which represents the next qubit permutation

            Examples:
                >>> import stim
                >>> pauli_iter = stim.PauliString.iter_all(5, min_weight=3, max_weight=5)
                >>> pauli_iter.next_qubit_permutation()
                array([ True,  True, False,  True, False])
        )DOC")
            .data());

    c.def(
        "seed_iterator",
        [](PauliStringIterator<stim::MAX_BITWORD_WIDTH> &self, const pybind11::object &permutation) {
            size_t n = numpy_to_size(permutation, self.result.num_qubits);
            stim_pybind::memcpy_bits_from_numpy_to_simd(n, permutation, self.cur_perm);
        },
        clean_doc_string(R"DOC(
            Seed the iterator with a given qubit pattern.

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> pauli_iter = stim.PauliString.iter_all(4, min_weight=2, max_weight=2)
                >>> seed = np.array([False, True, True, False])
                >>> pauli_iter.seed_iterator(seed)
                >>> pauli_iter.next_qubit_permutation()
                array([ True, False, False,  True])
        )DOC")
            .data());
}
