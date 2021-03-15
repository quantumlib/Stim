// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "circuit.h"

#include <cctype>
#include <string>
#include <utility>

#include "../simulators/tableau_simulator.h"
#include "../stabilizers/tableau.h"
#include "gate_data.h"

enum READ_CONDITION {
    READ_AS_LITTLE_AS_POSSIBLE,
    READ_UNTIL_END_OF_BLOCK,
    READ_UNTIL_END_OF_FILE,
};

void fuse_data(PointerRange<uint32_t> &dst, PointerRange<uint32_t> src, MonotonicBuffer<uint32_t> &buf) {
    if (dst.ptr_end != src.ptr_start) {
        buf.ensure_available(src.size() + dst.size());
        dst = buf.take_copy(dst);
        src = buf.take_copy(src);
    }
    assert(dst.ptr_end == src.ptr_start);
    dst.ptr_end = src.ptr_end;
}

DetectorsAndObservables::DetectorsAndObservables(const Circuit &circuit) {
    size_t tick = 0;
    auto resolve_into = [&](const Operation &op, const std::function<void(uint32_t)> &func) {
        for (auto qb : op.target_data.targets) {
            auto dt = qb ^ TARGET_RECORD_BIT;
            if (!dt) {
                throw std::out_of_range("Record lookback can't be 0 (unspecified).");
            }
            if (dt > tick) {
                throw std::out_of_range("Referred to a measurement result before the beginning of time.");
            }
            func(tick - dt);
        }
    };

    for (const auto &p : circuit.operations) {
        if (p.gate->flags & GATE_PRODUCES_RESULTS) {
            tick += p.target_data.targets.size();
        } else if (p.gate->id == gate_name_to_id("DETECTOR")) {
            resolve_into(p, [&](uint32_t k){ jagged_detector_data.append_tail(k); });
            detectors.push_back(jagged_detector_data.commit_tail());
        } else if (p.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
            size_t obs = (size_t)p.target_data.arg;
            if (obs != p.target_data.arg) {
                throw std::out_of_range("Observable index must be an integer.");
            }
            while (observables.size() <= obs) {
                observables.emplace_back();
            }
            resolve_into(p, [&](uint32_t k){ observables[obs].push_back(k); });
        }
    }
}

Circuit::Circuit() : jag_targets(), operations(), num_qubits(0), num_measurements(0) {
}

Circuit::Circuit(const Circuit &circuit)
    : jag_targets(circuit.jag_targets.total_allocated()),
      operations(circuit.operations),
      num_qubits(circuit.num_qubits),
      num_measurements(circuit.num_measurements) {
    // Keep local copy of operation data.
    for (auto &op : operations) {
        op.target_data.targets = jag_targets.take_copy(op.target_data.targets);
    }
}

Circuit::Circuit(Circuit &&circuit) noexcept
    : jag_targets(circuit.jag_targets.total_allocated()),
      operations(std::move(circuit.operations)),
      num_qubits(circuit.num_qubits),
      num_measurements(circuit.num_measurements) {
    // Keep local copy of operation data.
    for (auto &op : operations) {
        op.target_data.targets = jag_targets.take_copy(op.target_data.targets);
    }
}

Circuit &Circuit::operator=(const Circuit &circuit) {
    if (&circuit != this) {
        num_qubits = circuit.num_qubits;
        num_measurements = circuit.num_measurements;
        operations = circuit.operations;

        // Keep local copy of operation data.
        jag_targets = MonotonicBuffer<uint32_t>(circuit.jag_targets.total_allocated());
        for (auto &op : operations) {
            op.target_data.targets = jag_targets.take_copy(op.target_data.targets);
        }
    }
    return *this;
}

Circuit &Circuit::operator=(Circuit &&circuit) noexcept {
    if (&circuit != this) {
        num_qubits = circuit.num_qubits;
        num_measurements = circuit.num_measurements;
        operations = std::move(circuit.operations);

        // Keep local copy of operation data.
        jag_targets = MonotonicBuffer<uint32_t>(circuit.jag_targets.total_allocated());
        for (auto &op : operations) {
            op.target_data.targets = jag_targets.take_copy(op.target_data.targets);
        }
    }
    return *this;
}

