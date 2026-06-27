#ifndef _STIM_CIRCUIT_INSTRUCTION_H
#define _STIM_CIRCUIT_INSTRUCTION_H

#include <cstdint>
#include <span>
namespace stim {

struct CircuitInstruction {
    uint32_t gate_type;
    std::span<const uint32_t> targets;
    uint64_t count_measurement_results() const;
};

}  // namespace stim

#endif
