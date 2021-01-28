#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <functional>
#include <iostream>
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
};

struct OperationData {
    std::vector<size_t> targets;
    std::vector<bool> flags;
    double arg;

    OperationData(size_t target);
    OperationData(std::initializer_list<size_t> init_targets);
    OperationData(const std::vector<size_t> &init_targets);
    OperationData(const std::vector<size_t> &init_targets, std::vector<bool> init_flags, float arg);

    OperationData &operator+=(const OperationData &other);
};

struct Operation {
    std::string name;
    OperationData target_data;
    bool try_fuse_with(const Operation &other);

    bool operator==(const Operation &other) const;
    bool operator!=(const Operation &other) const;
    std::string str() const;
};

struct Instruction {
    Operation operation;
    bool started_block;
    bool ended_block;
    static Instruction from_line(const std::string &line, size_t start, size_t end);
    bool operator==(const Operation &other) const;
    bool operator!=(const Operation &other) const;
    bool operator==(const Instruction &other) const;
    bool operator!=(const Instruction &other) const;
    std::string str() const;
};

struct Circuit {
    std::vector<Operation> operations;
    size_t num_qubits;
    size_t num_measurements;

    Circuit(const std::vector<Operation> &operations);
    Circuit(std::vector<Operation> &&operations);

    static Circuit from_text(const std::string &text);
    static Circuit from_file(FILE *file);

    std::string str() const;
    bool operator==(const Circuit &other) const;
    bool operator!=(const Circuit &other) const;
};

struct CircuitReader {
    std::vector<Operation> ops;
    bool read_more(std::string text, bool inside_block, bool stop_after_measurement);
    bool read_more(FILE *file, bool inside_block, bool stop_after_measurement);
    bool read_more_helper(
        const std::function<std::string(void)> &line_getter, bool inside_block, bool stop_after_measurement);
};

std::ostream &operator<<(std::ostream &out, const Circuit &c);
std::ostream &operator<<(std::ostream &out, const Operation &op);
std::ostream &operator<<(std::ostream &out, const Instruction &inst);

#endif
