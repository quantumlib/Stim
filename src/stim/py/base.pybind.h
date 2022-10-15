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

#ifndef _STIM_PY_BASE_PYBIND_H
#define _STIM_PY_BASE_PYBIND_H

#include <pybind11/complex.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <random>

#include "stim/circuit/circuit.h"
#include "stim/io/stim_data_formats.h"

namespace stim_pybind {

std::shared_ptr<std::mt19937_64> make_py_seeded_rng(const pybind11::object &seed);
std::string clean_doc_string(const char *c);
stim::SampleFormat format_to_enum(const std::string &format);
bool normalize_index_or_slice(
    const pybind11::object &index_or_slice,
    size_t length,
    pybind11::ssize_t *start,
    pybind11::ssize_t *step,
    pybind11::ssize_t *slice_length);

template <typename T>
pybind11::tuple tuple_tree(const std::vector<T> &val, size_t offset = 0) {
    // A workaround for https://github.com/pybind/pybind11/issues/1928
    if (offset >= val.size()) {
        return pybind11::make_tuple();
    }
    if (offset + 1 == val.size()) {
        return pybind11::make_tuple(val[offset]);
    }
    return pybind11::make_tuple(val[offset], tuple_tree(val, offset + 1));
}

}  // namespace stim_pybind

#endif
