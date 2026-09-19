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

#include "stim/stabilizers/tableau_sampler.pybind.h"

#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

TableauSampler::TableauSampler(size_t num_qubits, std::mt19937_64 &&rng)
    : num_qubits(num_qubits), rng(std::move(rng)) {
}

Tableau<MAX_BITWORD_WIDTH> TableauSampler::next_tableau() {
    return Tableau<MAX_BITWORD_WIDTH>::random(num_qubits, rng);
}

std::string TableauSampler::repr() const {
    std::stringstream result;
    result << "stim.TableauSampler(num_qubits=";
    result << num_qubits;
    result << ")";
    return result.str();
}

pybind11::class_<TableauSampler> stim_pybind::pybind_tableau_sampler(pybind11::module &m) {
    return pybind11::class_<TableauSampler>(
        m,
        "TableauSampler",
        clean_doc_string(R"DOC(
            A tool for pseudo-random tableau sampling.

            Seeds the random number generator once at initialization, then
            produces a reproducible sequence of random tableaus via repeated
            calls to `next_tableau()`.

            Examples:
                >>> import stim
                >>> s = stim.TableauSampler(5, seed=42)
                >>> t1 = s.next_tableau()
                >>> t2 = s.next_tableau()
        )DOC")
            .data());
}

TableauSampler stim_pybind::py_init_tableau_sampler(size_t num_qubits, const pybind11::object &seed) {
    return TableauSampler(num_qubits, make_py_seeded_rng(seed));
}

void stim_pybind::pybind_tableau_sampler_methods(
    pybind11::module &m, pybind11::class_<TableauSampler> &c) {
    c.def(
        pybind11::init(&py_init_tableau_sampler),
        pybind11::arg("num_qubits"),
        pybind11::kw_only(),
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(R"DOC(
            Creates a tableau sampler.

            Args:
                num_qubits: The number of qubits each sampled tableau acts on.
                seed: PARTIALLY determines the sequence of sampled tableaus by
                    deterministically seeding the random number generator.

                    Must be None or an integer in range(2**64).

                    Defaults to None. When None, the prng is seeded from system
                    entropy.

                    When set to an integer, making the exact same series of calls
                    on the exact same machine with the exact same version of Stim
                    will produce the exact same sequence of tableaus.

                    CAUTION: the sequence produced by a specific seed *WILL NOT*
                    be consistent between versions of Stim. This restriction is
                    present to make it possible to have future optimizations to
                    the random sampling, and is enforced by introducing
                    intentional differences in the seeding strategy from version
                    to version.

                    CAUTION: the sequence produced by a specific seed *MAY NOT*
                    be consistent across machines that differ in the width of
                    supported SIMD instructions. For example, using the same seed
                    on a machine that supports AVX instructions and one that only
                    supports SSE instructions may produce different sequences.

            Examples:
                >>> import stim
                >>> sampler = stim.TableauSampler(4, seed=12345)
                >>> t = sampler.next_tableau()
        )DOC")
            .data());

    c.def(
        "next_tableau",
        [](TableauSampler &self) {
            return self.next_tableau();
        },
        clean_doc_string(R"DOC(
            Samples a uniformly random tableau.

            Returns:
                A uniformly random `stim.Tableau` over the sampler's `num_qubits`.

            Examples:
                >>> import stim
                >>> sampler = stim.TableauSampler(2, seed=42)
                >>> t1 = sampler.next_tableau()
                >>> t2 = sampler.next_tableau()
        )DOC")
            .data());

    c.def(
        "__repr__",
        &TableauSampler::repr,
        "Returns a string representation of the `stim.TableauSampler`.");
}
