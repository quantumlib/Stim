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

#ifndef STIM_BASE_PYBIND_H
#define STIM_BASE_PYBIND_H

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <random>

std::mt19937_64 &PYBIND_SHARED_RNG();
std::string clean_doc_string(const char *c);
bool normalize_index_or_slice(const pybind11::object &index_or_slice, size_t length, pybind11::ssize_t *start, pybind11::ssize_t *step, pybind11::ssize_t *slice_length);

#endif
