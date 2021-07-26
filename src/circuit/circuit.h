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

#ifndef STIM_CIRCUIT_H
#define STIM_CIRCUIT_H

#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../simd/monotonic_buffer.h"
#include "../simd/pointer_range.h"
#include "gate_data.h"
#include "gate_target.h"

namespace stim_internal {

uint64_t op_data_rep_count(const OperationData &data);

uint64_t add_saturate(uint64_t a, uint64_t b);
uint64_t mul_saturate(uint64_t a, uint64_t b);

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
    ///     Append a one to the shot
    ///     For each run length d of zeros between ones (including runs of length 0):
    ///         Output [0x255] * (d // 255) + [d % 255]
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
    /// Context-dependent numeric arguments (e.g. probabilities).
    PointerRange<double> args;
    /// Context-dependent data on what to target.
    ///
    /// The bottom 24 bits of each item always refer to a qubit index.
    /// The top 8 bits are used for additional data such as
    /// Pauli basis, record lookback, and measurement inversion.
    PointerRange<GateTarget> targets;

    bool operator==(const OperationData &other) const;
    bool operator!=(const OperationData &other) const;

    std::string str() const;
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

    /// Determines the number of entries added to the measurement record by the operation.
    ///
    /// Note: invalid to use this on REPEAT blocks.
    uint64_t count_measurement_results() const;
};

/// A description of a quantum computation.
struct Circuit {
    /// Backing data stores for variable-sized target data referenced by operations.
    MonotonicBuffer<GateTarget> target_buf;
    MonotonicBuffer<double> arg_buf;
    /// Operations in the circuit, from earliest to latest.
    std::vector<Operation> operations;
    std::vector<Circuit> blocks;

    size_t count_qubits() const;
    uint64_t count_measurements() const;
    uint64_t count_detectors() const;
    uint64_t num_observables() const;
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

    /// Parse constructor. Creates a circuit from text with operations like "H 0 \n CNOT 0 1 \n M 0 1".
    ///
    /// Note: operations are automatically fused.
    Circuit(const char *text);
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
    void append_from_file(FILE *file, bool stop_asap = false);
    /// Grows the circuit using operations from a string.
    ///
    /// Note: operations are automatically fused.
    void append_from_text(const char *text);

    Circuit operator+(const Circuit &other) const;
    Circuit operator*(uint64_t repetitions) const;
    Circuit &operator+=(const Circuit &other);
    Circuit &operator*=(uint64_t repetitions);

    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_operation(const Operation &operation);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_op(const std::string &gate_name, const std::vector<uint32_t> &targets, double singleton_arg);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_op(
        const std::string &gate_name, const std::vector<uint32_t> &targets, const std::vector<double> &args = {});
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void append_operation(const Gate &gate, ConstPointerRange<GateTarget> targets, ConstPointerRange<double> args);

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

    /// Gets a python-style slice of the circuit's instructions.
    Circuit py_get_slice(int64_t start, int64_t step, int64_t slice_length) const;

    template <typename CALLBACK>
    void for_each_operation(const CALLBACK &callback) const {
        for (const auto &op : operations) {
            assert(op.gate != nullptr);
            if (op.gate->id == gate_name_to_id("REPEAT")) {
                assert(op.target_data.targets.size() == 3);
                auto b = op.target_data.targets[0].data;
                assert(b < blocks.size());
                uint64_t repeats = op_data_rep_count(op.target_data);
                const auto &block = blocks[b];
                for (uint64_t k = 0; k < repeats; k++) {
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
                assert(op.target_data.targets.size() == 3);
                auto b = op.target_data.targets[0].data;
                assert(b < blocks.size());
                uint64_t repeats = op_data_rep_count(op.target_data);
                const auto &block = blocks[b];
                for (uint64_t k = 0; k < repeats; k++) {
                    block.for_each_operation_reverse(callback);
                }
            } else {
                callback(op);
            }
        }
    }

    template <typename COUNT>
    uint64_t flat_count_operations(const COUNT &count) const {
        uint64_t n = 0;
        for (const auto &op : operations) {
            assert(op.gate != nullptr);
            if (op.gate->id == gate_name_to_id("REPEAT")) {
                assert(op.target_data.targets.size() == 3);
                auto b = op.target_data.targets[0].data;
                assert(b < blocks.size());
                auto sub = blocks[b].flat_count_operations<COUNT>(count);
                n = add_saturate(n, mul_saturate(sub, op_data_rep_count(op.target_data)));
            } else {
                n = add_saturate(n, count(op));
            }
        }
        return n;
    }

    template <typename MAP>
    uint64_t max_operation_property(const MAP &map) const {
        uint64_t n = 0;
        for (const auto &block : blocks) {
            n = std::max(n, block.max_operation_property<MAP>(map));
        }
        for (const auto &op : operations) {
            if (op.gate->flags & GATE_IS_BLOCK) {
                // Handled in block case.
                continue;
            }
            n = std::max(n, (uint64_t)map(op));
        }
        return n;
    }
};

/// Lists sets of measurements that have deterministic parity under noiseless execution from a circuit.
struct DetectorsAndObservables {
    MonotonicBuffer<uint64_t> jagged_detector_data;
    std::vector<PointerRange<uint64_t>> detectors;
    std::vector<std::vector<uint64_t>> observables;
    DetectorsAndObservables(const Circuit &circuit);

