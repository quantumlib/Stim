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

#define TARGET_QUBIT_MASK ((uint32_t{1} << 24) - uint32_t{1})
#define TARGET_RECORD_MASK (~TARGET_QUBIT_MASK)
#define TARGET_RECORD_SHIFT (24)
#define TARGET_INVERTED_MASK (uint32_t{1} << 31)
#define TARGET_PAULI_X_MASK (uint32_t{1} << 30)
#define TARGET_PAULI_Z_MASK (uint32_t{1} << 29)

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
    ///     For each group of 8 measurement (padded with 0s if needed):
    ///         Output a bit packed byte (least significant bit of byte has first measurement)
    SAMPLE_FORMAT_B8,
    /// Transposed binary format.
    ///
    /// For each measurement:
    ///     For each group of 8 shots (padded with 0s if needed):
    ///         Output bit packed bytes (least significant bit of first byte has first shot)
    SAMPLE_FORMAT_PTB64,
    /// Human readable compressed format.
    ///
    /// For each shot:
    ///     For each measurement_index where the measurement result was 1:
    ///         Output decimal(measurement_index)
    SAMPLE_FORMAT_HITS,
    /// Binary run-length format.
    ///
    /// For each shot:
    ///     For each run of same-result measurements up to length 128:
    ///         Output (result ? 0x80 : 0) | (run_length + 1)
    SAMPLE_FORMAT_R8,
};

/// A pointer to contiguous data inside a std::vector.
///
/// Used by Circuit and OperationData to store jagged data with less memory fragmentation and fewer allocations.
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

/// The data that describes how a gate is being applied to qubits (or other targets).
///
/// This struct is not self-sufficient. It points into data stored elsewhere (e.g. in a Circuit's jagged_data).
struct OperationData {
    /// Context-dependent numeric argument (e.g. a probability).
    double arg;
    /// Context-dependent data on what to target.
    ///
    /// The bottom 24 bits of each item always refer to a qubit index.
    /// The top 8 bits are used for additional data such as
    /// Pauli basis, record lookback, and measurement inversion.
    VectorView<uint32_t> targets;

    bool operator==(const OperationData &other) const;
    bool operator!=(const OperationData &other) const;
};

/// A gate applied to targets.
///
/// This struct is not self-sufficient. It points into data stored elsewhere (e.g. in a Circuit's jagged_data).
struct Operation {
    /// The gate applied by the operation. Should never be nullptr.
    const Gate *gate;
    /// The targeting data for applying the gate.
    OperationData target_data;
    /// Determines if two operations can be combined into one operation (with combined targeting data).
    bool can_fuse(const Operation &other) const;
    /// Equality.
    bool operator==(const Operation &other) const;
    /// Inequality.
    bool operator!=(const Operation &other) const;
    /// Returns a text description of the operation's gate and targets.
    std::string str() const;
};

/// A set of measurements that have deterministic parity under noiseless execution.
struct MeasurementSet {
    /// The indices of the measurements that are part of the set (e.g. index 0 means the first measurement).
    std::vector<size_t> indices;
    /// Folds the given measurement set into this one, forming a combined set.
    MeasurementSet &operator*=(const MeasurementSet &other);
};

/// A description of a quantum computation.
struct Circuit {
    /// Variable-sized operation data is stored as views into this single contiguous array.
    /// Appending operations will append their target data into this vector, and the operation will reference it.
    /// This decreases memory fragmentation and the number of allocations during parsing.
    std::vector<uint32_t> jagged_data;
    /// Operations in the circuit, from earliest to latest.
    std::vector<Operation> operations;
    /// One more than the maximum qubit index seen in the circuit (so far).
    size_t num_qubits;
    /// The total number of measurement results the circuit (so far) will produce.
    size_t num_measurements;

    /// Constructs an empty circuit.
    Circuit();
    /// Copy constructor.
    Circuit(const Circuit &circuit);
    /// Move constructor.
    Circuit(Circuit &&circuit) noexcept;
    /// Copy assignment.
    Circuit &operator=(Circuit &&circuit) noexcept;
    /// Move assignment.
    Circuit &operator=(const Circuit &circuit);

    /// Parses a circuit from text with operations like "H 0 \n CNOT 0 1 \n M 0 1".
    ///
    /// Note: operations are automatically fused.
    static Circuit from_text(const std::string &text);
    /// Parses a circuit from a file containing operations.
    ///
    /// Note: operations are automatically fused.
    static Circuit from_file(FILE *file);
    /// Grows the circuit using operations from a file.
    ///
    /// Note: operations are automatically fused.
    ///
    /// Args:
    ///     file: The opened file to read from.
    ///     stop_asap: When set to true, the reading process stops after the next operation is read. This is used for
    ///         interactive (repl) mode, where measurements should produce results immediately instead of only after the
    ///         circuit is entirely specified. *This has significantly worse performance. It prevents measurement
    ///         batching.*
    ///
    /// Returns:
    ///     true: Operations were read from the file.
    ///     false: The file has ended, and no operations were read.
    bool append_from_file(FILE *file, bool stop_asap);
    /// Grows the circuit using operations from a string.
    ///
    /// Note: operations are automatically fused.
    ///
    /// Returns:
    ///     true: Operations were read from the string.
    ///     false: The string contained no operations.
    bool append_from_text(const std::string &text);

    Circuit operator+(const Circuit &other) const;
    Circuit operator*(size_t repetitions) const;
    Circuit &operator+=(const Circuit &other);
    Circuit &operator*=(size_t repetitions);

    /// Appends a circuit to the end of this one.
    void append_circuit(const Circuit &circuit, size_t repetitions);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_operation(const Operation &operation);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_op(
        const std::string &gate_name, const std::vector<uint32_t> &vec, double arg = 0, bool allow_fusing = false);

    /// Resets the circuit back to an empty circuit.
    void clear();

    /// Returns a text description of the circuit.
    std::string str() const;
    /// Equality.
    bool operator==(const Circuit &other) const;
    /// Inequality.
    bool operator!=(const Circuit &other) const;

    /// Converts the relative detector and observable annotations in the circuit into absolute measurement sets.
    std::pair<std::vector<MeasurementSet>, std::vector<MeasurementSet>> list_detectors_and_observables() const;
};

std::ostream &operator<<(std::ostream &out, const Circuit &c);
std::ostream &operator<<(std::ostream &out, const Operation &op);

#endif
