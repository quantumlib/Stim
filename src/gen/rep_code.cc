#include "rep_code.h"

using namespace stim_internal;

GeneratedCircuit stim_internal::generate_rep_code_circuit(const CircuitGenParameters &params) {
    if (params.task != "memory_z") {
        throw std::out_of_range("Rep code supports task=memory_z, not task='" + params.task + "'.");
    }
    uint32_t d = params.distance;
    size_t n = d * 2 + 1;
    assert(params.rounds > 0);

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

    Circuit body;
    params.append_round_transition(body, data_qubits);
    params.append_unitary_2(body, "CNOT", cnot_targets_1);
    body.append_op("TICK", {});
    params.append_unitary_2(body, "CNOT", cnot_targets_2);
    body.append_op("TICK", {});
    params.append_measure_reset(body, measurement_qubits);

    Circuit head;
    params.append_reset(head, all_qubits);
    head += body;
    for (uint32_t k = 0; k < d; k++) {
        head.append_op("DETECTOR", {(k + 1) | TARGET_RECORD_BIT});
        body.append_op("DETECTOR", {(k + 1) | TARGET_RECORD_BIT, (k + 1 + d) | TARGET_RECORD_BIT});
    }

    Circuit tail;
    params.append_measure(tail, data_qubits);
    for (uint32_t k = 0; k < d; k++) {
        tail.append_op("DETECTOR", {(k + 1) | TARGET_RECORD_BIT, (k + 2) | TARGET_RECORD_BIT, (k + 2 + d) | TARGET_RECORD_BIT});
    }
    tail.append_op("OBSERVABLE_INCLUDE", {1 | TARGET_RECORD_BIT}, 0);

    Circuit c = head + body * (params.rounds - 1) + tail;
    std::map<std::pair<uint32_t, uint32_t>, std::pair<std::string, uint32_t>> layout;
    for (size_t k = 0; k < n; k++) {
        layout[{k, 0}] = {std::string() + "dZ"[k & 1], k};
    }
    layout[{0, 0}].first = "L";
    return {c, layout};
}
