#ifndef CIRCUIT_TEST_H
#define CIRCUIT_TEST_H

#include "circuit.h"

// Helper class for creating temporary operation data.
struct OpDat {
    std::vector<uint32_t> targets;
    OpDat(uint32_t target);
    OpDat(std::vector<uint32_t> targets);
    static OpDat flipped(size_t target);
    operator OperationData();
};

#endif
