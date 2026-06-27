#include "stim/util_top/circuit_vs_amplitudes.h"

#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/util_bot/twiddle.h"
#include "stim/util_top/circuit_inverse_unitary.h"

using namespace stim;

inline static size_t biggest_index(const std::vector<std::complex<float>> &state_vector) {
    size_t best_index = 0;
    float best_size = std::norm(state_vector[0]);
    for (size_t k = 1; k < state_vector.size(); k++) {
        float size = std::norm(state_vector[k]);
        if (size > best_size) {
            best_size = size;
            best_index = k;
        }
    }
    return best_index;
}

inline static size_t compute_occupation(const std::vector<std::complex<float>> &state_vector) {
    size_t c = 0;
    for (const auto &v : state_vector) {
        if (v != std::complex<float>{0, 0}) {
            c++;
        }
    }
    return c;
}

Circuit stim::stabilizer_state_vector_to_circuit(
    const std::vector<std::complex<float>> &state_vector, bool little_endian) {
    if (!is_power_of_2(state_vector.size())) {
        std::stringstream ss;
        ss << "Expected number of amplitudes to be a power of 2.";
        ss << " The given state vector had " << state_vector.size() << " amplitudes.";
        throw std::invalid_argument(ss.str());
    }

    uint8_t num_qubits = floor_lg2(state_vector.size());
    VectorSimulator sim(num_qubits);
    sim.state = state_vector;

    Circuit recorded;
    auto apply = [&](GateType gate_type, uint32_t target) {
        sim.apply(gate_type, target);
        recorded.safe_append(CircuitInstruction(
            gate_type,
            {},
            std::vector<GateTarget>{
                GateTarget::qubit(little_endian ? target : (num_qubits - target - 1)),
            },
            ""));
    };
    auto apply2 = [&](GateType gate_type, uint32_t target, uint32_t target2) {
        sim.apply(gate_type, target, target2);
        recorded.safe_append(CircuitInstruction(
            gate_type,
            {},
            std::vector<GateTarget>{
                GateTarget::qubit(little_endian ? target : (num_qubits - target - 1)),
                GateTarget::qubit(little_endian ? target2 : (num_qubits - target2 - 1)),
            },
            ""));
    };

    // Move biggest amplitude to start of state vector.
    size_t pivot = biggest_index(state_vector);
    for (size_t q = 0; q < num_qubits; q++) {
        if ((pivot >> q) & 1) {
            apply(GateType::X, q);
        }
    }
    sim.smooth_stabilizer_state(sim.state[0]);
    size_t occupation = compute_occupation(sim.state);
    if (!is_power_of_2(occupation)) {
        throw std::invalid_argument("State vector isn't a stabilizer state.");
    }

    // Repeatedly cancel amplitudes
    while (occupation > 1) {
        size_t k = 1;
        for (; k < state_vector.size(); k++) {
            if (sim.state[k].real() || sim.state[k].imag()) {
                break;
            }
        }
        if (k == state_vector.size()) {
            break;
        }

        size_t base_qubit = SIZE_MAX;
        for (size_t q = 0; q < num_qubits; q++) {
            if ((k >> q) & 1) {
                if (base_qubit == SIZE_MAX) {
                    base_qubit = q;
                } else {
                    apply2(GateType::CX, base_qubit, q);
                }
            }
        }

        auto s = sim.state[1 << base_qubit];
        assert(s != (std::complex<float>{0, 0}));
        if (s == std::complex<float>{-1, 0}) {
            apply(GateType::Z, base_qubit);
        } else if (s == std::complex<float>{0, 1}) {
            apply(GateType::S_DAG, base_qubit);
        } else if (s == std::complex<float>{0, -1}) {
            apply(GateType::S, base_qubit);
        }
        apply(GateType::H, base_qubit);

        sim.smooth_stabilizer_state(sim.state[0]);
        if (compute_occupation(sim.state) * 2 != occupation) {
            throw std::invalid_argument("State vector isn't a stabilizer state.");
        }
        occupation >>= 1;
    }

    recorded = circuit_inverse_unitary(recorded);
    if (recorded.count_qubits() < num_qubits) {
        recorded.safe_append_u("I", {(uint32_t)(num_qubits - 1)});
    }

    return recorded;
}

std::vector<std::complex<float>> stim::circuit_to_output_state_vector(const Circuit &circuit, bool little_endian) {
    Tableau<64> result(circuit.count_qubits());
    TableauSimulator<64> sim(std::mt19937_64(0), circuit.count_qubits());

    circuit.for_each_operation([&](const CircuitInstruction &op) {
        const auto &flags = GATE_DATA[op.gate_type].flags;
        if (flags & GATE_IS_UNITARY) {
            sim.do_gate(op);
        } else if (flags & (GATE_IS_NOISY | GATE_IS_RESET | GATE_PRODUCES_RESULTS)) {
            throw std::invalid_argument(
                "The circuit has no well-defined tableau because it contains noisy or dissipative operations.\n"
                "The first such operation is: " +
                op.str());
        } else {
            // Operation should be an annotation like TICK or DETECTOR.
        }
    });

    return sim.to_state_vector(little_endian);
}
