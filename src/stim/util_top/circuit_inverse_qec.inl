#include "stim/stabilizers/flow.h"
#include "stim/util_top/circuit_inverse_qec.h"

namespace stim {

namespace internal {
template <size_t W>
void CircuitFlowReverser::xor_pauli_string_into_tracker_as_target(
    const PauliString<W> &pauli_string, DemTarget target) {
    pauli_string.ref().for_each_active_pauli([&](size_t q) {
        bool x = pauli_string.xs[q];
        bool z = pauli_string.zs[q];
        if (x) {
            rev.xs[q].xor_item(target);
        }
        if (z) {
            rev.zs[q].xor_item(target);
        }
    });
}

template <size_t W>
void CircuitFlowReverser::xor_flow_ends_into_tracker(std::span<const Flow<W>> flows) {
    for (size_t k = 0; k < flows.size(); k++) {
        const auto &flow = flows[k];
        DemTarget flow_target = DemTarget::observable_id(stats.num_observables + k);
        xor_pauli_string_into_tracker_as_target(flow.output, flow_target);
    }
}

template <size_t W>
void CircuitFlowReverser::xor_flow_measurements_into_tracker(std::span<const Flow<W>> flows) {
    for (size_t k = 0; k < flows.size(); k++) {
        const auto &flow = flows[k];
        DemTarget flow_target = DemTarget::observable_id(stats.num_observables + k);

        for (int32_t m : flow.measurements) {
            if (m < 0) {
                m += stats.num_measurements;
            }
            if (m < 0 || (uint64_t)m >= stats.num_measurements) {
                std::stringstream ss;
                ss << "Out of range measurement in one of the flows: " << flow;
                throw std::invalid_argument(ss.str());
            }
            rev.rec_bits[m].sorted_items.push_back(flow_target);
        }
    }
}

template <size_t W>
void CircuitFlowReverser::xor_flow_starts_into_tracker(std::span<const Flow<W>> flows) {
    for (size_t k = 0; k < flows.size(); k++) {
        const auto &flow = flows[k];
        DemTarget flow_target = DemTarget::observable_id(stats.num_observables + k);
        xor_pauli_string_into_tracker_as_target(flow.input, flow_target);
    }
}

template <size_t W>
void CircuitFlowReverser::verify_flow_observables_disappeared(std::span<const Flow<W>> flows) {
    bool failed = false;
    DemTarget example{};
    for (size_t q = 0; q < stats.num_qubits; q++) {
        for (auto &e : rev.xs[q]) {
            failed = true;
            example = e;
        }
        for (auto &e : rev.zs[q]) {
            failed = true;
            example = e;
        }
    }
    if (failed) {
        if (example.is_relative_detector_id() ||
            (example.is_observable_id() && example.raw_id() < stats.num_observables)) {
            std::stringstream ss;
            ss << "The detecting region of " << example << " reached the start of the circuit.\n";
            ss << "Only flows given as arguments are permitted to touch the start or end of the circuit.\n";
            ss << "There are four potential ways to fix this issue, depending on what's wrong:\n";
            ss << "- If " + example.str() +
                      " was relying on implicit initialization into |0> at the start of the circuit, add explicit "
                      "resets to the circuit.\n";
            ss << "- If " + example.str() + " shouldn't be reaching the start of the circuit, fix its declaration.\n";
            ss << "- If " + example.str() + " isn't needed, delete it from the circuit.\n";
            ss << "- If the given circuit is a partial circuit, and " << example
               << " is reaching outside of it, refactor " << example << "into a flow argument.";
            throw std::invalid_argument(ss.str());
        } else {
            std::stringstream ss;
            const auto &flow = flows[example.raw_id() - stats.num_observables];
            ss << "The circuit didn't satisfy one of the given flows (ignoring sign): ";
            ss << flow;
            auto v = rev.current_error_sensitivity_for<W>(example);
            v.xs ^= flow.input.xs;
            v.zs ^= flow.input.zs;
            ss << "\nChanging the flow to '"
               << Flow<W>{.input = v, .output = flow.output, .measurements = flow.measurements}
               << "' would make it a valid flow.";
            throw std::invalid_argument(ss.str());
        }
    }
}

template <size_t W>
std::vector<Flow<W>> CircuitFlowReverser::build_inverted_flows(std::span<const Flow<W>> flows) {
    std::vector<Flow<W>> inverted_flows;
    for (size_t k = 0; k < flows.size(); k++) {
        const auto &f = flows[k];
        inverted_flows.push_back(
            Flow<W>{
                .input = f.output,
                .output = f.input,
                .measurements = {},
            });
        auto &f2 = inverted_flows.back();
        f2.input.sign = false;
        f2.output.sign = false;
        for (auto &m : d2ms[DemTarget::observable_id(k + stats.num_observables)]) {
            f2.measurements.push_back((int32_t)m - (int32_t)num_new_measurements);
        }
    }
    return inverted_flows;
}
}  // namespace internal

template <size_t W>
std::pair<Circuit, std::vector<Flow<W>>> circuit_inverse_qec(
    const Circuit &circuit, std::span<const Flow<W>> flows, bool dont_turn_measurements_into_resets) {
    size_t max_flow_qubit = 0;
    for (const auto &flow : flows) {
        max_flow_qubit = std::max(max_flow_qubit, flow.input.num_qubits);
        max_flow_qubit = std::max(max_flow_qubit, flow.output.num_qubits);
    }
    if (max_flow_qubit >= UINT32_MAX) {
        throw std::invalid_argument("Flow qubit is too large. Not supported.");
    }

    CircuitStats stats = circuit.compute_stats();
    stats.num_qubits = std::max(stats.num_qubits, (uint32_t)max_flow_qubit + 1);
    internal::CircuitFlowReverser reverser(stats, dont_turn_measurements_into_resets);

    reverser.xor_flow_ends_into_tracker(flows);
    reverser.xor_flow_measurements_into_tracker(flows);
    circuit.for_each_operation_reverse([&](const CircuitInstruction &inst) {
        reverser.do_instruction(inst);
    });
    reverser.xor_flow_starts_into_tracker(flows);
    reverser.verify_flow_observables_disappeared(flows);

    auto inverted_flows = reverser.build_inverted_flows(flows);
    Circuit inverted_circuit = reverser.build_and_move_final_inverted_circuit();
    return {inverted_circuit, inverted_flows};
}

}  // namespace stim
