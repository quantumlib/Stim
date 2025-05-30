#include "stim/circuit/circuit.h"
#include "stim/stabilizers/flex_pauli_string.h"
#include "stim/stabilizers/flow.h"
#include "stim/util_bot/arg_parse.h"
#include "stim/mem/span_ref.h"

namespace stim {

inline bool parse_obs_index(std::string_view obs, uint32_t *out) {
    if (obs.size() < 6 || obs[0] != 'o' || obs[1] != 'b' || obs[2] != 's' || obs[3] != '[' || obs.back() != ']') {
        throw std::invalid_argument("");  // Caught and given a message by caller.
    }
    int64_t i = 0;
    if (!parse_int64(obs.substr(4, obs.size() - 5), &i)) {
        return false;
    }

    if (i >= 0 && i <= UINT32_MAX) {
        *out = (uint32_t)i;
        return true;
    }
    return false;
}

inline bool parse_rec_allowing_non_negative(std::string_view rec, int32_t *out) {
    if (rec.size() < 6 || rec[0] != 'r' || rec[1] != 'e' || rec[2] != 'c' || rec[3] != '[' || rec.back() != ']') {
        throw std::invalid_argument("");  // Caught and given a message by caller.
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
        std::vector<uint32_t> observables;
        bool flip_out = false;
        if (parts[k].starts_with('-')) {
            flip_out = true;
            parts[k] = parts[k].substr(1);
        }
        if (!parts[k].empty() && parts[k][0] != 'r' && parts[k][0] != 'o') {
            out = parse_non_empty_pauli_string_allowing_i<W>(parts[k], &imag_out);
        } else if (parts[k][0] == 'r') {
            int32_t rec;
            if (!parse_rec_allowing_non_negative(parts[k], &rec)) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            measurements.push_back(rec);
        } else if (parts[k][0] == 'o') {
            uint32_t rec;
            if (!parse_obs_index(parts[k], &rec)) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            observables.push_back(rec);
        } else {
            throw std::invalid_argument("Parsing reached an expected-to-be-impossible state. "
                                        "Failed to understand a Pauli term.");
        }
        out.sign ^= flip_out;
        k++;
        while (k < parts.size()) {
            if (parts[k] != "xor" || k + 1 == parts.size()) {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            if (parts[k + 1].starts_with("r")) {
                int32_t rec;
                if (!parse_rec_allowing_non_negative(parts[k + 1], &rec)) {
                    throw std::invalid_argument("");  // Caught and given a message below.
                }
                measurements.push_back(rec);
            } else if (parts[k + 1].starts_with("o")) {
                uint32_t obs;
                if (!parse_obs_index(parts[k + 1], &obs)) {
                    throw std::invalid_argument("");  // Caught and given a message below.
                }
                observables.push_back(obs);
            } else {
                throw std::invalid_argument("");  // Caught and given a message below.
            }
            k += 2;
        }
        if (imag_inp != imag_out) {
            throw std::invalid_argument("Anti-Hermitian flows aren't allowed.");
        }
        size_t measurements_kept = inplace_xor_sort(SpanRef<int32_t>(measurements)).size();
        size_t obs_kept = inplace_xor_sort(SpanRef<uint32_t>(observables)).size();
        measurements.resize(measurements_kept);
        observables.resize(obs_kept);
        return Flow{inp, out, measurements, observables};
    } catch (const std::invalid_argument &ex) {
        if (*ex.what() != '\0') {
            throw;
        }
        throw std::invalid_argument("Invalid stabilizer flow text: '" + std::string(text) + "'.");
    }
}

template <size_t W>
bool Flow<W>::operator==(const Flow<W> &other) const {
    return input == other.input && output == other.output && measurements == other.measurements && observables == other.observables;
}

template <size_t W>
bool Flow<W>::operator<(const Flow<W> &other) const {
    if (input != other.input) {
        return input < other.input;
    }
    if (output != other.output) {
        return output < other.output;
    }
    if (measurements != other.measurements) {
        return SpanRef<const int32_t>(measurements) < SpanRef<const int32_t>(other.measurements);
    }
    if (observables != other.observables) {
        return SpanRef<const uint32_t>(observables) < SpanRef<const uint32_t>(other.observables);
    }
    return false;
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
        if (use_sparse) {
            ps.ref().for_each_active_pauli([&](size_t q) {
                uint8_t p = ps.xs[q] + 2 * ps.zs[q];
                if (has_any) {
                    out << "*";
                }
                out << "_XZY"[p];
                out << q;
                has_any = true;
            });
        } else {
            for (size_t q = 0; q < ps.num_qubits; q++) {
                uint8_t p = ps.xs[q] + 2 * ps.zs[q];
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
    for (const auto &t : flow.observables) {
        if (has_out) {
            out << " xor ";
        }
        has_out = true;
        out << "obs[" << t << "]";
    }
    if (!has_out) {
        out << "1";
    }
    return out;
}

template <size_t W>
void Flow<W>::canonicalize() {
    size_t measurements_kept = inplace_xor_sort(SpanRef<int32_t>(measurements)).size();
    size_t observables_kept = inplace_xor_sort(SpanRef<uint32_t>(observables)).size();
    measurements.resize(measurements_kept);
    observables.resize(observables_kept);
}

template <size_t W>
Flow<W> Flow<W>::operator*(const Flow<W> &rhs) const {
    Flow<W> result = *this;

    result.input.ensure_num_qubits(rhs.input.num_qubits, 1.1);
    result.output.ensure_num_qubits(rhs.output.num_qubits, 1.1);
    uint8_t log_i = 0;
    log_i -= result.input.ref().inplace_right_mul_returning_log_i_scalar(rhs.input.ref());
    log_i += result.output.ref().inplace_right_mul_returning_log_i_scalar(rhs.output.ref());
    if (log_i & 1) {
        throw std::invalid_argument(str() + " anticommutes with " + rhs.str());
    }
    if (log_i & 2) {
        result.output.sign ^= true;
    }

    result.measurements.insert(result.measurements.end(), rhs.measurements.begin(), rhs.measurements.end());
    result.observables.insert(result.observables.end(), rhs.observables.begin(), rhs.observables.end());
    result.canonicalize();
    return result;
}

}  // namespace stim
