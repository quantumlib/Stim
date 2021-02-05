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

#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

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

struct OperationData {
    /// Qubits targeted by this operation (with implicit broadcasting).
    std::vector<size_t> targets;
    /// Context-dependent accompanying data for each target (e.g. backtrack distance, measurement inversion).
    std::vector<size_t> metas;
    /// Context-dependent parens argument (e.g. noise probability).
    double arg;

    OperationData();
    OperationData(size_t target);
    OperationData(std::initializer_list<size_t> init_targets);
    OperationData(const std::vector<size_t> &init_targets);
    OperationData(const std::vector<size_t> &init_targets, std::vector<size_t> init_flags, float arg);

    OperationData &operator+=(const OperationData &other);
};

struct Operation {
    std::string name;
    OperationData target_data;

    Operation();
    Operation(std::string name, OperationData target_data);

    bool try_fuse_with(const Operation &other);

    bool operator==(const Operation &other) const;
    bool operator!=(const Operation &other) const;
    std::string str() const;
};

enum InstructionType {
    INSTRUCTION_TYPE_EMPTY,
    INSTRUCTION_TYPE_OPERATION,
    INSTRUCTION_TYPE_BLOCK_OPERATION_START,
    INSTRUCTION_TYPE_BLOCK_END,
};

struct Instruction {
    InstructionType type;
    Operation op;
    Instruction(InstructionType type, Operation op);
    static Instruction from_line(const std::string &line, size_t start, size_t end);
    bool operator==(const Operation &other) const;
    bool operator!=(const Operation &other) const;
    bool operator==(const Instruction &other) const;
    bool operator!=(const Instruction &other) const;
    std::string str() const;
};

struct MeasurementSet {
    std::vector<size_t> indices;
    bool expected_parity;
    MeasurementSet &operator*=(const MeasurementSet &other);
};

struct Circuit {
    std::vector<Operation> operations;
    size_t num_qubits;
    size_t num_measurements;
    std::vector<MeasurementSet> detectors;
    std::vector<MeasurementSet> observables;

    Circuit(std::vector<Operation> operations);

    static Circuit from_text(const std::string &text);
    static Circuit from_file(FILE *file);

    std::string str() const;
    bool operator==(const Circuit &other) const;
    bool operator!=(const Circuit &other) const;
};

struct CircuitReader {
    std::vector<Operation> ops;

    void read_all(std::string text);
    bool read_more(FILE *file, bool inside_block, bool stop_after_measurement);
    bool read_more_helper(
        const std::function<std::string(void)> &line_getter, bool inside_block, bool stop_after_measurement);
};

std::ostream &operator<<(std::ostream &out, const Circuit &c);
std::ostream &operator<<(std::ostream &out, const Operation &op);
std::ostream &operator<<(std::ostream &out, const Instruction &inst);

#endif