bool Operation::can_fuse(const Operation &other) const {
    return gate->id == other.gate->id && target_data.arg == other.target_data.arg &&
           !(gate->flags & GATE_IS_NOT_FUSABLE);
}

bool Operation::operator==(const Operation &other) const {
    return gate->id == other.gate->id && target_data == other.target_data;
}
bool Operation::approx_equals(const Operation &other, double atol) const {
    if (gate->id != other.gate->id || target_data.targets != other.target_data.targets) {
        return false;
    }
    return abs(target_data.arg - other.target_data.arg) <= atol;
}

bool Operation::operator!=(const Operation &other) const {
    return !(*this == other);
}

bool OperationData::operator==(const OperationData &other) const {
    return arg == other.arg && targets == other.targets;
}
bool OperationData::operator!=(const OperationData &other) const {
    return !(*this == other);
}

bool Circuit::operator==(const Circuit &other) const {
    return num_qubits == other.num_qubits && num_measurements == other.num_measurements &&
           operations == other.operations;
}
bool Circuit::operator!=(const Circuit &other) const {
    return !(*this == other);
}
bool Circuit::approx_equals(const Circuit &other, double atol) const {
    if (num_qubits != other.num_qubits || num_measurements != other.num_measurements ||
        operations.size() != other.operations.size()) {
        return false;
    }
    for (size_t k = 0; k < operations.size(); k++) {
        if (!operations[k].approx_equals(other.operations[k], atol)) {
            return false;
        }
    }
    return true;
}

template <typename SOURCE>
inline void read_past_within_line_whitespace(int &c, SOURCE read_char) {
    while (c == ' ' || c == '\t') {
        c = read_char();
    }
}

inline bool is_name_char(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

template <typename SOURCE>
inline const Gate &read_gate_name(int &c, SOURCE read_char) {
    char name_buf[32];
    size_t n = 0;
    while (is_name_char(c) && n < sizeof(name_buf)) {
        name_buf[n] = (char)c;
        c = read_char();
        n++;
    }
    // Note: in the name-too-long case, the full buffer name won't match any gate and an exception will fire.
    return GATE_DATA.at(name_buf, n);
}

inline bool is_double_char(int c) {
    return (c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-';
}

template <typename SOURCE>
double read_non_negative_double(int &c, SOURCE read_char) {
    char buf[64];
    size_t n = 0;
    while (n < sizeof(buf) - 1 && is_double_char(c)) {
        buf[n] = (char)c;
        c = read_char();
        n++;
    }
    buf[n] = '\0';

    char *end;
    double result = strtod(buf, &end);
    if (end != buf + n || !(result >= 0)) {
        throw std::out_of_range("Not a non-negative real number: " + std::string(buf));
    }
    return result;
}

template <typename SOURCE>
double read_parens_argument(int &c, const Gate &gate, SOURCE read_char) {
    if (c != '(') {
        throw std::out_of_range("Gate " + std::string(gate.name) + "(X) missing a parens argument.");
    }
    c = read_char();
    read_past_within_line_whitespace(c, read_char);
    double result = read_non_negative_double(c, read_char);
    read_past_within_line_whitespace(c, read_char);
    if (c != ')') {
        throw std::out_of_range("Gate " + std::string(gate.name) + "(X) missing a closing parens for its argument.");
    }
    c = read_char();
    return result;
}

template <typename SOURCE>
uint32_t read_uint24_t(int &c, SOURCE read_char) {
    if (!(c >= '0' && c <= '9')) {
        throw std::out_of_range("Expected a digit but got " + std::string(1, c));
    }
    uint32_t result = 0;
    do {
        result *= 10;
        result += c - '0';
        if (result >= uint32_t{1} << 24) {
            throw std::out_of_range("Number too large.");
        }
        c = read_char();
    } while (c >= '0' && c <= '9');
    return result;
}

template <typename SOURCE>
bool read_until_next_line_arg(int &c, SOURCE read_char) {
    if (c != ' ' && c != '#' && c != '\t' && c != '\n' && c != '{' && c != EOF) {
        throw std::out_of_range("Gate targets must be separated by spacing.");
    }
    while (c == ' ' || c == '\t') {
        c = read_char();
    }
    if (c == '#') {
        do {
            c = read_char();
        } while (c != '\n' && c != EOF);
    }
    return c != '\n' && c != '{' && c != EOF;
}

template <typename SOURCE>
inline void read_raw_qubit_target_into(int &c, SOURCE read_char, Circuit &circuit) {
    uint32_t q = read_uint24_t(c, read_char);
    circuit.jag_targets.append_tail(q);
    circuit.num_qubits = std::max(circuit.num_qubits, (size_t)q + 1);
}

template <typename SOURCE>
inline void read_record_target_into(int &c, SOURCE read_char, Circuit &circuit) {
    if (c != 'r' || read_char() != 'e' || read_char() != 'c' || read_char() != '[' || read_char() != '-') {
        throw std::out_of_range("Expected a record argument like 'rec[-1]'.");
    }
    c = read_char();
    uint32_t lookback = read_uint24_t(c, read_char);
    if (c != ']') {
        throw std::out_of_range("Expected a record argument like 'rec[-1]'.");
    }
    c = read_char();
    circuit.jag_targets.append_tail(lookback | TARGET_RECORD_BIT);
}

template <typename SOURCE>
inline void read_raw_qubit_targets_into(int &c, SOURCE read_char, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        read_raw_qubit_target_into(c, read_char, circuit);
    }
}

template <typename SOURCE>
inline void read_classically_controllable_qubit_targets_into(int &c, SOURCE read_char, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        if (c == 'r') {
            read_record_target_into(c, read_char, circuit);
        } else {
            read_raw_qubit_target_into(c, read_char, circuit);
        }
    }
}

