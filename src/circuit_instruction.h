#ifndef _STIM_CIRCUIT_INSTRUCTION_H
#define _STIM_CIRCUIT_INSTRUCTION_H

#include <cstdint>
#include <span>
namespace stim {

struct CircuitInstruction {
    uint32_t gate_type;
    std::span<const uint32_t> targets;
    uint64_t count_measurement_results() const {
        uint64_t n = (uint64_t)targets.size();
        std::cerr << "counting start ... " << n << "\n";
        for (auto e : targets) {
            if (e == 27) {
                n -= 2;
            }
        }
        std::cerr << "count final " << n << "\n";
        return n;
    }
};

}  // namespace stim

#endif
