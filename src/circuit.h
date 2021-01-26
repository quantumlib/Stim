#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <iostream>

enum SampleFormat {
    SAMPLE_FORMAT_01,
    SAMPLE_FORMAT_B8,
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
    static Operation from_line(const std::string &line, size_t start, size_t end);
    bool try_fuse_with(const Operation &other);

    bool operator==(const Operation &other) const;
    bool operator!=(const Operation &other) const;
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
    Circuit with_fused_operations() const;

    std::string str() const;
    bool operator==(const Circuit &other) const;
    bool operator!=(const Circuit &other) const;
};

struct CircuitReader {
    std::vector<Operation> ops;
    void read_operation(Operation operation);
    bool read_more(FILE *file, bool stop_after_measurement = false);
};

std::ostream &operator<<(std::ostream &out, const Circuit &c);
std::ostream &operator<<(std::ostream &out, const Operation &op);

#endif
