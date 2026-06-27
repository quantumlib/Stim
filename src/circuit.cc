#include "circuit.h"

#include <algorithm>
#include <string>
#include <utility>

using namespace stim;


uint64_t Circuit::count_observables() const {
    return max_operation_property([=](const CircuitInstruction &op) -> uint64_t {
        return op.gate_type == 7 ? 2 : 0;
    });
}
