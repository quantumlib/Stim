#include "circuit_instruction.h"
#include "circuit.h"

using namespace stim;

uint64_t CircuitInstruction::repeat_block_rep_count() const {
    assert(targets.size() == 3);
    uint64_t low = targets[1];
    uint64_t high = targets[2];
    return low | (high << 32);
}

Circuit &CircuitInstruction::repeat_block_body(Circuit &host) const {
    assert(targets.size() == 3);
    auto b = targets[0];
    assert(b < host.blocks.size());
    return host.blocks[b];
}

const Circuit &CircuitInstruction::repeat_block_body(const Circuit &host) const {
    assert(targets.size() == 3);
    auto b = targets[0];
    assert(b < host.blocks.size());
    return host.blocks[b];
}

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
