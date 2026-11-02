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

#ifndef _STIM_STABILIZERS_TABLEAU_SAMPLER_PYBIND_H
#define _STIM_STABILIZERS_TABLEAU_SAMPLER_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/stabilizers/tableau.h"

namespace stim_pybind {

struct TableauSampler {
    size_t num_qubits;
    std::mt19937_64 rng;
    TableauSampler() = delete;
    TableauSampler(const TableauSampler &) = delete;
    TableauSampler(TableauSampler &&) = default;
    TableauSampler(size_t num_qubits, std::mt19937_64 &&rng);
    stim::Tableau<stim::MAX_BITWORD_WIDTH> next_tableau();
    std::string repr() const;
};

pybind11::class_<TableauSampler> pybind_tableau_sampler(pybind11::module &m);
void pybind_tableau_sampler_methods(pybind11::module &m, pybind11::class_<TableauSampler> &c);
TableauSampler py_init_tableau_sampler(size_t num_qubits, const pybind11::object &seed);

}  // namespace stim_pybind

#endif