template <typename SOURCE>
inline void read_pauli_targets_into(int &c, SOURCE read_char, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        uint32_t m = 0;
        if (c == 'X' || c == 'x') {
            m = TARGET_PAULI_X_BIT;
        } else if (c == 'Y' || c == 'y') {
            m = TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT;
        } else if (c == 'Z' || c == 'z') {
            m = TARGET_PAULI_Z_BIT;
        } else {
            throw std::out_of_range("Expected a Pauli target (like X1, Y2, or Z3) but got " + std::string(c, 1));
        }
        c = read_char();
        if (c == ' ') {
            throw std::out_of_range("Unexpected space after Pauli before target qubit index.");
        }
        size_t q = read_uint24_t(c, read_char);
        circuit.jag_targets.append_tail(q | m);
        circuit.num_qubits = std::max(circuit.num_qubits, (size_t)q + 1);
    }
}

template <typename SOURCE>
inline void read_result_targets_into(int &c, SOURCE read_char, const Gate &gate, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        uint32_t flipped_flag = c == '!' ? uint32_t{1} << 31 : 0;
        if (flipped_flag) {
            c = read_char();
        }
        uint32_t q = read_uint24_t(c, read_char);
        circuit.num_qubits = std::max(circuit.num_qubits, (size_t)q + 1);
        circuit.jag_targets.append_tail(q ^ flipped_flag);
        circuit.num_measurements++;
    }
}

template <typename SOURCE>
inline void read_record_targets_into(int &c, SOURCE read_char, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        read_record_target_into(c, read_char, circuit);
    }
}

template <typename SOURCE>
void read_past_dead_space_between_commands(int &c, SOURCE read_char) {
    while (true) {
        while (isspace(c)) {
            c = read_char();
        }
        if (c == EOF) {
            break;
        }
        if (c != '#') {
            break;
        }
        while (c != '\n' && c != EOF) {
            c = read_char();
        }
    }
}

