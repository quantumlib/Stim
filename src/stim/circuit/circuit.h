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

    // Returns one more than the largest `k` from any qubit target `k` or `!k` or `{X,Y,Z}k`.
    size_t count_qubits() const;
    // Returns the number of measurement bits produced by the circuit.
    uint64_t count_measurements() const;
    // Returns the number of detection event bits produced by the circuit.
    uint64_t count_detectors() const;
    // Returns one more than the largest `k` from any `OBSERVABLE_INCLUDE(k)` instruction.
    uint64_t count_observables() const;
    // Returns the number of ticks performed when running the circuit.
    uint64_t count_ticks() const;
    // Returns the largest `k` from any `rec[-k]` target.
    size_t max_lookback() const;
    // Returns one more than the largest `k` from any `sweep[k]` target.
    size_t count_sweep_bits() const;

    /// Constructs an empty circuit.
    Circuit();
    /// Copy constructor.
    Circuit(const Circuit &circuit);
    /// Move constructor.
    Circuit(Circuit &&circuit) noexcept;
    /// Copy assignment.
    Circuit &operator=(const Circuit &circuit);
    /// Move assignment.
    Circuit &operator=(Circuit &&circuit) noexcept;

    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void safe_append(CircuitInstruction operation, bool block_fusion = false);
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void safe_append_ua(
        std::string_view gate_name,
        const std::vector<uint32_t> &targets,
        double singleton_arg,
        std::string_view tag = "");
    /// Safely adds an operation at the end of the circuit, copying its data into the circuit's jagged data as needed.
    void safe_append_u(
        std::string_view gate_name,
        const std::vector<uint32_t> &targets,
        const std::vector<double> &args = {},
        std::string_view tag = "");

    /// Resets the circuit back to an empty circuit.
    void clear();

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

    /// Helper method for finding the largest observable, etc.
    template <typename MAP>
    uint64_t max_operation_property(const MAP &map) const {
        uint64_t n = 0;
        for (const auto &block : blocks) {
            n = std::max(n, block.max_operation_property<MAP>(map));
        }
        for (const auto &op : operations) {
            if (op.gate_type == GateType::REPEAT) {
                // Handled in block case.
                continue;
            }
            n = std::max(n, (uint64_t)map(op));
        }
        return n;
    }
};

void vec_pad_add_mul(std::vector<double> &target, SpanRef<const double> offset, uint64_t mul = 1);

}  // namespace stim

#endif
