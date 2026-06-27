#include "stim/util_top/circuit_inverse_unitary.h"

using namespace stim;

Circuit stim::circuit_inverse_unitary(const Circuit &unitary_circuit) {
    Circuit inverted;
    unitary_circuit.for_each_operation_reverse([&](const CircuitInstruction &op) {
        const auto &gate_data = GATE_DATA[op.gate_type];
        if (!(gate_data.flags & GATE_IS_UNITARY)) {
            throw std::invalid_argument("Not unitary: " + op.str());
        }
        size_t step = (gate_data.flags & GATE_TARGETS_PAIRS) ? 2 : 1;
        auto s = op.targets.ptr_start;
        const auto &inv_gate = gate_data.inverse();
        for (size_t k = op.targets.size(); k > 0; k -= step) {
            inverted.safe_append(CircuitInstruction(inv_gate.id, op.args, {s + k - step, s + k}, op.tag));
        }
    });
    return inverted;
}