template <typename SOURCE>
void circuit_read_single_operation(Circuit &circuit, char lead_char, SOURCE read_char) {
    int c = (int)lead_char;
    const auto &gate = read_gate_name(c, read_char);
    double val = 0;
    if (gate.flags & GATE_TAKES_PARENS_ARGUMENT) {
        read_past_within_line_whitespace(c, read_char);
        val = read_parens_argument(c, gate, read_char);
    }
    if (!(gate.flags & (GATE_IS_BLOCK | GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_PRODUCES_RESULTS |
                        GATE_TARGETS_PAULI_STRING | GATE_CAN_TARGET_MEASUREMENT_RECORD))) {
        read_raw_qubit_targets_into(c, read_char, circuit);
    } else if (gate.flags & GATE_ONLY_TARGETS_MEASUREMENT_RECORD) {
        read_record_targets_into(c, read_char, circuit);
    } else if (gate.flags & GATE_CAN_TARGET_MEASUREMENT_RECORD) {
        read_classically_controllable_qubit_targets_into(c, read_char, circuit);
    } else if (gate.flags & GATE_PRODUCES_RESULTS) {
        read_result_targets_into(c, read_char, gate, circuit);
    } else if (gate.flags & GATE_TARGETS_PAULI_STRING) {
        read_pauli_targets_into(c, read_char, circuit);
    } else {
        while (read_until_next_line_arg(c, read_char)) {
            circuit.jag_targets.append_tail(read_uint24_t(c, read_char));
        }
    }
    if (c != '{' && (gate.flags & GATE_IS_BLOCK)) {
        throw std::out_of_range("Missing '{' at start of " + std::string(gate.name) + " block.");
    }
    if (c == '{' && !(gate.flags & GATE_IS_BLOCK)) {
        throw std::out_of_range("Unexpected '{' after non-block command " + std::string(gate.name) + ".");
    }

    auto view = circuit.jag_targets.commit_tail();
    if (gate.flags & GATE_TARGETS_PAIRS) {
        if (view.size() & 1) {
            throw std::out_of_range(
                "Two qubit gate " + std::string(gate.name) + " applied to an odd number of targets.");
        }
        for (size_t k = 0; k < view.size(); k += 2) {
            if (view[k] == view[k + 1]) {
                throw std::out_of_range(
                    "Interacting a target with itself " + std::to_string(view[k] & TARGET_VALUE_MASK) + " using gate " +
                    gate.name + ".");
            }
        }
    }
    circuit.operations.push_back({&gate, {val, view}});
}

template <typename SOURCE>
void circuit_read_operations(Circuit &circuit, SOURCE read_char, READ_CONDITION read_condition) {
    auto &ops = circuit.operations;
    do {
        int c = read_char();
        read_past_dead_space_between_commands(c, read_char);
        if (c == EOF) {
            if (read_condition == READ_UNTIL_END_OF_BLOCK) {
                throw std::out_of_range("Unterminated block. Got a '{' without an eventual '}'.");
            }
            return;
        }
        if (c == '}') {
            if (read_condition != READ_UNTIL_END_OF_BLOCK) {
                throw std::out_of_range("Uninitiated block. Got a '}' without a '{'.");
            }
            return;
        }
        size_t s = ops.size();
        circuit_read_single_operation(circuit, c, read_char);

        if (ops[s].gate->id == gate_name_to_id("REPEAT")) {
            if (ops[s].target_data.targets.size() != 1) {
                throw std::out_of_range("Invalid instruction. Expected one repetition arg like `REPEAT 100 {`.");
            }
            size_t rep_count = ops[s].target_data.targets[0];
            ops.pop_back();
            if (rep_count == 0) {
                throw std::out_of_range("Repeating 0 times is not supported.");
            }
            size_t ops_start = ops.size();
            size_t num_measure_start = circuit.num_measurements;
            circuit.fusion_barrier();
            circuit_read_operations(circuit, read_char, READ_UNTIL_END_OF_BLOCK);
            size_t ops_end = ops.size();
            circuit.num_measurements += (circuit.num_measurements - num_measure_start) * (rep_count - 1);
            while (rep_count > 1) {
                ops.insert(ops.end(), ops.data() + ops_start, ops.data() + ops_end);
                rep_count--;
            }
            circuit.fusion_barrier();
        }

        // Fuse operations.
        while (s > circuit.min_safe_fusion_index && ops[s - 1].can_fuse(ops[s])) {
            fuse_data(ops[s - 1].target_data.targets, ops[s].target_data.targets, circuit.jag_targets);
            ops.pop_back();
            s--;
        }
    } while (read_condition != READ_AS_LITTLE_AS_POSSIBLE);
}

bool Circuit::append_from_text(const char *text) {
    size_t before = operations.size();
    size_t k = 0;
    circuit_read_operations(
        *this,
        [&]() {
            return text[k] != 0 ? text[k++] : EOF;
        },
        READ_UNTIL_END_OF_FILE);
    return operations.size() > before;
}

