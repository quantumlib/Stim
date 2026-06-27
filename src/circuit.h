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
#include <cassert>

#include "circuit_instruction.h"

namespace stim {

inline uint64_t add_saturate(uint64_t a, uint64_t b) {
    uint64_t r = a + b;
    if (r < a) {
        return UINT64_MAX;
    }
    return r;
}

inline uint64_t mul_saturate(uint64_t a, uint64_t b) {
    if (b && a > UINT64_MAX / b) {
        return UINT64_MAX;
    }
    return a * b;
}

struct Circuit {
    std::vector<CircuitInstruction> operations;

    uint64_t count_observables() const;

    template <typename MAP>
    uint64_t max_operation_property(const MAP &map) const {
        uint64_t n = 0;
        for (const auto &op : operations) {
            if (op.gate_type == 8) {
                // Handled in block case.
                continue;
            }
            n = std::max(n, (uint64_t)map(op));
        }
        return n;
    }
};

}  // namespace stim

#endif
