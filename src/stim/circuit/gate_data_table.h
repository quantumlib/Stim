#ifndef _STIM_CIRCUIT_GATE_DATA_TABLE_H
#define _STIM_CIRCUIT_GATE_DATA_TABLE_H

#include <array>

#include "gate_data.h"

namespace stim {

/// A class that maps each gate to a value.
///
/// The only real difference between this class and a raw array is that this class makes it
/// harder to forget to define a value for a gate, and supports key-value style
/// initialization at compile time.
///
/// The value for gate G is stored at: vtable.data[G.id]
template <typename GATE_VALUE_TYPE>
struct GateVTable {
    std::array<GATE_VALUE_TYPE, 256> data;

    // Must construct explicitly, but can copy and move.
    GateVTable() = delete;
    GateVTable(const GateVTable &) = default;
    GateVTable(GateVTable &&) = default;
    GateVTable &operator=(const GateVTable &) = default;
    GateVTable &operator=(GateVTable &&) = default;

    // Construct from gate-value pairs.
    constexpr GateVTable(const std::array<std::pair<GateType, GATE_VALUE_TYPE>, NUM_DEFINED_GATES> &gate_data_pairs)
        : data({}) {
        for (const auto &[gate_id, value] : gate_data_pairs) {
            data[gate_id] = value;
        }
#ifndef NDEBUG
        std::array<bool, NUM_DEFINED_GATES> seen{};
        for (const auto &[gate_id, value] : gate_data_pairs) {
            seen[(size_t)gate_id] = true;
        }
        for (const auto &gate : GATE_DATA.items) {
            if (!seen[(size_t)gate.id]) {
                throw std::invalid_argument(
                    "Missing gate data! A value was not defined for '" + std::string(gate.name) + "'.");
            }
        }
#endif
    }
};

}  // namespace stim

#endif
