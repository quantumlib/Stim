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

#include "../simd/monotonic_buffer.h"
#include "gate_data.h"

namespace stim_internal {

#define TARGET_VALUE_MASK ((uint32_t{1} << 24) - uint32_t{1})
#define TARGET_INVERTED_BIT (uint32_t{1} << 31)
#define TARGET_PAULI_X_BIT (uint32_t{1} << 30)
#define TARGET_PAULI_Z_BIT (uint32_t{1} << 29)
#define TARGET_RECORD_BIT (uint32_t{1} << 28)

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
    ///     For each group of 64 shots (padded with 0s if needed):
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
    /// Specific to detection event data.
    ///
    /// For each shot:
    ///     Output "shot" + " D#" for each detector that fired + " L#" for each observable that was inverted + "\n".
    SAMPLE_FORMAT_DETS,
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
    PointerRange<uint32_t> targets;

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
    /// Approximate equality.
    bool approx_equals(const Operation &other, double atol) const;
};

/// A description of a quantum computation.
struct Circuit {
    /// Backing data store for variable-sized target data referenced by operations.
    MonotonicBuffer<uint32_t> jag_targets;
    /// Operations in the circuit, from earliest to latest.
    std::vector<Operation> operations;
    std::vector<Circuit> blocks;

    size_t count_qubits() const;
    uint64_t count_measurements() const;
    uint64_t count_detectors_and_observables() const;
    size_t max_lookback() const;

    /// Constructs an empty circuit.
    Circuit();
    /// Copy constructor.
    Circuit(const Circuit &circuit);
    /// Move constructor.
    Circuit(Circuit &&circuit) noexcept;
    /// Copy assignment.
    Circuit &operator=(const Circuit &circuit);
    /// Move assignment.
    Circuit &operator=(Circuit &&circuit) noexcept;

    /// Parses a circuit from text with operations like "H 0 \n CNOT 0 1 \n M 0 1".
    ///
    /// Note: operations are automatically fused.
    static Circuit from_text(const char *text);
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
    void append_from_file(FILE *file, bool stop_asap);
    /// Grows the circuit using operations from a string.
    ///
    /// Note: operations are automatically fused.
    void append_from_text(const char *text);

    Circuit operator+(const Circuit &other) const;
    Circuit operator*(size_t repetitions) const;
    Circuit &operator+=(const Circuit &other);
    Circuit &operator*=(size_t repetitions);

    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_operation(const Operation &operation);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_op(const std::string &gate_name, const std::vector<uint32_t> &vec, double arg = 0);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_operation(const Gate &gate, ConstPointerRange<uint32_t> targets, double arg);

    /// Resets the circuit back to an empty circuit.
    void clear();

    /// Returns a text description of the circuit.
    std::string str() const;
    /// Equality.
    bool operator==(const Circuit &other) const;
    /// Inequality.
    bool operator!=(const Circuit &other) const;
    /// Approximate equality.
    bool approx_equals(const Circuit &other, double atol) const;

    template <typename CALLBACK>
    void for_each_operation(const CALLBACK &callback) const {
        for (const auto &op : operations) {
            assert(op.gate != nullptr);
            if (op.gate->id == gate_name_to_id("REPEAT")) {
                assert(op.target_data.targets.size() == 2);
                assert(op.target_data.targets[0] < blocks.size());
                size_t repeats = op.target_data.targets[1];
                const auto &block = blocks[op.target_data.targets[0]];
                for (size_t k = 0; k < repeats; k++) {
                    block.for_each_operation(callback);
                }
            } else {
                callback(op);
            }
        }
    }

    template <typename CALLBACK>
    void for_each_operation_reverse(const CALLBACK &callback) const {
        for (size_t p = operations.size(); p-- > 0;) {
            const auto &op = operations[p];
            assert(op.gate != nullptr);
            if (op.gate->id == gate_name_to_id("REPEAT")) {
                assert(op.target_data.targets.size() == 2);
                assert(op.target_data.targets[0] < blocks.size());
                size_t repeats = op.target_data.targets[1];
                const auto &block = blocks[op.target_data.targets[0]];
                for (size_t k = 0; k < repeats; k++) {
                    block.for_each_operation_reverse(callback);
                }
            } else {
                callback(op);
            }
        }
    }
};

/// Lists sets of measurements that have deterministic parity under noiseless execution from a circuit.
struct DetectorsAndObservables {
    MonotonicBuffer<uint32_t> jagged_detector_data;
    std::vector<PointerRange<uint32_t>> detectors;
    std::vector<std::vector<uint32_t>> observables;
    DetectorsAndObservables(const Circuit &circuit);

    DetectorsAndObservables(DetectorsAndObservables &&other) noexcept;
    DetectorsAndObservables &operator=(DetectorsAndObservables &&other) noexcept;
    DetectorsAndObservables(const DetectorsAndObservables &other);
    DetectorsAndObservables &operator=(const DetectorsAndObservables &other);
};

}

std::ostream &operator<<(std::ostream &out, const stim_internal::Circuit &c);
std::ostream &operator<<(std::ostream &out, const stim_internal::Operation &op);


#endif
