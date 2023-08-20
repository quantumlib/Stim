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

#ifndef _STIM_CIRCUIT_CIRCUIT_H
#define _STIM_CIRCUIT_CIRCUIT_H

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "stim/circuit/circuit_instruction.h"
#include "stim/circuit/gate_data.h"
#include "stim/circuit/gate_target.h"
#include "stim/mem/monotonic_buffer.h"
#include "stim/mem/span_ref.h"

namespace stim {

uint64_t add_saturate(uint64_t a, uint64_t b);
uint64_t mul_saturate(uint64_t a, uint64_t b);

/// A description of a quantum computation.
struct Circuit {
    /// Backing data stores for variable-sized target data referenced by operations.
    MonotonicBuffer<GateTarget> target_buf;
    MonotonicBuffer<double> arg_buf;
    /// Operations in the circuit, from earliest to latest.
    std::vector<CircuitInstruction> operations;
    std::vector<Circuit> blocks;

    // Returns one more than the largest `k` from any qubit target `k` or `!k` or `{X,Y,Z}k`.
    size_t count_qubits() const;
    // Returns the number of measurement bits produced by the circuit.
    uint64_t count_measurements() const;
    // Returns the number of detection event bits produced by the circuit.
    uint64_t count_detectors() const;
    // Returns one more than the largest `k` from any `OBSERVABLE_INCLUDE(k)` instruction.
    uint64_t count_observables() const;
    // Returns the number of ticks performed when running the circuit.
    uint64_t count_ticks() const;
    // Returns the largest `k` from any `rec[-k]` target.
    size_t max_lookback() const;
    // Returns one more than the largest `k` from any `sweep[k]` target.
    size_t count_sweep_bits() const;

    CircuitStats compute_stats() const;

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
    void safe_append(const CircuitInstruction &operation);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void safe_append_ua(const std::string &gate_name, const std::vector<uint32_t> &targets, double singleton_arg);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void safe_append_u(
        const std::string &gate_name, const std::vector<uint32_t> &targets, const std::vector<double> &args = {});
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void safe_append(GateType gate_type, SpanRef<const GateTarget> targets, SpanRef<const double> args);
    /// Safely copies a repeat block to the end of the circuit.
    void append_repeat_block(uint64_t repeat_count, const Circuit &body);
    /// Safely moves a repeat block to the end of the circuit.
    void append_repeat_block(uint64_t repeat_count, Circuit &&body);

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

    /// Returns a noiseless version of the given circuit. The result must live for less time than the given circuit.
    ///
    /// CAUTION: for performance, the returned circuit contains pointers into the given circuit!
    /// The result's lifetime must be shorter than the given circuit's lifetime!
    const Circuit aliased_noiseless_circuit() const;

    /// Returns a copy of the circuit with all noise processes removed.
    Circuit without_noise() const;

    /// Returns an equivalent circuit without REPEAT or SHIFT_COORDS instructions.
    Circuit flattened() const;

    /// Returns a circuit that implements the inverse Clifford operation.
    ///
    /// Args:
    ///     allow_weak_inverse: When this is set to true, the inverse of the circuit doesn't need
    ///         to be exact. In particular, noise and measurement becomes self-inverse. Examples:
    ///             - The weak inverse of MX is MX.
    ///             - The weak inverse of MRX is MRX.
    ///             - The weak inverse of RX is MRX.
    ///             - The weak inverse of X_ERROR(0.1) is X_ERROR(0.1).
    ///             - The weak inverse of DETECTOR is [discard the operation].
    ///             - The weak inverse of OBSERVABLE_INCLUDE is [discard the operation].
    ///
    /// Returns:
    ///     The inverted circuit.
    Circuit inverse(bool allow_weak_inverse = false) const;

    /// Helper method for executing the circuit, e.g. repeating REPEAT blocks.
    template <typename CALLBACK>
    void for_each_operation(const CALLBACK &callback) const {
        for (const auto &op : operations) {
            if (op.gate_type == GateType::REPEAT) {
                uint64_t repeats = op.repeat_block_rep_count();
                const auto &block = op.repeat_block_body(*this);
                for (uint64_t k = 0; k < repeats; k++) {
                    block.for_each_operation(callback);
                }
            } else {
                callback(op);
            }
        }
    }

