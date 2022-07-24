#include "stim/stabilizers/conversions.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

uint8_t stim::floor_lg2(size_t value) {
    uint8_t result = 0;
    while (value > 1) {
        result += 1;
        value >>= 1;
    }
    return result;
}

uint8_t stim::is_power_of_2(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

size_t biggest_index(const std::vector<std::complex<float>> &state_vector) {
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

size_t compute_occupation(const std::vector<std::complex<float>> &state_vector) {
    size_t c = 0;
    for (const auto &v : state_vector) {
        if (v != std::complex<float>{0, 0}) {
            c++;
        }
    }
    return c;
}

Circuit stim::stabilizer_state_vector_to_circuit(const std::vector<std::complex<float>> &state_vector, bool little_endian) {
    if (!is_power_of_2(state_vector.size())) {
        std::stringstream ss;
        ss << "Expected number of amplitudes to be a power of 2.";
        ss << " The given state vector had " << state_vector.size() << " amplitudes.";
        throw std::invalid_argument(ss.str());
    }

    uint8_t num_qubits = floor_lg2(state_vector.size());
    double weight = 0;
    for (const auto &c : state_vector) {
        weight += std::norm(c);
    }
    if (abs(weight - 1) > 0.125) {
        throw std::invalid_argument("The given state vector wasn't a unit vector. It had a length of " + std::to_string(weight) + ".");
    }

    VectorSimulator sim(num_qubits);
    sim.state = state_vector;

    Circuit recorded;
    auto apply = [&](const std::string &name, uint32_t target) {
        sim.apply(name, target);
        recorded.append_op(name, {little_endian ? target : (num_qubits - target - 1)});
    };
    auto apply2 = [&](const std::string &name, uint32_t target, uint32_t target2) {
        sim.apply(name, target, target2);
        recorded.append_op(name, {
            little_endian ? target : (num_qubits - target - 1),
            little_endian ? target2 : (num_qubits - target2 - 1),
        });
    };

    // Move biggest amplitude to start of state vector..
    size_t pivot = biggest_index(state_vector);
    for (size_t q = 0; q < num_qubits; q++) {
        if ((pivot >> q) & 1) {
            apply("X", q);
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
                    apply2("CNOT", base_qubit, q);
                }
            }
        }

        auto s = sim.state[1 << base_qubit];
        assert(s != (std::complex<float>{0, 0}));
        if (s == std::complex<float>{-1, 0}) {
            apply("Z", base_qubit);
        } else if (s == std::complex<float>{0, 1}) {
            apply("S_DAG", base_qubit);
        } else if (s == std::complex<float>{0, -1}) {
            apply("S", base_qubit);
        }
        apply("H", base_qubit);

        sim.smooth_stabilizer_state(sim.state[0]);
        if (compute_occupation(sim.state) * 2 != occupation) {
            throw std::invalid_argument("State vector isn't a stabilizer state.");
        }
        occupation >>= 1;
    }

    // Note: requires all targets within an individual CNOT to commute.
    // This is guaranteed by the code above.
    Circuit inverted;
    recorded.for_each_operation_reverse([&](const Operation &op) {
        inverted.append_operation(op.gate->inverse(), op.target_data.targets, op.target_data.args);
    });

    if (inverted.count_qubits() < num_qubits) {
        inverted.append_op("I", {(uint32_t)(num_qubits - 1)});
    }

    return inverted;
}

Tableau stim::circuit_to_tableau(const Circuit &circuit, bool ignore_noise, bool ignore_measurement, bool ignore_reset) {
    Tableau result(circuit.count_qubits());
    std::mt19937_64 unused_rng(0);
    TableauSimulator sim(unused_rng, circuit.count_qubits());

    circuit.for_each_operation([&](const Operation &op){
        if (op.gate->flags & GATE_IS_UNITARY) {
            (sim.*op.gate->tableau_simulator_function)(op.target_data);
        } else if (op.gate->flags & GATE_IS_NOISE) {
            if (!ignore_noise) {
                throw std::invalid_argument(
                    "The circuit has no well-defined tableau because it contains noisy operations.\n"
                    "To ignore noisy operations, pass the argument ignore_noise=True.\n"
                    "The first noisy operation is: " + op.str());
            }
        } else if (op.gate->flags & (GATE_IS_RESET | GATE_PRODUCES_NOISY_RESULTS)) {
            if (!ignore_measurement && (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS)) {
                throw std::invalid_argument(
                    "The circuit has no well-defined tableau because it contains measurement operations.\n"
                    "To ignore measurement operations, pass the argument ignore_measurement=True.\n"
                    "The first measurement operation is: " + op.str());
            }
            if (!ignore_reset && (op.gate->flags & GATE_IS_RESET)) {
                throw std::invalid_argument(
                    "The circuit has no well-defined tableau because it contains reset operations.\n"
                    "To ignore reset operations, pass the argument ignore_reset=True.\n"
                    "The first reset operation is: " + op.str());
            }
        } else {
            // Operation should be an annotation like TICK or DETECTOR.
        }
    });

    return sim.inv_state.inverse();
}

std::vector<std::complex<float>> stim::circuit_to_output_state_vector(const Circuit &circuit, bool little_endian) {
    Tableau result(circuit.count_qubits());
    std::mt19937_64 unused_rng(0);
    TableauSimulator sim(unused_rng, circuit.count_qubits());

    circuit.for_each_operation([&](const Operation &op){
        if (op.gate->flags & GATE_IS_UNITARY) {
            (sim.*op.gate->tableau_simulator_function)(op.target_data);
        } else if (op.gate->flags & (GATE_IS_NOISE | GATE_IS_RESET | GATE_PRODUCES_NOISY_RESULTS)) {
            throw std::invalid_argument(
                "The circuit has no well-defined tableau because it contains noisy or dissipative operations.\n"
                "The first such operation is: " + op.str());
        } else {
            // Operation should be an annotation like TICK or DETECTOR.
        }
    });

    return sim.to_state_vector(little_endian);
}
