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
#include "stim/circuit/gate_target.h"
#include "stim/gates/gates.h"
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
    MonotonicBuffer<char> tag_buf;
    /// Operations in the circuit, from earliest to latest.
    std::vector<CircuitInstruction> operations;
    std::vector<Circuit> blocks;

    size_t count_qubits() const;
    uint64_t count_measurements() const;
    uint64_t count_detectors() const;
    uint64_t count_observables() const;
    uint64_t count_ticks() const;
    size_t max_lookback() const;
    size_t count_sweep_bits() const;

    Circuit();

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
};

}  // namespace stim

#endif