    /// Helper method for reverse-executing the circuit, e.g. repeating REPEAT blocks.
    template <typename CALLBACK>
    void for_each_operation_reverse(const CALLBACK &callback) const {
        for (size_t p = operations.size(); p-- > 0;) {
            const auto &op = operations[p];
            if (op.gate_type == GateType::REPEAT) {
                uint64_t repeats = op.repeat_block_rep_count();
                const auto &block = op.repeat_block_body(*this);
                for (uint64_t k = 0; k < repeats; k++) {
                    block.for_each_operation_reverse(callback);
                }
            } else {
                callback(op);
            }
        }
    }

    /// Helper method for counting measurements, detectors, etc.
    template <typename COUNT>
    uint64_t flat_count_operations(const COUNT &count) const {
        uint64_t n = 0;
        for (const auto &op : operations) {
            if (op.gate_type == GateType::REPEAT) {
                assert(op.targets.size() == 3);
                auto b = op.targets[0].data;
                assert(b < blocks.size());
                auto sub = blocks[b].flat_count_operations<COUNT>(count);
                n = add_saturate(n, mul_saturate(sub, op.repeat_block_rep_count()));
            } else {
                n = add_saturate(n, count(op));
            }
        }
        return n;
    }

    /// Helper method for finding the largest observable, etc.
    template <typename MAP>
    uint64_t max_operation_property(const MAP &map) const {
        uint64_t n = 0;
        for (const auto &block : blocks) {
            n = std::max(n, block.max_operation_property<MAP>(map));
        }
        for (const auto &op : operations) {
            if (op.gate_type == GateType::REPEAT) {
                // Handled in block case.
                continue;
            }
            n = std::max(n, (uint64_t)map(op));
        }
        return n;
    }

    /// Looks for QUBIT_COORDS instructions in the circuit, and returns a map from qubit index to qubit coordinate.
    ///
    /// This method efficiently handles REPEAT blocks. It finishes in time proportional to the size of the circuit
    /// file, not time proportional to the amount of data produced by the circuit.
    std::map<uint64_t, std::vector<double>> get_final_qubit_coords() const;

    /// Looks up the coordinate data of a detector.
    ///
    /// Args:
    ///     detector_index: The index of the detector to get coordinate data for.
    ///         Detectors are indexed by the order they appear in the circuit (accounting for repeat blocks).
    ///
    /// Returns:
    ///     The coordinate data for the detector.
    ///     If the detector has no coordinate data, an empty vector is returned.
    ///
    /// Throws:
    ///     std::invalid_argument: The detector index is greater than or equal to circuit.count_detectors().
    std::vector<double> coords_of_detector(uint64_t detector_index) const;

    /// Looks up the coordinate data of a given set of detectors.
    ///
    /// Args:
    ///     included_detector_indices: An ordered set of the indices of detectors to get coordinate data for.
    ///
    /// Returns:
    ///     A map from detector index to coordinate data.
    ///     Every index from included_detector_indices will be in this map as a key.
    ///     Detectors with no coordinate data are mapped to an empty vector.
    ///
    /// Throws:
    ///     std::invalid_argument: A detector index is greater than or equal to circuit.count_detectors().
    std::map<uint64_t, std::vector<double>> get_detector_coordinates(
        const std::set<uint64_t> &included_detector_indices) const;

    /// Returns the total coordinate shift accumulated over the entire circuit, accounting for REPEAT blocks.
    std::vector<double> final_coord_shift() const;

    /// Helper method for building up human readable descriptions of circuit locations.
    std::string describe_instruction_location(size_t instruction_offset) const;

    void try_fuse_last_two_ops();
};

void vec_pad_add_mul(std::vector<double> &target, SpanRef<const double> offset, uint64_t mul = 1);

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
        if (c != ' ' && c != '#' && c != '\t' && c != '\n' && c != '\r' && c != '{' && c != EOF) {
            throw std::invalid_argument("Targets must be separated by spacing.");
        }
    }
    while (c == ' ' || c == '\t' || c == '\r') {
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
std::ostream &operator<<(std::ostream &out, const CircuitInstruction &op);
void print_circuit(std::ostream &out, const Circuit &c, const std::string &indentation);

}  // namespace stim

#endif