void Circuit::update_metadata_for_manually_appended_operation() {
    const auto &op = operations.back();
    const auto &gate = *op.gate;
    const auto &vec = op.target_data.targets;
    if (gate.flags & GATE_PRODUCES_RESULTS) {
        num_measurements += vec.size();
    }
    for (auto q : vec) {
        if (!(q & TARGET_RECORD_BIT)) {
            num_qubits = std::max(num_qubits, (size_t)((q & TARGET_VALUE_MASK) + 1));
        }
    }
}

void Circuit::append_circuit(const Circuit &circuit, size_t repetitions) {
    if (!repetitions) {
        return;
    }
    auto original_size = operations.size();

    if (&circuit == this) {
        num_measurements *= repetitions + 1;
        do {
            operations.insert(operations.end(), operations.begin(), operations.begin() + original_size);
        } while (--repetitions);
        return;
    }

    fusion_barrier();
    for (const auto &op : circuit.operations) {
        append_operation(op);
    }
    auto single_rep_end = operations.end();
    while (--repetitions) {
        operations.insert(operations.end(), operations.begin() + original_size, single_rep_end);
    }
    fusion_barrier();
}

void Circuit::append_operation(const Operation &operation) {
    operations.push_back(
        {operation.gate, {operation.target_data.arg, jag_targets.take_copy(operation.target_data.targets)}});
    update_metadata_for_manually_appended_operation();
}

void Circuit::append_op(const std::string &gate_name, const std::vector<uint32_t> &vec, double arg) {
    const auto &gate = GATE_DATA.at(gate_name);
    append_operation(gate, vec, arg);
}

void Circuit::append_operation(
    const Gate &gate, ConstPointerRange<uint32_t> targets, double arg) {
    if (gate.flags & GATE_TARGETS_PAIRS) {
        if (targets.size() & 1) {
            throw std::out_of_range(
                "Two qubit gate " + std::string(gate.name) + " requires have an even number of targets.");
        }
        for (size_t k = 0; k < targets.size(); k += 2) {
            if (targets[k] == targets[k + 1]) {
                throw std::out_of_range(
                    "Interacting a target with itself " + std::to_string(targets[k] & TARGET_VALUE_MASK) +
                    " using gate " + std::string(gate.name) + ".");
            }
        }
    }
    if (arg != 0 && !(gate.flags & GATE_TAKES_PARENS_ARGUMENT)) {
        throw std::out_of_range("Gate " + std::string(gate.name) + " doesn't take a parens arg.");
    }

    // Check that targets are in range.
    uint32_t valid_target_mask = TARGET_VALUE_MASK;
    if (gate.flags & GATE_PRODUCES_RESULTS) {
        valid_target_mask |= TARGET_INVERTED_BIT;
    }
    if (gate.flags & GATE_TARGETS_PAULI_STRING) {
        valid_target_mask |= TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT;
    }
    if (gate.flags & (GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_CAN_TARGET_MEASUREMENT_RECORD)) {
        valid_target_mask |= TARGET_RECORD_BIT;
    }
    for (uint32_t q : targets) {
        if (q != (q & valid_target_mask)) {
            throw std::out_of_range(
                "Target " + std::to_string(q & TARGET_VALUE_MASK) + " has invalid flags " +
                std::to_string(q & ~TARGET_VALUE_MASK) + " for gate " + std::string(gate.name) + ".");
        }
    }

    auto added = jag_targets.take_copy(targets);
    if (!(gate.flags & GATE_IS_NOT_FUSABLE) && operations.size() > min_safe_fusion_index &&
        operations.back().gate->id == gate.id && operations.back().target_data.arg == arg) {
        // Don't double count measurements when doing incremental update.
        if (gate.flags & GATE_PRODUCES_RESULTS) {
            num_measurements -= operations.back().target_data.targets.size();
        }
        // Extend targets of last gate.
        fuse_data(operations.back().target_data.targets, added, jag_targets);
    } else {
        // Add a fresh new operation with its own target data.
        operations.push_back({&gate, {arg, added}});
    }
    // Update num_measurements and num_qubits appropriately.
    update_metadata_for_manually_appended_operation();
}

bool Circuit::append_from_file(FILE *file, bool stop_asap) {
    size_t before = operations.size();
    circuit_read_operations(
        *this,
        [&]() {
            return getc(file);
        },
        stop_asap ? READ_AS_LITTLE_AS_POSSIBLE : READ_UNTIL_END_OF_FILE);
    return operations.size() > before;
}

