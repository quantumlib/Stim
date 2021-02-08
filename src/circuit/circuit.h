/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "gate_data.h"

#define MEASURE_TARGET_MASK ((uint32_t{1} << 24) - uint32_t{1})
#define MEASURE_FLIPPED_MASK (uint32_t{1} << 31)

enum SampleFormat {
    /// Human readable format.
    ///
    /// For each shot:
    ///     For each measurement:
    ///         Output '0' if false, '1' if true
    ///     Output '\n'
    SAMPLE_FORMAT_01,
    /// Binary format.
    ///
    /// For each shot:
    ///     for each group of 8 measurement (padded with 0s if needed):
    ///         Output a bit packed byte (least significant bit of byte has first measurement)
    SAMPLE_FORMAT_B8,
    /// Transposed binary format.
    ///
    /// For each measurement:
    ///     for each group of 8 shots (padded with 0s if needed):
    ///         Output bit packed bytes (least significant bit of first byte has first shot)
    SAMPLE_FORMAT_PTB64,
    SAMPLE_FORMAT_HITS,
};

template <typename T>
struct VectorView {
    const std::vector<T> *arena;
    size_t offset;
    size_t length;

    inline size_t size() const {
        return length;
    }
    inline T operator[](size_t k) const {
        return (*arena)[offset + k];
    }

    const T *begin() const {
        return arena->data() + offset;
    }

    const T *end() const {
        return arena->data() + offset + length;
    }

    bool operator==(const VectorView<T> &other) const {
        if (length != other.length) {
            return false;
        }
        for (size_t k = 0; k < length; k++) {
            if ((*this)[k] != other[k]) {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const VectorView<T> &other) const {
        return !(*this == other);
    }
};

struct OperationData {
    /// Context-dependent numeric argument (e.g. a probability).
    double arg;
    /// Context-dependent data on what to target.
    VectorView<uint32_t> targets;

    bool operator==(const OperationData &other) const;
    bool operator!=(const OperationData &other) const;
};

struct Operation {
    const Gate *gate;
    OperationData target_data;
    bool can_fuse(const Operation &other) const;
    bool operator==(const Operation &other) const;
    bool operator!=(const Operation &other) const;
    std::string str() const;
};

struct MeasurementSet {
    std::vector<size_t> indices;
    bool expected_parity;
    MeasurementSet &operator*=(const MeasurementSet &other);
};

struct Circuit {
    std::vector<uint32_t> jagged_data;
    std::vector<Operation> operations;
    size_t num_qubits;
    size_t num_measurements;

    Circuit();
    static Circuit from_text(const std::string &text);
    static Circuit from_file(FILE *file);
    bool append_from_file(FILE *file, bool stop_asap);
    bool append_from_text(const std::string &text);

    void append_operation(const Operation &operation);
    void append_operation(const Gate &gate, const std::vector<uint32_t> &vec, double arg = 0);
    void append_operation(const std::string &gate, const std::vector<uint32_t> &vec, double arg = 0);

    void clear();
    std::string str() const;
    bool operator==(const Circuit &other) const;
    bool operator!=(const Circuit &other) const;

    std::pair<std::vector<MeasurementSet>, std::vector<MeasurementSet>> list_detectors_and_observables() const;
};

std::ostream &operator<<(std::ostream &out, const Circuit &c);
std::ostream &operator<<(std::ostream &out, const Operation &op);

#endif
