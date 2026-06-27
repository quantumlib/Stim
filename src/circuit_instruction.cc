#include "circuit_instruction.h"
#include "circuit.h"

using namespace stim;

uint64_t CircuitInstruction::count_measurement_results() const {
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
