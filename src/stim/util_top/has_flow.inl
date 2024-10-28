#include "stim/util_top/has_flow.h"

namespace stim {

template <size_t W>
void _pauli_string_controlled_not(PauliStringRef<W> control, uint32_t target, Circuit &out) {
    control.for_each_active_pauli([&](size_t q) {
        uint32_t q32 = (uint32_t)q;
        auto p = control.xs[q] + 2 * control.zs[q];
        if (p == 1) {
            out.safe_append_u("XCX", {q32, target});
        } else if (p == 2) {
            out.safe_append_u("ZCX", {q32, target});
        } else if (p == 3) {
            out.safe_append_u("YCX", {q32, target});
        }
    });
    if (control.sign) {
        out.safe_append_u("X", {target});
    }
}

template <size_t W>
static GateTarget measurement_index_to_target(int32_t m, uint64_t num_measurements, const Flow<W> &flow) {
    if ((m >= 0 && (uint64_t)m >= num_measurements) || (m < 0 && (uint64_t)-(int64_t)m > num_measurements)) {
        std::stringstream ss;
        ss << "The flow '" << flow;
        ss << "' is malformed for the given circuit. ";
        ss << "The flow mentions a measurement index '" << m;
        ss << "', but this index out of range because the circuit only has ";
        ss << num_measurements << " measurements.";
        throw std::invalid_argument(ss.str());
    }
    if (m >= 0) {
        m -= num_measurements;
    }
    return GateTarget::rec(m);
}

template <size_t W>
bool _sample_if_noiseless_circuit_has_stabilizer_flow(
    size_t num_samples, std::mt19937_64 &rng, const Circuit &circuit, const Flow<W> &flow) {
    uint32_t num_qubits = (uint32_t)circuit.count_qubits();
    uint64_t num_measurements = circuit.count_measurements();

    num_qubits = std::max(num_qubits, (uint32_t)flow.input.num_qubits);
    num_qubits = std::max(num_qubits, (uint32_t)flow.output.num_qubits);
    Circuit augmented_circuit;
    for (uint32_t k = 0; k < num_qubits; k++) {
        augmented_circuit.safe_append_u("XCX", {k, k + num_qubits + 1}, {});
    }
    for (uint32_t k = 0; k < num_qubits; k++) {
        augmented_circuit.safe_append_u("DEPOLARIZE1", {k}, {0.75});
    }
    augmented_circuit.append_from_text("TICK");
    _pauli_string_controlled_not<W>(flow.input, num_qubits, augmented_circuit);
    augmented_circuit.append_from_text("TICK");
    augmented_circuit += circuit;
    augmented_circuit.append_from_text("TICK");

    _pauli_string_controlled_not<W>(flow.output, num_qubits, augmented_circuit);
    for (int32_t m : flow.measurements) {
        std::array<GateTarget, 2> targets{
            measurement_index_to_target<W>(m, num_measurements, flow), GateTarget::qubit(num_qubits)};
        augmented_circuit.safe_append(CircuitInstruction(GateType::CX, {}, targets, ""));
    }
    augmented_circuit.safe_append_u("M", {num_qubits}, {});

    auto out = sample_batch_measurements(
        augmented_circuit, TableauSimulator<W>::reference_sample_circuit(augmented_circuit), num_samples, rng, false);

    size_t m = augmented_circuit.count_measurements() - 1;
    return !out[m].not_zero();
}

template <size_t W>
std::vector<bool> sample_if_circuit_has_stabilizer_flows(
    size_t num_samples, std::mt19937_64 &rng, const Circuit &circuit, std::span<const Flow<W>> flows) {
    const auto &noiseless = circuit.aliased_noiseless_circuit();
    std::vector<bool> result;
    for (const auto &flow : flows) {
        result.push_back(_sample_if_noiseless_circuit_has_stabilizer_flow(num_samples, rng, noiseless, flow));
    }
    return result;
}

template <size_t W>
std::vector<bool> check_if_circuit_has_unsigned_stabilizer_flows(
    const Circuit &circuit, std::span<const Flow<W>> flows) {
    auto stats = circuit.compute_stats();
    size_t num_qubits = stats.num_qubits;
    for (const auto &flow : flows) {
        num_qubits = std::max(num_qubits, flow.input.num_qubits);
        num_qubits = std::max(num_qubits, flow.output.num_qubits);
    }
    SparseUnsignedRevFrameTracker rev(num_qubits, stats.num_measurements, flows.size(), false);

    // Add end of flows into frames.
    for (size_t f = 0; f < flows.size(); f++) {
        const auto &flow = flows[f];
        for (size_t q = 0; q < flow.output.num_qubits; q++) {
            if (flow.output.xs[q]) {
                rev.xs[q].xor_item(DemTarget::relative_detector_id(f));
            }
            if (flow.output.zs[q]) {
                rev.zs[q].xor_item(DemTarget::relative_detector_id(f));
            }
        }
    }

    // Mark measurements for inclusion.
    for (size_t f = flows.size(); f--;) {
        const auto &flow = flows[f];
        std::vector<GateTarget> targets;
        for (int32_t m : flow.measurements) {
            targets.push_back(measurement_index_to_target<W>(m, stats.num_measurements, flow));
        }
        rev.undo_DETECTOR(CircuitInstruction{GateType::DETECTOR, {}, targets, ""});
    }

    // Undo the circuit.
    circuit.for_each_operation_reverse([&](const CircuitInstruction &inst) {
        if (inst.gate_type == GateType::DETECTOR) {
            // Substituted.
        } else if (inst.gate_type == GateType::OBSERVABLE_INCLUDE) {
            // Skip.
        } else {
            rev.undo_gate(inst);
        }
    });

    // Remove start of flows from frames.
    for (size_t f = 0; f < flows.size(); f++) {
        const auto &flow = flows[f];
        for (size_t q = 0; q < flow.input.num_qubits; q++) {
            if (flow.input.xs[q]) {
                rev.xs[q].xor_item(DemTarget::relative_detector_id(f));
            }
            if (flow.input.zs[q]) {
                rev.zs[q].xor_item(DemTarget::relative_detector_id(f));
            }
        }
    }

    // Determine which flows survived.
    std::vector<bool> result(flows.size(), true);
    for (const auto &xs : rev.xs) {
        for (const auto &t : xs) {
            result[t.val()] = false;
        }
    }
    for (const auto &zs : rev.zs) {
        for (const auto &t : zs) {
            result[t.val()] = false;
        }
    }
    for (const auto &[dem_target, gate_target] : rev.anticommutations) {
        result[dem_target.val()] = false;
    }

    return result;
}

}  // namespace stim
