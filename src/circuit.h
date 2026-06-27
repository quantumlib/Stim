#ifndef _STIM_CIRCUIT_CIRCUIT_H
#define _STIM_CIRCUIT_CIRCUIT_H


#include <vector>
#include "circuit_instruction.h"

namespace stim {

struct Circuit {
    std::vector<CircuitInstruction> operations;

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
