#include "stim/arg_parse.h"
#include "stim/circuit/circuit.h"
#include "stim/circuit/stabilizer_flow.h"
#include "stim/simulators/frame_simulator_util.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/sparse_rev_frame_tracker.h"

namespace stim {

template <size_t W>
void _pauli_string_controlled_not(PauliStringRef<W> control, uint32_t target, Circuit &out) {
    for (uint32_t q = 0; q < (uint32_t)control.num_qubits; q++) {
        auto p = control.xs[q] + 2 * control.zs[q];
        if (p == 1) {
            out.safe_append_u("XCX", {q, target});
        } else if (p == 2) {
            out.safe_append_u("ZCX", {q, target});
        } else if (p == 3) {
            out.safe_append_u("YCX", {q, target});
        }
    }
    if (control.sign) {
        out.safe_append_u("X", {target});
    }
}

template <size_t W>
bool _sample_if_circuit_has_stabilizer_flow(
    size_t num_samples, std::mt19937_64 &rng, const Circuit &circuit, const StabilizerFlow<W> &flow) {
    uint32_t n = (uint32_t)circuit.count_qubits();
    n = std::max(n, (uint32_t)flow.input.num_qubits);
    n = std::max(n, (uint32_t)flow.output.num_qubits);
    Circuit augmented_circuit;
    for (uint32_t k = 0; k < n; k++) {
        augmented_circuit.safe_append_u("XCX", {k, k + n + 1}, {});
    }
    for (uint32_t k = 0; k < n; k++) {
        augmented_circuit.safe_append_u("DEPOLARIZE1", {k}, {0.75});
    }
    augmented_circuit.append_from_text("TICK");
    _pauli_string_controlled_not<W>(flow.input, n, augmented_circuit);
    augmented_circuit.append_from_text("TICK");
    augmented_circuit += circuit;
    augmented_circuit.append_from_text("TICK");

    _pauli_string_controlled_not<W>(flow.output, n, augmented_circuit);
    for (const auto &m : flow.measurement_outputs) {
        assert(m.is_measurement_record_target());
        std::vector<GateTarget> targets{m, GateTarget::qubit(n)};
        augmented_circuit.safe_append(GateType::CX, targets, {});
    }
    augmented_circuit.safe_append_u("M", {n}, {});

    auto out = sample_batch_measurements(
        augmented_circuit, TableauSimulator<W>::reference_sample_circuit(augmented_circuit), num_samples, rng, false);

    size_t m = augmented_circuit.count_measurements() - 1;
    return !out[m].not_zero();
}

template <size_t W>
std::vector<bool> sample_if_circuit_has_stabilizer_flows(
    size_t num_samples, std::mt19937_64 &rng, const Circuit &circuit, SpanRef<const StabilizerFlow<W>> flows) {
    std::vector<bool> result;
    for (const auto &flow : flows) {
        result.push_back(_sample_if_circuit_has_stabilizer_flow(num_samples, rng, circuit, flow));
    }
    return result;
}

inline bool parse_rec_allowing_non_negative(std::string_view rec, size_t num_measurements_for_non_neg, GateTarget *out) {
    if (rec.size() < 6 || rec[0] != 'r' || rec[1] != 'e' || rec[2] != 'c' || rec[3] != '[' || rec.back() != ']') {
        throw std::invalid_argument("");  // Caught and given a message below.
    }
    int64_t i = 0;
    if (!parse_int64(rec.substr(4, rec.size() - 5), &i)) {
        return false;
    }

    if (i >= INT32_MIN && i < 0) {
        *out = stim::GateTarget::rec((int32_t)i);
        return true;
    }
    if (i >= 0 && (size_t)i < num_measurements_for_non_neg) {
        *out = stim::GateTarget::rec((int32_t)i - (int32_t)num_measurements_for_non_neg);
        return true;
    }
    return false;
}

template <size_t W>
PauliString<W> parse_non_empty_pauli_string_allowing_i(std::string_view text, bool *imag_out) {
    *imag_out = false;
    if (text == "+1" || text == "1") {
        return PauliString<W>(0);
    }
    if (text == "-1") {
        PauliString<W> r(0);
        r.sign = true;
        return r;
    }
    if (text.empty()) {
        throw std::invalid_argument("Got an ambiguously blank pauli string. Use '1' for the empty Pauli string.");
    }

    bool negate = false;
    if (text.starts_with('i')) {
        *imag_out = true;
        text = text.substr(1);
    } else if (text.starts_with("-i")) {
        negate = true;
        *imag_out = true;
        text = text.substr(2);
    } else if (text.starts_with("+i")) {
        *imag_out = true;
        text = text.substr(2);
    }
    PauliString<W> result = PauliString<W>::from_str(text);
    if (negate) {
        result.sign ^= 1;
    }
    return result;
}

template <size_t W>
StabilizerFlow<W> StabilizerFlow<W>::from_str(const char *text, uint64_t num_measurements_for_non_neg_recs) {
    try {
        auto parts = split('>', text);
        if (parts.size() != 2 || parts[0].empty() || parts[0].back() != '-') {
            throw std::invalid_argument("");  // Caught and given a message below.
        }
        parts[0].pop_back();
        while (!parts[0].empty() && parts[0].back() == ' ') {
            parts[0].pop_back();
        }
        bool imag_inp = false;
        bool imag_out = false;
        PauliString<W> inp = parse_non_empty_pauli_string_allowing_i<W>(parts[0], &imag_inp);

        parts = split(' ', parts[1]);
        size_t k = 0;
        while (k < parts.size() && parts[k].empty()) {
            k += 1;
        }
        if (k >= parts.size()) {
            throw std::invalid_argument("");  // Caught and given a message below.
        }
        PauliString<W> out(0);
        std::vector<GateTarget> measurements;
        if (!parts[k].empty() && parts[k][0] != 'r') {
            out = parse_non_empty_pauli_string_allowing_i<W>(parts[k], &imag_out);
        } else {
            GateTarget t;
            if (!parse_rec_allowing_non_negative(parts[k], num_measurements_for_non_neg_recs, &t)) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            measurements.push_back(t);
        }
        k++;
        while (k < parts.size()) {
            if (parts[k] != "xor" || k + 1 == parts.size()) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            GateTarget rec;
            if (!parse_rec_allowing_non_negative(parts[k + 1], num_measurements_for_non_neg_recs, &rec)) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            measurements.push_back(rec);
            k += 2;
        }
        if (imag_inp != imag_out) {
            throw std::invalid_argument("Anti-hermitian flows aren't allowed.");
        }
        return StabilizerFlow{inp, out, measurements};
    } catch (const std::invalid_argument &ex) {
        throw std::invalid_argument("Invalid stabilizer flow text: '" + std::string(text) + "'.");
    }
}

template <size_t W>
bool StabilizerFlow<W>::operator==(const StabilizerFlow<W> &other) const {
    return input == other.input && output == other.output && measurement_outputs == other.measurement_outputs;
}

template <size_t W>
bool StabilizerFlow<W>::operator!=(const StabilizerFlow<W> &other) const {
    return !(*this == other);
}

template <size_t W>
std::string StabilizerFlow<W>::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

template <size_t W>
std::ostream &operator<<(std::ostream &out, const StabilizerFlow<W> &flow) {
    if (flow.input.num_qubits == 0) {
        if (flow.input.sign) {
            out << "-";
        }
        out << "1";
    } else {
        out << flow.input;
    }
    out << " -> ";
    bool skip_xor = false;
    if (flow.output.num_qubits == 0) {
        if (flow.output.sign) {
            out << "-1";
        } else if (flow.measurement_outputs.empty()) {
            out << "+1";
        }
        skip_xor = true;
    } else {
        out << flow.output;
    }
    for (const auto &t : flow.measurement_outputs) {
        if (!skip_xor) {
            out << " xor ";
        }
        skip_xor = false;
        t.write_succinct(out);
    }
    return out;
}

template <size_t W>
std::vector<bool> check_if_circuit_has_unsigned_stabilizer_flows(const Circuit &circuit, SpanRef<const StabilizerFlow<W>> flows) {
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
        rev.undo_DETECTOR(CircuitInstruction{GateType::DETECTOR, {}, flow.measurement_outputs});
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
    for (const auto &anti : rev.anticommutations) {
        result[anti.val()] = false;
    }

    return result;
}

}  // namespace stim
