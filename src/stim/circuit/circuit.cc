#include "stim/circuit/circuit.h"

#include <algorithm>
#include <string>
#include <utility>

#include "stim/circuit/gate_target.h"
#include "stim/gates/gates.h"

using namespace stim;

uint64_t stim::add_saturate(uint64_t a, uint64_t b) {
    uint64_t r = a + b;
    if (r < a) {
        return UINT64_MAX;
    }
    return r;
}

uint64_t stim::mul_saturate(uint64_t a, uint64_t b) {
    if (b && a > UINT64_MAX / b) {
        return UINT64_MAX;
    }
    return a * b;
}

uint64_t Circuit::count_measurements() const {
    return flat_count_operations([=](const CircuitInstruction &op) -> uint64_t {
        return op.count_measurement_results();
    });
}

uint64_t Circuit::count_detectors() const {
    return flat_count_operations([=](const CircuitInstruction &op) -> uint64_t {
        return op.gate_type == GateType::DETECTOR;
    });
}

uint64_t Circuit::count_ticks() const {
    return flat_count_operations([=](const CircuitInstruction &op) -> uint64_t {
        return op.gate_type == GateType::TICK;
    });
}

uint64_t Circuit::count_observables() const {
    return max_operation_property([=](const CircuitInstruction &op) -> uint64_t {
        return op.gate_type == GateType::OBSERVABLE_INCLUDE ? 2 : 0;
    });
}
