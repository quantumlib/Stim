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

#include "stim/py/base.pybind.h"

#include <memory>

#include "stim/probability_util.h"

using namespace stim;

static std::shared_ptr<std::mt19937_64> shared_rng;

std::shared_ptr<std::mt19937_64> PYBIND_SHARED_RNG(const pybind11::object &seed) {
    if (seed.is(pybind11::none())) {
        if (!shared_rng) {
            shared_rng = std::make_shared<std::mt19937_64>(std::move(externally_seeded_rng()));
        }
        return shared_rng;
    }

    try {
        uint64_t s = pybind11::cast<uint64_t>(seed) ^ INTENTIONAL_VERSION_SEED_INCOMPATIBILITY;
        return std::make_shared<std::mt19937_64>(s);
    } catch (const pybind11::cast_error &) {
        throw std::invalid_argument("Expected seed to be None or a 64 bit unsigned integer.");
    }
}

std::string clean_doc_string(const char *c) {
    // Skip leading empty lines.
    while (*c == '\n') {
        c++;
    }

    // Determine indentation using first non-empty line.
    size_t indent = 0;
    while (*c == ' ') {
        indent++;
        c++;
    }

    std::string result;
    while (*c != '\0') {
        // Skip indentation.
        for (size_t j = 0; j < indent && *c == ' '; j++) {
            c++;
        }

        // Copy rest of line.
        while (*c != '\0') {
            result.push_back(*c);
            c++;
            if (result.back() == '\n') {
                break;
            }
        }
    }

    return result;
}

bool normalize_index_or_slice(
    const pybind11::object &index_or_slice,
    size_t length,
    pybind11::ssize_t *start,
    pybind11::ssize_t *step,
    pybind11::ssize_t *slice_length) {
    try {
        *start = pybind11::cast<pybind11::ssize_t>(index_or_slice);
        if (*start < 0) {
            *start += length;
        }
        if (*start < 0 || (size_t)*start >= length) {
            throw std::out_of_range(
                "Index " + std::to_string(pybind11::cast<pybind11::ssize_t>(index_or_slice)) +
                " not in range for sequence of length " + std::to_string(length) + ".");
        }
        return false;
    } catch (const pybind11::cast_error &) {
    }

    pybind11::slice slice;
    try {
        slice = pybind11::cast<pybind11::slice>(index_or_slice);
    } catch (const pybind11::cast_error &ex) {
        throw pybind11::type_error("Expected an integer index or a slice.");
    }

    pybind11::ssize_t stop;
    if (!slice.compute(length, start, &stop, step, slice_length)) {
        throw pybind11::error_already_set();
    }
    return true;
}

SampleFormat format_to_enum(const std::string &format) {
    auto found_format = format_name_to_enum_map.find(format);
    if (found_format == format_name_to_enum_map.end()) {
        std::stringstream msg;
        msg << "Unrecognized output format: '" << format << "'. Recognized formats are:\n";
        for (const auto &kv : format_name_to_enum_map) {
            msg << "    " << kv.first << "\n";
        }
        throw std::invalid_argument(msg.str());
    }
    return found_format->second.id;
}
