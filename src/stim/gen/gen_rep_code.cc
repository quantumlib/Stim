#include "stim/gen/gen_rep_code.h"

using namespace stim;

GeneratedCircuit stim::generate_rep_code_circuit(const CircuitGenParameters &params) {
    if (params.task != "memory") {
        throw std::invalid_argument(
            "Unrecognized task '" + params.task +
            "'. Known repetition_code tasks:\n"
            "    'memory': Initialize |0>, protect with parity measurements, measure.\n");
    }
    if (params.rounds < 1) {
        throw std::invalid_argument("Need rounds >= 1.");
    }
    if (params.distance < 2) {
        throw std::invalid_argument("Need a distance >= 2.");
    }

    uint32_t m = params.distance - 1;
    uint32_t n = m * 2 + 1;

    // Lay out qubits and determine interaction targets.
    std::vector<uint32_t> all_qubits;
    std::vector<uint32_t> data_qubits;
    std::vector<uint32_t> cnot_targets_1;
    std::vector<uint32_t> cnot_targets_2;
    std::vector<uint32_t> measurement_qubits;
    for (uint32_t q = 0; q < n; q++) {
        all_qubits.push_back(q);
        if (q % 2 == 0) {
            data_qubits.push_back(q);
        } else {
            measurement_qubits.push_back(q);
            cnot_targets_1.push_back(q - 1);
            cnot_targets_1.push_back(q);
            cnot_targets_2.push_back(q + 1);
            cnot_targets_2.push_back(q);
        }
    }

    // Build the repeated actions that make up the repetition code cycle.
    Circuit cycle_actions;
    params.append_begin_round_tick(cycle_actions, data_qubits);
    params.append_unitary_2(cycle_actions, "CNOT", cnot_targets_1);
    cycle_actions.append_op("TICK", {});
    params.append_unitary_2(cycle_actions, "CNOT", cnot_targets_2);
    cycle_actions.append_op("TICK", {});
    params.append_measure_reset(cycle_actions, measurement_qubits);

    // Build the start of the circuit, getting a state that's ready to cycle.
    // In particular, the first cycle has different detectors and so has to be handled special.
    Circuit head;
    params.append_reset(head, all_qubits);
    head += cycle_actions;
    for (uint32_t k = 0; k < m; k++) {
        head.append_op("DETECTOR", {(m - k) | TARGET_RECORD_BIT}, {(double)2 * k + 1, 0});
    }

    // Build the repeated body of the circuit, including the detectors comparing to previous cycles.
    Circuit body = cycle_actions;
    body.append_op("SHIFT_COORDS", {}, {0, 1});
    for (uint32_t k = 0; k < m; k++) {
        body.append_op(
            "DETECTOR", {(m - k) | TARGET_RECORD_BIT, (2 * m - k) | TARGET_RECORD_BIT}, {(double)2 * k + 1, 0});
    }

    // Build the end of the circuit, getting out of the cycle state and terminating.
    // In particular, the data measurements create detectors that have to be handled special.
    // Also, the tail is responsible for identifying the logical observable.
    Circuit tail;
    params.append_measure(tail, data_qubits);
    for (uint32_t k = 0; k < m; k++) {
        tail.append_op(
            "DETECTOR",
            {(m - k) | TARGET_RECORD_BIT, (m - k + 1) | TARGET_RECORD_BIT, (2 * m - k + 1) | TARGET_RECORD_BIT},
            {(double)2 * k + 1, 1});
    }
    tail.append_op("OBSERVABLE_INCLUDE", {1 | TARGET_RECORD_BIT}, 0);

    // Combine to form final circuit.
    Circuit full_circuit = head + body * (params.rounds - 1) + tail;

    // Produce a 2d layout.
    std::map<std::pair<uint32_t, uint32_t>, std::pair<std::string, uint32_t>> layout;
    for (uint32_t k = 0; k < n; k++) {
        layout[{k, 0}] = {std::string() + "dZ"[k & 1], k};
    }
    layout[{0, 0}].first = "L";

    return {
        full_circuit,
        layout,
        "# Legend:\n"
        "#     d# = data qubit\n"
        "#     L# = data qubit with logical observable crossing\n"
        "#     Z# = measurement qubit\n"};
}
