#include "stim/simulators/tableau_simulator.h"
#include "stim/util_top/count_determined_measurements.h"

namespace stim {

template <size_t W>
uint64_t count_determined_measurements(const Circuit &circuit) {
    uint64_t result = 0;
    auto n = circuit.count_qubits();
    TableauSimulator<W> sim(std::mt19937_64{0}, n);
    PauliString<W> obs_buffer(n);

    circuit.for_each_operation([&](const CircuitInstruction &inst) {
        if (!(GATE_DATA[inst.gate_type].flags & GATE_PRODUCES_RESULTS)) {
            sim.do_gate(inst);
            return;
        }
        switch (inst.gate_type) {
            case GateType::M:
                [[fallthrough]];
            case GateType::MR: {
                for (const auto &t : inst.targets) {
                    assert(t.is_qubit_target());
                    result += sim.peek_z(t.qubit_value()) != 0;
                    sim.do_gate(CircuitInstruction{inst.gate_type, {}, {&t}, ""});
                }
                break;
            }

            case GateType::MX:
                [[fallthrough]];
            case GateType::MRX: {
                for (const auto &t : inst.targets) {
                    assert(t.is_qubit_target());
                    result += sim.peek_x(t.qubit_value()) != 0;
                    sim.do_gate(CircuitInstruction{inst.gate_type, {}, {&t}, ""});
                }
                break;
            }

            case GateType::MY:
                [[fallthrough]];
            case GateType::MRY: {
                for (const auto &t : inst.targets) {
                    assert(t.is_qubit_target());
                    result += sim.peek_y(t.qubit_value()) != 0;
                    sim.do_gate(CircuitInstruction{inst.gate_type, {}, {&t}, ""});
                }
                break;
            }

            case GateType::MXX:
                [[fallthrough]];
            case GateType::MYY:
                [[fallthrough]];
            case GateType::MZZ: {
                bool x = inst.gate_type != GateType::MZZ;
                bool z = inst.gate_type != GateType::MXX;
                for (size_t k = 0; k < inst.targets.size(); k += 2) {
                    auto q0 = inst.targets[k].qubit_value();
                    auto q1 = inst.targets[k + 1].qubit_value();
                    obs_buffer.xs[q0] = x;
                    obs_buffer.xs[q1] = x;
                    obs_buffer.zs[q0] = z;
                    obs_buffer.zs[q1] = z;
                    result += sim.peek_observable_expectation(obs_buffer) != 0;
                    obs_buffer.xs[q0] = 0;
                    obs_buffer.xs[q1] = 0;
                    obs_buffer.zs[q0] = 0;
                    obs_buffer.zs[q1] = 0;
                    sim.do_gate(CircuitInstruction{inst.gate_type, {}, inst.targets.sub(k, k + 2), ""});
                }
                break;
            }

            case GateType::MPP: {
                for (size_t start = 0; start < inst.targets.size();) {
                    size_t end = start + 1;
                    while (end < inst.targets.size() && inst.targets[end].is_combiner()) {
                        end += 2;
                    }

                    for (size_t k = start; k < end; k += 2) {
                        auto t = inst.targets[k];
                        auto q = t.qubit_value();
                        obs_buffer.xs[q] = (bool)(t.data & TARGET_PAULI_X_BIT);
                        obs_buffer.zs[q] = (bool)(t.data & TARGET_PAULI_Z_BIT);
                    }
                    result += sim.peek_observable_expectation(obs_buffer) != 0;
                    obs_buffer.xs.clear();
                    obs_buffer.zs.clear();

                    sim.do_gate({inst.gate_type, {}, inst.targets.sub(start, end), ""});
                    start = end;
                }
                break;
            }
            default:
                throw std::invalid_argument("count_determined_measurements unhandled measurement type " + inst.str());
        }
    });
    return result;
}

}  // namespace stim