std::ostream &operator<<(std::ostream &out, const Operation &op) {
    out << op.gate->name;
    if (op.target_data.arg != 0 || (op.gate->flags & GATE_TAKES_PARENS_ARGUMENT)) {
        out << '(';
        if ((size_t)op.target_data.arg == op.target_data.arg) {
            out << (size_t)op.target_data.arg;
        } else {
            out << op.target_data.arg;
        }
        out << ')';
    }
    for (auto t : op.target_data.targets) {
        out << ' ';
        if (t & TARGET_INVERTED_BIT) {
            out << '!';
        }
        if (t & (TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT)) {
            bool x = t & TARGET_PAULI_X_BIT;
            bool z = t & TARGET_PAULI_Z_BIT;
            out << "IXZY"[x + z * 2];
        }
        if (t & TARGET_RECORD_BIT) {
            out << "rec[-" << (t & TARGET_VALUE_MASK) << "]";
        } else {
            out << (t & TARGET_VALUE_MASK);
        }
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const Circuit &c) {
    bool first = true;
    for (const auto &op : c.operations) {
        if (first) {
            first = false;
        } else {
            out << "\n";
        }
        out << op;
    }
    return out;
}

void Circuit::clear() {
    num_qubits = 0;
    num_measurements = 0;
    jag_targets.clear();
    operations.clear();
    min_safe_fusion_index = 0;
}

Circuit Circuit::operator+(const Circuit &other) const {
    Circuit result = *this;
    result += other;
    return result;
}
Circuit Circuit::operator*(size_t repetitions) const {
    Circuit result = *this;
    result *= repetitions;
    return result;
}

void Circuit::fusion_barrier() {
    min_safe_fusion_index = operations.size();
}

Circuit &Circuit::operator+=(const Circuit &other) {
    append_circuit(other, 1);
    return *this;
}
Circuit &Circuit::operator*=(size_t repetitions) {
    if (repetitions == 0) {
        clear();
    } else {
        append_circuit(*this, repetitions - 1);
    }
    return *this;
}

std::string Circuit::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

std::string Operation::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

Circuit Circuit::from_file(FILE *file) {
    Circuit result;
    result.append_from_file(file, false);
    return result;
}

Circuit Circuit::from_text(const char *text) {
    Circuit result;
    result.append_from_text(text);
    return result;
}

DetectorsAndObservables::DetectorsAndObservables(DetectorsAndObservables &&other) noexcept
    : jagged_detector_data(other.jagged_detector_data.total_allocated()),
      detectors(std::move(other.detectors)),
      observables(std::move(other.observables)) {
    // Keep a local copy of the detector data.
    for (PointerRange<uint32_t> &e : detectors) {
        e = jagged_detector_data.take_copy(e);
    }
}

DetectorsAndObservables &DetectorsAndObservables::operator=(DetectorsAndObservables &&other) noexcept {
    observables = std::move(other.observables);
    detectors = std::move(other.detectors);

    // Keep a local copy of the detector data.
    jagged_detector_data = MonotonicBuffer<uint32_t>(other.jagged_detector_data.total_allocated());
    for (PointerRange<uint32_t> &e : detectors) {
        e = jagged_detector_data.take_copy(e);
    }

    return *this;
}

DetectorsAndObservables::DetectorsAndObservables(const DetectorsAndObservables &other)
    : jagged_detector_data(other.jagged_detector_data.total_allocated()),
      detectors(other.detectors),
      observables(other.observables) {

    // Keep a local copy of the detector data.
    for (PointerRange<uint32_t> &e : detectors) {
        e = jagged_detector_data.take_copy(e);
    }
}

DetectorsAndObservables &DetectorsAndObservables::operator=(const DetectorsAndObservables &other) {
    if (this == &other) {
        return *this;
    }

    observables = other.observables;
    detectors = other.detectors;

    // Keep a local copy of the detector data.
    jagged_detector_data = MonotonicBuffer<uint32_t>(other.jagged_detector_data.total_allocated());
    for (PointerRange<uint32_t> &e : detectors) {
        e = jagged_detector_data.take_copy(e);
    }

    return *this;
}