    DetectorsAndObservables(DetectorsAndObservables &&other) noexcept;
    DetectorsAndObservables &operator=(DetectorsAndObservables &&other) noexcept;
    DetectorsAndObservables(const DetectorsAndObservables &other);
    DetectorsAndObservables &operator=(const DetectorsAndObservables &other);
};

Circuit &op_data_block_body(Circuit &host, const OperationData &data);
const Circuit &op_data_block_body(const Circuit &host, const OperationData &data);

template <typename SOURCE>
inline void read_past_within_line_whitespace(int &c, SOURCE read_char) {
    while (c == ' ' || c == '\t') {
        c = read_char();
    }
}

template <typename SOURCE>
bool read_until_next_line_arg(int &c, SOURCE read_char, bool space_required = true) {
    if (c == '*') {
        return true;
    }
    if (space_required) {
        if (c != ' ' && c != '#' && c != '\t' && c != '\n' && c != '{' && c != EOF) {
            throw std::invalid_argument("Targets must be separated by spacing.");
        }
    }
    while (c == ' ' || c == '\t') {
        c = read_char();
    }
    if (c == '#') {
        do {
            c = read_char();
        } while (c != '\n' && c != EOF);
    }
    return c != '\n' && c != '{' && c != EOF;
}

template <typename SOURCE>
void read_past_dead_space_between_commands(int &c, SOURCE read_char) {
    while (true) {
        while (isspace(c)) {
            c = read_char();
        }
        if (c == EOF) {
            break;
        }
        if (c != '#') {
            break;
        }
        while (c != '\n' && c != EOF) {
            c = read_char();
        }
    }
}

inline bool is_double_char(int c) {
    return (c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-';
}

template <typename SOURCE>
double read_normal_double(int &c, SOURCE read_char) {
    char buf[64];
    size_t n = 0;
    while (n < sizeof(buf) - 1 && is_double_char(c)) {
        buf[n] = (char)c;
        c = read_char();
        n++;
    }
    buf[n] = '\0';

    char *end;
    double result = strtod(buf, &end);
    if (end != buf + n || std::isinf(result) || std::isnan(result)) {
        throw std::invalid_argument("Not a real number: " + std::string(buf));
    }
    return result;
}

template <typename SOURCE>
void read_parens_arguments(int &c, const char *name, SOURCE read_char, MonotonicBuffer<double> &out) {
    if (c != '(') {
        return;
    }
    c = read_char();

    read_past_within_line_whitespace(c, read_char);
    while (true) {
        out.append_tail(read_normal_double(c, read_char));
        read_past_within_line_whitespace(c, read_char);
        if (c != ',') {
            break;
        }
        c = read_char();
        read_past_within_line_whitespace(c, read_char);
    }

    read_past_within_line_whitespace(c, read_char);
    if (c != ')') {
        throw std::invalid_argument("Parens arguments for '" + std::string(name) + "' didn't end with a ')'.");
    }
    c = read_char();
}

std::ostream &operator<<(std::ostream &out, const Circuit &c);
std::ostream &operator<<(std::ostream &out, const Operation &op);
std::ostream &operator<<(std::ostream &out, const OperationData &dat);
void print_circuit(std::ostream &out, const Circuit &c, const std::string &indentation);

}  // namespace stim_internal

#endif
