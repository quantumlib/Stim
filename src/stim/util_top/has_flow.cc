#include "stim/util_top/has_flow.h"

using namespace stim;

Circuit stim::flow_test_block_for_circuit(
    const Circuit &circuit, GateTarget ancilla_qubit, const std::set<uint32_t> &obs_indices) {
    Circuit result;

    for (CircuitInstruction inst : circuit.operations) {
        if (inst.gate_type == GateType::REPEAT) {
            const Circuit &body = inst.repeat_block_body(circuit);
            Circuit new_body = flow_test_block_for_circuit(body, ancilla_qubit, obs_indices);
            result.append_repeat_block(inst.repeat_block_rep_count(), std::move(new_body), inst.tag);
        } else if (inst.gate_type == GateType::OBSERVABLE_INCLUDE && obs_indices.contains((uint32_t)inst.args[0])) {
            for (GateTarget t : inst.targets) {
                if (t.is_inverted_result_target()) {
                    result.safe_append(CircuitInstruction{GateType::X, {}, &ancilla_qubit, inst.tag});
                }
                if (t.is_measurement_record_target()) {
                    std::array<GateTarget, 2> targets{t, ancilla_qubit};
                    result.safe_append(
                        CircuitInstruction{
                            GateType::CX,
                            {},
                            targets,
                            inst.tag,
                        });
                } else if (t.is_x_target()) {
                    std::array<GateTarget, 2> targets{GateTarget::qubit(t.qubit_value()), ancilla_qubit};
                    result.safe_append(
                        CircuitInstruction{
                            GateType::XCX,
                            {},
                            targets,
                            inst.tag,
                        });
                } else if (t.is_y_target()) {
                    std::array<GateTarget, 2> targets{GateTarget::qubit(t.qubit_value()), ancilla_qubit};
                    result.safe_append(
                        CircuitInstruction{
                            GateType::YCX,
                            {},
                            targets,
                            inst.tag,
                        });
                } else if (t.is_z_target()) {
                    std::array<GateTarget, 2> targets{GateTarget::qubit(t.qubit_value()), ancilla_qubit};
                    result.safe_append(
                        CircuitInstruction{
                            GateType::CX,
                            {},
                            targets,
                            inst.tag,
                        });
                } else {
                    throw std::invalid_argument("Not handled: " + inst.str());
                }
            }
        } else {
            result.safe_append(inst);
        }
    }

    return result;
}
