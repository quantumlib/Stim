#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <immintrin.h>
#include <vector>
#include <iostream>

enum SampleFormat {
    SAMPLE_FORMAT_ASCII,
    SAMPLE_FORMAT_BINLE8,
    SAMPLE_FORMAT_RAW_UNSTABLE,
};

struct OperationData {
    std::vector<size_t> targets;
    std::vector<bool> flags;

    OperationData(size_t target);
    OperationData(std::initializer_list<size_t> init_targets);
    OperationData(const std::vector<size_t> &init_targets);
    OperationData(const std::vector<size_t> &init_targets, std::vector<bool> init_flags);

    OperationData &operator+=(const OperationData &other);
};

struct Operation {
    std::string name;
    OperationData target_data;
    static Operation from_line(const std::string &line, size_t start, size_t end);

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

    static Circuit from_text(const std::string &text, bool fuse_operations = true);
    static Circuit from_file(FILE *file);
    Circuit with_reference_measurements_from_tableau_simulation() const;

    std::string str() const;
    bool operator==(const Circuit &other) const;
    bool operator!=(const Circuit &other) const;
};

struct CircuitReader {
private:
    FILE *input_file;
    std::string line_buf;
public:
    std::vector<Operation> operations;

    CircuitReader(FILE *file);
    bool read_next_moment(bool stop_after_measurement = false);
};

std::ostream &operator<<(std::ostream &out, const Circuit &c);
std::ostream &operator<<(std::ostream &out, const Operation &op);

#endif
