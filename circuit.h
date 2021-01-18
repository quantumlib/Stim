#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <immintrin.h>
#include <vector>
#include <iostream>

struct Operation {
    std::string name;
    std::vector<size_t> targets;
    static Operation from_line(const std::string &line, size_t start, size_t end);

    bool operator==(const Operation &other) const;
    bool operator!=(const Operation &other) const;
};

struct Circuit {
    size_t num_qubits;
    std::vector<Operation> operations;

    static Circuit from_text(const std::string &text);
    static Circuit from_file(FILE *file);

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

#endif
