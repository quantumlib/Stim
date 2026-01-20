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
#include "stim/cmd/command_help.h"
#include "stim/io/stim_data_formats.h"

namespace stim_pybind {

std::mt19937_64 make_py_seeded_rng(const pybind11::object &seed);
stim::SampleFormat format_to_enum(std::string_view format);

/// Converts a python index or slice into a form that's easier to consume.
///
/// In particular, replaces negative indices with non-negative indices.
/// When the object is an integer, it is treated as a slice over a single
/// index. The boolean returned from the method can be used to distinguish
/// a single-index slice from an integer index.
///
/// The result can be consumed as follows:
///
///     pybind11::ssize_t slice_start;
///     pybind11::ssize_t slice_step;
///     pybind11::ssize_t slice_length;
///     bool was_slice = normalize_index_or_slice(
///        obj,
///        collection_length,
///        &slice_start,
///        &slice_step,
///        &slice_length)
///     for (size_t k = 0; k < (size_t)slice_length; k++) {
///         size_t target_k = index + step * k;
///         auto &target = collection[slice_start + slice_step*k];
///         ...
///     }
///
/// Args:
///     index_or_slice: An int or slice object.
///     length: The length of the collection being indexed or sliced.
///     start: Output address for the offset to use when looping.
///         The value written to this pointer will be non-negative
//          (unless an exception is thrown).
///     step: Output address for the stride to use when looping.
///         The value written to this pointer may be negative.
///     slice_length: Output address for how many iterations to run the loop.
///         The value written to this pointer will be non-negative
//          (unless an exception is thrown).
///
/// Returns:
///     True: the given object was a slice.
///     False: the given object was an integer index.
///
/// Raises:
///     invalid_argument: The given object was not a valid slice or index for
///         the given collection length;
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
