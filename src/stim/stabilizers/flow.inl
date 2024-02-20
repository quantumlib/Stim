#include "stim/arg_parse.h"
#include "stim/circuit/circuit.h"
#include "stim/simulators/frame_simulator_util.h"
#include "stim/simulators/sparse_rev_frame_tracker.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/flex_pauli_string.h"
#include "stim/stabilizers/flow.h"

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
static GateTarget measurement_index_to_target(int32_t m, uint64_t num_measurements, const Flow<W> &flow) {
    if ((m >= 0 && (uint64_t)m >= num_measurements) || (m < 0 && (uint64_t) - (int64_t)m > num_measurements)) {
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
bool _sample_if_circuit_has_stabilizer_flow(
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
        augmented_circuit.safe_append(GateType::CX, targets, {});
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
    std::vector<bool> result;
    for (const auto &flow : flows) {
        result.push_back(_sample_if_circuit_has_stabilizer_flow(num_samples, rng, circuit, flow));
    }
    return result;
}

inline bool parse_rec_allowing_non_negative(std::string_view rec, int32_t *out) {
    if (rec.size() < 6 || rec[0] != 'r' || rec[1] != 'e' || rec[2] != 'c' || rec[3] != '[' || rec.back() != ']') {
        throw std::invalid_argument("");  // Caught and given a message below.
    }
    int64_t i = 0;
    if (!parse_int64(rec.substr(4, rec.size() - 5), &i)) {
        return false;
    }

    if (i >= INT32_MIN && i <= INT32_MAX) {
        *out = (int32_t)i;
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

    auto flex = FlexPauliString::from_text(text);
    *imag_out = flex.imag;

    PauliString<W> result(flex.value.num_qubits);
    size_t nb = std::min(flex.value.xs.num_u8_padded(), result.xs.num_u8_padded());
    memcpy(result.xs.u8, flex.value.xs.u8, nb);
    memcpy(result.zs.u8, flex.value.zs.u8, nb);
    result.sign = flex.value.sign;
    return result;
}

template <size_t W>
Flow<W> Flow<W>::from_str(std::string_view text) {
    try {
        auto parts = split_view('>', text);
        if (parts.size() != 2 || parts[0].empty() || parts[0].back() != '-') {
            throw std::invalid_argument("");  // Caught and given a message below.
        }
        parts[0] = parts[0].substr(0, parts[0].size() - 1);
        while (!parts[0].empty() && parts[0].back() == ' ') {
            parts[0] = parts[0].substr(0, parts[0].size() - 1);
        }
        bool imag_inp = false;
        bool imag_out = false;
        PauliString<W> inp = parse_non_empty_pauli_string_allowing_i<W>(parts[0], &imag_inp);

        parts = split_view(' ', parts[1]);
        size_t k = 0;
        while (k < parts.size() && parts[k].empty()) {
            k += 1;
        }
        if (k >= parts.size()) {
            throw std::invalid_argument("");  // Caught and given a message below.
        }
        PauliString<W> out(0);
        std::vector<int32_t> measurements;
        if (!parts[k].empty() && parts[k][0] != 'r') {
            out = parse_non_empty_pauli_string_allowing_i<W>(parts[k], &imag_out);
        } else {
            int32_t rec;
            if (!parse_rec_allowing_non_negative(parts[k], &rec)) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            measurements.push_back(rec);
        }
        k++;
        while (k < parts.size()) {
            if (parts[k] != "xor" || k + 1 == parts.size()) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            int32_t rec;
            if (!parse_rec_allowing_non_negative(parts[k + 1], &rec)) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            measurements.push_back(rec);
            k += 2;
        }
        if (imag_inp != imag_out) {
            throw std::invalid_argument("Anti-Hermitian flows aren't allowed.");
        }
        return Flow{inp, out, measurements};
    } catch (const std::invalid_argument &ex) {
        if (*ex.what() != '\0') {
            throw;
        }
        throw std::invalid_argument("Invalid stabilizer flow text: '" + std::string(text) + "'.");
    }
}

template <size_t W>
bool Flow<W>::operator==(const Flow<W> &other) const {
    return input == other.input && output == other.output && measurements == other.measurements;
}

template <size_t W>
bool Flow<W>::operator!=(const Flow<W> &other) const {
    return !(*this == other);
}

template <size_t W>
std::string Flow<W>::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

template <size_t W>
std::ostream &operator<<(std::ostream &out, const Flow<W> &flow) {
    bool use_sparse = false;

    // Sparse is only useful if most terms are identity.
    if (flow.input.num_qubits > 8 && flow.input.ref().weight() * 8 <= flow.input.num_qubits) {
        use_sparse = true;
    }
    if (flow.output.num_qubits > 8 && flow.output.ref().weight() * 8 <= flow.output.num_qubits) {
        use_sparse = true;
    }

    // Sparse would lose length data if the last pauli is an identity.
    if (flow.input.num_qubits > 0 && !flow.input.xs[flow.input.num_qubits - 1] &&
        !flow.input.zs[flow.input.num_qubits - 1]) {
        use_sparse = false;
    }
    if (flow.output.num_qubits > 0 && !flow.output.xs[flow.output.num_qubits - 1] &&
        !flow.output.zs[flow.output.num_qubits - 1]) {
        use_sparse = false;
    }

    auto write_sparse = [&](const PauliString<W> &ps) -> bool {
        if (ps.sign) {
            out << "-";
        }
        bool has_any = false;
        for (size_t q = 0; q < ps.num_qubits; q++) {
            uint8_t p = ps.xs[q] + 2 * ps.zs[q];
            if (use_sparse) {
                if (p) {
                    if (has_any) {
                        out << "*";
                    }
                    out << "_XZY"[p];
                    out << q;
                    has_any = true;
                }
            } else {
                out << "_XZY"[p];
                has_any = true;
            }
        }
        return has_any;
    };

    if (!write_sparse(flow.input)) {
        out << "1";
    }
    out << " -> ";
    bool has_out = write_sparse(flow.output);
    for (const auto &t : flow.measurements) {
        if (has_out) {
            out << " xor ";
        }
        has_out = true;
        out << "rec[" << t << "]";
    }
    if (!has_out) {
        out << "1";
    }
    return out;
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
        rev.undo_DETECTOR(CircuitInstruction{GateType::DETECTOR, {}, targets});
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
