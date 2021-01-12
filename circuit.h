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

    bool operator==(const Circuit &other) const;
    bool operator!=(const Circuit &other) const;
};

#endif
