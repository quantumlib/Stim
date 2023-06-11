#include "stim/circuit/stabilizer_flow.h"

#include "stim/arg_parse.h"
#include "stim/circuit/circuit.h"
#include "stim/simulators/frame_simulator_util.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

void _pauli_string_controlled_not(PauliStringRef control, uint32_t target, Circuit &out) {
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

bool _check_if_circuit_has_stabilizer_flow(
    size_t num_samples,
    std::mt19937_64 &rng,
    const Circuit &circuit,
    const StabilizerFlow &flow) {

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
    _pauli_string_controlled_not(flow.input, n, augmented_circuit);
    augmented_circuit.append_from_text("TICK");
    augmented_circuit += circuit;
    augmented_circuit.append_from_text("TICK");

    _pauli_string_controlled_not(flow.output, n, augmented_circuit);
    for (const auto &m : flow.measurement_outputs) {
        assert(m.is_measurement_record_target());
        std::vector<GateTarget> targets{m, GateTarget::qubit(n)};
        augmented_circuit.safe_append(GateType::CX, targets, {});
    }
    augmented_circuit.safe_append_u("M", {n}, {});

    auto out = sample_batch_measurements(
        augmented_circuit,
        TableauSimulator::reference_sample_circuit(augmented_circuit),
        num_samples,
        rng,
        false);

    size_t m = augmented_circuit.count_measurements() - 1;
    return !out[m].not_zero();
}

std::vector<bool> stim::check_if_circuit_has_stabilizer_flows(
    size_t num_samples,
    std::mt19937_64 &rng,
    const Circuit &circuit,
    const std::vector<StabilizerFlow> flows) {
    std::vector<bool> result;
    for (const auto &flow : flows) {
        result.push_back(_check_if_circuit_has_stabilizer_flow(
            num_samples, rng, circuit, flow));
    }
    return result;
}

StabilizerFlow StabilizerFlow::from_str(const char *text) {
    try {
        auto parts = split('>', text);
        if (parts.size() != 2 || parts[0].empty() || parts[0].back() != '-') {
           throw std::invalid_argument("");
        }
        parts[0].pop_back();
        while (!parts[0].empty() && parts[0].back() == ' ') {
           parts[0].pop_back();
        }
        PauliString input = parts[0] == "1" ? PauliString(0) : parts[0] == "-1" ? PauliString::from_str("-") : PauliString::from_str(parts[0].c_str());

        parts = split(' ', parts[1]);
        size_t k = 0;
        while (k < parts.size() && parts[k].empty()) {
           k += 1;
        }
        PauliString output(0);
        std::vector<GateTarget> measurements;

        if (!parts[k].empty() && parts[k][0] != 'r') {
           output = PauliString::from_str(parts[k].c_str());
        } else {
           auto t = stim::GateTarget::from_target_str(parts[k].c_str());
           if (!t.is_measurement_record_target()) {
               throw std::invalid_argument("");
           }
           measurements.push_back(t);
        }
        k++;
        while (k < parts.size()) {
           if (parts[k] != "xor" || k + 1 == parts.size()) {
               throw std::invalid_argument("");
           }
           auto t = stim::GateTarget::from_target_str(parts[k + 1].c_str());
           if (!t.is_measurement_record_target()) {
               throw std::invalid_argument("");
           }
           measurements.push_back(t);
           k += 2;
        }
        return StabilizerFlow{input, output, measurements};
    } catch (const std::invalid_argument &ex) {
        throw std::invalid_argument("Invalid stabilizer flow text: '" + std::string(text) + "'.");
    }
}
bool StabilizerFlow::operator==(const StabilizerFlow &other) const {
     return input == other.input && output == other.output && measurement_outputs == other.measurement_outputs;
}
bool StabilizerFlow::operator!=(const StabilizerFlow &other) const {
     return !(*this == other);
}
std::string StabilizerFlow::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

std::ostream &stim::operator<<(std::ostream &out, const StabilizerFlow &flow) {
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
