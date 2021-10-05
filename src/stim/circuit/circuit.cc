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

#include "stim/circuit/circuit.h"

#include <cctype>
#include <string>
#include <utility>

#include "stim/circuit/gate_data.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/tableau.h"
#include "stim/str_util.h"

using namespace stim;

enum READ_CONDITION {
    READ_AS_LITTLE_AS_POSSIBLE,
    READ_UNTIL_END_OF_BLOCK,
    READ_UNTIL_END_OF_FILE,
};

/// Concatenates the second pointer range's data into the first.
/// Typically, the two ranges are contiguous and so this only requires advancing the end of the destination region.
/// In cases where that doesn't occur, space is created in the given monotonic buffer to store the result and both
/// the start and end of the destination range move.
void fuse_data(
    ConstPointerRange<GateTarget> &dst, ConstPointerRange<GateTarget> src, MonotonicBuffer<GateTarget> &buf) {
    if (dst.ptr_end != src.ptr_start) {
        buf.ensure_available(src.size() + dst.size());
        dst = buf.take_copy(dst);
        src = buf.take_copy(src);
    }
    assert(dst.ptr_end == src.ptr_start);
    dst.ptr_end = src.ptr_end;
}

void write_target(std::ostream &out, GateTarget t) {
    if (t.data == TARGET_COMBINER) {
        out << "*";
        return;
    }
    if (t.data & TARGET_INVERTED_BIT) {
        out << '!';
    }
    if (t.data & (TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT)) {
        bool x = t.data & TARGET_PAULI_X_BIT;
        bool z = t.data & TARGET_PAULI_Z_BIT;
        out << "IXZY"[x + z * 2];
    }
    if (t.data & TARGET_RECORD_BIT) {
        out << "rec[-" << (t.data & TARGET_VALUE_MASK) << "]";
    } else if (t.data & TARGET_SWEEP_BIT) {
        out << "sweep[" << (t.data & TARGET_VALUE_MASK) << "]";
    } else {
        out << (t.data & TARGET_VALUE_MASK);
    }
}

std::string target_str(GateTarget t) {
    std::stringstream out;
    write_target(out, t);
    return out.str();
}

void write_targets(std::ostream &out, ConstPointerRange<GateTarget> targets) {
    bool skip_space = false;
    for (auto t : targets) {
        if (t.is_combiner()) {
            skip_space = true;
        } else if (!skip_space) {
            out << ' ';
        } else {
            skip_space = false;
        }
        write_target(out, t);
    }
}

std::string targets_str(ConstPointerRange<GateTarget> targets) {
    std::stringstream out;
    write_targets(out, targets);
    return out.str();
}

uint64_t stim::op_data_rep_count(const OperationData &data) {
    uint64_t low = data.targets[1].data;
    uint64_t high = data.targets[2].data;
    return low | (high << 32);
}

void validate_gate(const Gate &gate, ConstPointerRange<GateTarget> targets, ConstPointerRange<double> args) {
    if (gate.flags & GATE_TARGETS_PAIRS) {
        if (targets.size() & 1) {
            throw std::invalid_argument(
                "Two qubit gate " + std::string(gate.name) +
                " requires an even number of targets but was given "
                "(" +
                comma_sep(args).str() + ").");
        }
        for (size_t k = 0; k < targets.size(); k += 2) {
            if (targets[k] == targets[k + 1]) {
                throw std::invalid_argument(
                    "The two qubit gate " + std::string(gate.name) +
                    " was applied to a target pair with the same target (" + target_str(targets[k]) +
                    ") twice. Gates can't interact targets with themselves.");
            }
        }
    }

    if (gate.arg_count == ARG_COUNT_SYGIL_ZERO_OR_ONE) {
        if (args.size() > 1) {
            throw std::invalid_argument(
                "Gate " + std::string(gate.name) + " was given " + std::to_string(args.size()) + " arguments (" +
                comma_sep(args).str() + ") but takes 0 or 1 arguments.");
        }
    } else if (args.size() != gate.arg_count && gate.arg_count != ARG_COUNT_SYGIL_ANY) {
        throw std::invalid_argument(
            "Gate " + std::string(gate.name) + " was given " + std::to_string(args.size()) + " arguments (" +
            comma_sep(args).str() + ") but takes " + std::to_string(gate.arg_count) + " arguments.");
    }

    if ((gate.flags & GATE_TAKES_NO_TARGETS) && !targets.empty()) {
        throw std::invalid_argument(
            "Gate " + std::string(gate.name) + " takes no targets but was given targets" + targets_str(targets) + ".");
    }

    if (gate.flags & GATE_ARGS_ARE_DISJOINT_PROBABILITIES) {
        double total = 0;
        for (const auto p : args) {
            if (p < 0 || p > 1) {
                throw std::invalid_argument(
                    "Gate " + std::string(gate.name) + " only takes probability arguments, but one of its arguments (" +
                    comma_sep(args).str() + ") wasn't a probability.");
            }
            total += p;
        }
        if (total > 1.0000001) {
            throw std::invalid_argument(
                "The disjoint probability arguments (" + comma_sep(args).str() + ") given to gate " +
                std::string(gate.name) + " sum to more than 1.");
        }
    } else if (gate.flags & GATE_ARGS_ARE_UNSIGNED_INTEGERS) {
        for (const auto p : args) {
            if (p < 0 || p != round(p)) {
                throw std::invalid_argument(
                    "Gate " + std::string(gate.name) +
                    " only takes non-negative integer arguments, but one of its arguments (" + comma_sep(args).str() +
                    ") wasn't a non-negative integer.");
            }
        }
    }

    uint32_t valid_target_mask = TARGET_VALUE_MASK;

    // Check combiners.
    if (gate.flags & GATE_TARGETS_COMBINERS) {
        bool combiner_allowed = false;
        bool just_saw_combiner = false;
        bool failed = false;
        for (const auto p : targets) {
            if (p.is_combiner()) {
                failed |= !combiner_allowed;
                combiner_allowed = false;
                just_saw_combiner = true;
            } else {
                combiner_allowed = true;
                just_saw_combiner = false;
            }
        }
        failed |= just_saw_combiner;
        if (failed) {
            throw std::invalid_argument(
                "Gate " + std::string(gate.name) +
                " given combiners ('*') that aren't between other targets: " + targets_str(targets) + ".");
        }
        valid_target_mask |= TARGET_COMBINER;
    }

    // Check that targets are in range.
    if (gate.flags & GATE_PRODUCES_NOISY_RESULTS) {
        valid_target_mask |= TARGET_INVERTED_BIT;
    }
    if (gate.flags & GATE_CAN_TARGET_BITS) {
        valid_target_mask |= TARGET_RECORD_BIT | TARGET_SWEEP_BIT;
    }
    if (gate.flags & GATE_ONLY_TARGETS_MEASUREMENT_RECORD) {
        for (GateTarget q : targets) {
            if (!(q.data & TARGET_RECORD_BIT)) {
                throw std::invalid_argument("Gate " + std::string(gate.name) + " only takes rec[-k] targets.");
            }
        }
    } else if (gate.flags & GATE_TARGETS_PAULI_STRING) {
        for (GateTarget q : targets) {
            if (!(q.data & (TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT | TARGET_COMBINER))) {
                throw std::invalid_argument(
                    "Gate " + std::string(gate.name) + " only takes Pauli targets ('X2', 'Y3', 'Z5', etc).");
            }
        }
    } else {
        for (GateTarget q : targets) {
            if (q.data != (q.data & valid_target_mask)) {
                std::stringstream ss;
                ss << "Target ";
                write_target(ss, q);
                ss << " has invalid modifiers for gate type '" << gate.name << "'.";
                throw std::invalid_argument(ss.str());
            }
        }
    }
}

DetectorsAndObservables::DetectorsAndObservables(const Circuit &circuit) {
    uint64_t tick = 0;
    auto resolve_into = [&](const Operation &op, const std::function<void(uint64_t)> &func) {
        for (auto qb : op.target_data.targets) {
            uint64_t dt = qb.data ^ TARGET_RECORD_BIT;
            if (!dt) {
                throw std::invalid_argument("Record lookback can't be 0 (unspecified).");
            }
            if (dt > tick) {
                throw std::invalid_argument("Referred to a measurement result before the beginning of time.");
            }
            func(tick - dt);
        }
    };

    circuit.for_each_operation([&](const Operation &p) {
        if (p.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
            tick += p.count_measurement_results();
        } else if (p.gate->id == gate_name_to_id("DETECTOR")) {
            resolve_into(p, [&](uint64_t k) {
                jagged_detector_data.append_tail(k);
            });
            detectors.push_back(jagged_detector_data.commit_tail());
        } else if (p.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
            size_t obs = (size_t)p.target_data.args[0];
            if (obs != p.target_data.args[0]) {
                throw std::invalid_argument("Observable index must be an integer.");
            }
            while (observables.size() <= obs) {
                observables.emplace_back();
            }
            resolve_into(p, [&](uint32_t k) {
                observables[obs].push_back(k);
            });
        }
    });
}

uint64_t Operation::count_measurement_results() const {
    if (!(gate->flags & GATE_PRODUCES_NOISY_RESULTS)) {
        return 0;
    }
    uint64_t n = (uint64_t)target_data.targets.size();
    if (gate->flags & GATE_TARGETS_COMBINERS) {
        for (auto e : target_data.targets) {
            if (e.is_combiner()) {
                n -= 2;
            }
        }
    }
    return n;
}

Circuit::Circuit() : target_buf(), operations(), blocks() {
}

Circuit::Circuit(const Circuit &circuit)
    : target_buf(circuit.target_buf.total_allocated()),
      arg_buf(circuit.arg_buf.total_allocated()),
      operations(circuit.operations),
      blocks(circuit.blocks) {
    // Keep local copy of operation data.
    for (auto &op : operations) {
        op.target_data.targets = target_buf.take_copy(op.target_data.targets);
    }
    for (auto &op : operations) {
        op.target_data.args = arg_buf.take_copy(op.target_data.args);
    }
}

Circuit::Circuit(Circuit &&circuit) noexcept
    : target_buf(std::move(circuit.target_buf)),
      arg_buf(std::move(circuit.arg_buf)),
      operations(std::move(circuit.operations)),
      blocks(std::move(circuit.blocks)) {
}

Circuit &Circuit::operator=(const Circuit &circuit) {
    if (&circuit != this) {
        blocks = circuit.blocks;

        // Keep local copy of operation data.
        target_buf = MonotonicBuffer<GateTarget>(circuit.target_buf.total_allocated());
        for (auto &op : operations) {
            op.target_data.targets = target_buf.take_copy(op.target_data.targets);
        }
        arg_buf = MonotonicBuffer<double>(circuit.arg_buf.total_allocated());
        for (auto &op : operations) {
            op.target_data.args = arg_buf.take_copy(op.target_data.args);
        }
    }
    return *this;
}

Circuit &Circuit::operator=(Circuit &&circuit) noexcept {
    if (&circuit != this) {
        operations = std::move(circuit.operations);
        blocks = std::move(circuit.blocks);
        target_buf = std::move(circuit.target_buf);
        arg_buf = std::move(circuit.arg_buf);
    }
    return *this;
}

bool Operation::can_fuse(const Operation &other) const {
    return gate->id == other.gate->id && target_data.args == other.target_data.args &&
           !(gate->flags & GATE_IS_NOT_FUSABLE);
}

bool Operation::operator==(const Operation &other) const {
    return gate->id == other.gate->id && target_data == other.target_data;
}
bool Operation::approx_equals(const Operation &other, double atol) const {
    if (gate->id != other.gate->id || target_data.targets != other.target_data.targets ||
        target_data.args.size() != other.target_data.args.size()) {
        return false;
    }
    for (size_t k = 0; k < target_data.args.size(); k++) {
        if (abs(target_data.args[k] - other.target_data.args[k]) > atol) {
            return false;
        }
    }
    return true;
}

bool Operation::operator!=(const Operation &other) const {
    return !(*this == other);
}

bool OperationData::operator==(const OperationData &other) const {
    return args == other.args && targets == other.targets;
}
bool OperationData::operator!=(const OperationData &other) const {
    return !(*this == other);
}

bool Circuit::operator==(const Circuit &other) const {
    return operations == other.operations && blocks == other.blocks;
}
bool Circuit::operator!=(const Circuit &other) const {
    return !(*this == other);
}
bool Circuit::approx_equals(const Circuit &other, double atol) const {
    if (operations.size() != other.operations.size() || blocks.size() != other.blocks.size()) {
        return false;
    }
    for (size_t k = 0; k < operations.size(); k++) {
        if (!operations[k].approx_equals(other.operations[k], atol)) {
            return false;
        }
    }
    for (size_t k = 0; k < blocks.size(); k++) {
        if (!blocks[k].approx_equals(other.blocks[k], atol)) {
            return false;
        }
    }
    return true;
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
    try {
        return GATE_DATA.at(name_buf, n);
    } catch (const std::out_of_range &ex) {
        throw std::invalid_argument(ex.what());
    }
}

template <typename SOURCE>
uint32_t read_uint24_t(int &c, SOURCE read_char) {
    if (!(c >= '0' && c <= '9')) {
        throw std::invalid_argument("Expected a digit but got '" + std::string(1, c) + "'");
    }
    uint32_t result = 0;
    do {
        result *= 10;
        result += c - '0';
        if (result >= uint32_t{1} << 24) {
            throw std::invalid_argument("Number too large.");
        }
        c = read_char();
    } while (c >= '0' && c <= '9');
    return result;
}

template <typename SOURCE>
uint64_t read_uint63_t(int &c, SOURCE read_char) {
    if (!(c >= '0' && c <= '9')) {
        throw std::invalid_argument("Expected a digit but got '" + std::string(1, c) + "'");
    }
    uint64_t result = 0;
    do {
        result *= 10;
        result += c - '0';
        if (result >= uint64_t{1} << 63) {
            throw std::invalid_argument("Number too large.");
        }
        c = read_char();
    } while (c >= '0' && c <= '9');
    return result;
}

template <typename SOURCE>
inline void read_raw_qubit_target_into(int &c, SOURCE read_char, Circuit &circuit) {
    circuit.target_buf.append_tail(GateTarget::qubit(read_uint24_t(c, read_char)));
}

template <typename SOURCE>
inline void read_measurement_record_target_into(int &c, SOURCE read_char, Circuit &circuit) {
    if (c != 'r' || read_char() != 'e' || read_char() != 'c' || read_char() != '[' || read_char() != '-') {
        throw std::invalid_argument("Target started with 'r' but wasn't a record argument like 'rec[-1]'.");
    }
    c = read_char();
    uint32_t lookback = read_uint24_t(c, read_char);
    if (c != ']') {
        throw std::invalid_argument("Target started with 'r' but wasn't a record argument like 'rec[-1]'.");
    }
    c = read_char();
    circuit.target_buf.append_tail({lookback | TARGET_RECORD_BIT});
}

template <typename SOURCE>
inline void read_sweep_bit_target_into(int &c, SOURCE read_char, Circuit &circuit) {
    if (c != 's' || read_char() != 'w' || read_char() != 'e' || read_char() != 'e' || read_char() != 'p' ||
        read_char() != '[') {
        throw std::invalid_argument("Target started with 's' but wasn't a sweep bit argument like 'sweep[5]'.");
    }
    c = read_char();
    uint32_t lookback = read_uint24_t(c, read_char);
    if (c != ']') {
        throw std::invalid_argument("Target started with 's' but wasn't a sweep bit argument like 'sweep[5]'.");
    }
    c = read_char();
    circuit.target_buf.append_tail({lookback | TARGET_SWEEP_BIT});
}

template <typename SOURCE>
inline void read_pauli_target_into(int &c, SOURCE read_char, Circuit &circuit) {
    uint32_t m = 0;
    if (c == 'x' || c == 'X') {
        m = TARGET_PAULI_X_BIT;
    } else if (c == 'y' || c == 'Y') {
        m = TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT;
    } else if (c == 'z' || c == 'Z') {
        m = TARGET_PAULI_Z_BIT;
    } else {
        assert(false);
    }
    c = read_char();
    if (c == ' ') {
        throw std::invalid_argument(
            "Pauli target '" + std::string(1, c) + "' followed by a space instead of a qubit index.");
    }
    uint32_t q = read_uint24_t(c, read_char);

    circuit.target_buf.append_tail({q | m});
}

template <typename SOURCE>
inline void read_inverted_target_into(int &c, SOURCE read_char, Circuit &circuit) {
    assert(c == '!');
    c = read_char();
    if (c == 'X' || c == 'x' || c == 'Y' || c == 'y' || c == 'Z' || c == 'z') {
        read_pauli_target_into(c, read_char, circuit);
    } else {
        read_raw_qubit_target_into(c, read_char, circuit);
    }
    circuit.target_buf.tail.back().data ^= TARGET_INVERTED_BIT;
}

template <typename SOURCE>
inline void read_arbitrary_targets_into(int &c, SOURCE read_char, Circuit &circuit) {
    bool need_space = true;
    while (read_until_next_line_arg(c, read_char, need_space)) {
        need_space = true;
        switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                read_raw_qubit_target_into(c, read_char, circuit);
                break;
            case 'r':
                read_measurement_record_target_into(c, read_char, circuit);
                break;
            case '!':
                read_inverted_target_into(c, read_char, circuit);
                break;
            case 'X':
            case 'Y':
            case 'Z':
            case 'x':
            case 'y':
            case 'z':
                read_pauli_target_into(c, read_char, circuit);
                break;
            case '*':
                circuit.target_buf.append_tail(GateTarget::combiner());
                c = read_char();
                need_space = false;
                break;
            case 's':
                read_sweep_bit_target_into(c, read_char, circuit);
                break;
            default:
                throw std::invalid_argument("Unrecognized target prefix '" + std::string(1, c) + "'.");
        }
    }
}

template <typename SOURCE>
inline void read_result_targets64_into(int &c, SOURCE read_char, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        uint64_t q = read_uint63_t(c, read_char);
        circuit.target_buf.append_tail({(uint32_t)(q & 0xFFFFFFFFULL)});
        circuit.target_buf.append_tail({(uint32_t)(q >> 32)});
    }
}

template <typename SOURCE>
void circuit_read_single_operation(Circuit &circuit, char lead_char, SOURCE read_char) {
    int c = (int)lead_char;
    const auto &gate = read_gate_name(c, read_char);
    try {
        read_parens_arguments(c, gate.name, read_char, circuit.arg_buf);
        if (gate.flags & GATE_IS_BLOCK) {
            read_result_targets64_into(c, read_char, circuit);
            if (c != '{') {
                throw std::invalid_argument("Missing '{' at start of " + std::string(gate.name) + " block.");
            }
        } else {
            read_arbitrary_targets_into(c, read_char, circuit);
            if (c == '{') {
                throw std::invalid_argument("Unexpected '{'.");
            }
            validate_gate(gate, circuit.target_buf.tail, circuit.arg_buf.tail);
        }
    } catch (const std::invalid_argument &ex) {
        circuit.target_buf.discard_tail();
        circuit.arg_buf.discard_tail();
        throw ex;
    }

    circuit.operations.push_back({&gate, {circuit.arg_buf.commit_tail(), circuit.target_buf.commit_tail()}});
}

template <typename SOURCE>
void circuit_read_operations(Circuit &circuit, SOURCE read_char, READ_CONDITION read_condition) {
    auto &ops = circuit.operations;
    do {
        int c = read_char();
        read_past_dead_space_between_commands(c, read_char);
        if (c == EOF) {
            if (read_condition == READ_UNTIL_END_OF_BLOCK) {
                throw std::invalid_argument("Unterminated block. Got a '{' without an eventual '}'.");
            }
            return;
        }
        if (c == '}') {
            if (read_condition != READ_UNTIL_END_OF_BLOCK) {
                throw std::invalid_argument("Uninitiated block. Got a '}' without a '{'.");
            }
            return;
        }
        circuit_read_single_operation(circuit, c, read_char);
        Operation &new_op = ops.back();

        if (new_op.gate->id == gate_name_to_id("REPEAT")) {
            if (new_op.target_data.targets.size() != 2) {
                throw std::invalid_argument("Invalid instruction. Expected one repetition arg like `REPEAT 100 {`.");
            }
            uint32_t rep_count_low = new_op.target_data.targets[0].data;
            uint32_t rep_count_high = new_op.target_data.targets[1].data;
            uint32_t block_id = (uint32_t)circuit.blocks.size();
            if (rep_count_low == 0 && rep_count_high == 0) {
                throw std::invalid_argument("Repeating 0 times is not supported.");
            }

            // Read block.
            circuit.blocks.emplace_back();
            circuit_read_operations(circuit.blocks.back(), read_char, READ_UNTIL_END_OF_BLOCK);

            // Rewrite target data to reference the parsed block.
            circuit.target_buf.ensure_available(3);
            circuit.target_buf.append_tail({block_id});
            circuit.target_buf.append_tail({rep_count_low});
            circuit.target_buf.append_tail({rep_count_high});
            new_op.target_data.targets = circuit.target_buf.commit_tail();
        }

        // Fuse operations.
        while (ops.size() > 1 && ops[ops.size() - 2].can_fuse(new_op)) {
            fuse_data(ops[ops.size() - 2].target_data.targets, new_op.target_data.targets, circuit.target_buf);
            ops.pop_back();
        }
    } while (read_condition != READ_AS_LITTLE_AS_POSSIBLE);
}

void Circuit::append_from_text(const char *text) {
    size_t k = 0;
    circuit_read_operations(
        *this,
        [&]() {
            return text[k] != 0 ? text[k++] : EOF;
        },
        READ_UNTIL_END_OF_FILE);
}

PointerRange<stim::GateTarget> Circuit::append_operation(const Operation &operation) {
    PointerRange<stim::GateTarget> target_data = target_buf.take_copy(operation.target_data.targets);
    OperationData op_data{arg_buf.take_copy(operation.target_data.args), target_data};
    operations.push_back({operation.gate, op_data});
    return target_data;
}

void Circuit::append_op(const std::string &gate_name, const std::vector<uint32_t> &targets, double singleton_arg) {
    const auto &gate = GATE_DATA.at(gate_name);

    std::vector<GateTarget> converted;
    converted.reserve(targets.size());
    for (auto e : targets) {
        converted.push_back({e});
    }

    append_operation(gate, converted, {&singleton_arg});
}

void Circuit::append_op(
    const std::string &gate_name, const std::vector<uint32_t> &targets, const std::vector<double> &args) {
    const auto &gate = GATE_DATA.at(gate_name);

    std::vector<GateTarget> converted;
    converted.reserve(targets.size());
    for (auto e : targets) {
        converted.push_back({e});
    }

    append_operation(gate, converted, args);
}

void Circuit::append_operation(
    const Gate &gate, ConstPointerRange<GateTarget> targets, ConstPointerRange<double> args) {
    if (gate.flags & GATE_IS_BLOCK) {
        throw std::invalid_argument("Can't append a block like a normal operation.");
    }
    validate_gate(gate, targets, args);

    auto added_args = arg_buf.take_copy(args);
    auto added_targets = target_buf.take_copy(targets);
    Operation to_add = {&gate, {added_args, added_targets}};
    if (!operations.empty() && operations.back().can_fuse(to_add)) {
        // Extend targets of last gate.
        fuse_data(operations.back().target_data.targets, to_add.target_data.targets, target_buf);
    } else {
        // Add a fresh new operation with its own target data.
        operations.push_back(to_add);
    }
}

void Circuit::append_from_file(FILE *file, bool stop_asap) {
    circuit_read_operations(
        *this,
        [&]() {
            return getc(file);
        },
        stop_asap ? READ_AS_LITTLE_AS_POSSIBLE : READ_UNTIL_END_OF_FILE);
}

std::ostream &stim::operator<<(std::ostream &out, const OperationData &dat) {
    if (!dat.args.empty()) {
        out << '(';
        bool first = true;
        for (auto e : dat.args) {
            if (first) {
                first = false;
            } else {
                out << ", ";
            }
            if (e > (double)INT64_MIN && e < (double)INT64_MAX && (int64_t)e == e) {
                out << (int64_t)e;
            } else {
                out << e;
            }
        }
        out << ')';
    }
    write_targets(out, dat.targets);
    return out;
}

std::ostream &stim::operator<<(std::ostream &out, const Operation &op) {
    out << op.gate->name << op.target_data;
    return out;
}

void stim::print_circuit(std::ostream &out, const Circuit &c, const std::string &indentation) {
    bool first = true;
    for (const auto &op : c.operations) {
        if (first) {
            first = false;
        } else {
            out << "\n";
        }

        // Recurse on repeat blocks.
        if (op.gate && op.gate->id == gate_name_to_id("REPEAT")) {
            if (op.target_data.targets.size() == 3 && op.target_data.targets[0].data < c.blocks.size()) {
                out << indentation << "REPEAT " << op_data_rep_count(op.target_data) << " {\n";
                print_circuit(out, c.blocks[op.target_data.targets[0].data], indentation + "    ");
                out << "\n" << indentation << "}";
                continue;
            }
        }

        out << indentation << op;
    }
}

Circuit &stim::op_data_block_body(Circuit &host, const OperationData &data) {
    return host.blocks[data.targets[0].data];
}

const Circuit &stim::op_data_block_body(const Circuit &host, const OperationData &data) {
    return host.blocks[data.targets[0].data];
}

std::ostream &stim::operator<<(std::ostream &out, const Circuit &c) {
    print_circuit(out, c, "");
    return out;
}

void Circuit::clear() {
    target_buf.clear();
    arg_buf.clear();
    operations.clear();
    blocks.clear();
}

Circuit Circuit::operator+(const Circuit &other) const {
    Circuit result = *this;
    result += other;
    return result;
}
Circuit Circuit::operator*(uint64_t repetitions) const {
    if (repetitions == 0) {
        return Circuit();
    }
    if (repetitions == 1) {
        return *this;
    }
    // If the entire circuit is a repeat block, just adjust its repeat count.
    if (operations.size() == 1 && operations[0].gate->id == gate_name_to_id("REPEAT")) {
        uint64_t old_reps = op_data_rep_count(operations[0].target_data);
        uint64_t new_reps = old_reps * repetitions;
        if (old_reps != new_reps / repetitions) {
            throw std::invalid_argument("Fused repetition count is too large.");
        }
        Circuit copy;
        copy.append_repeat_block(new_reps, op_data_block_body(*this, operations[0].target_data));
        return copy;
    }

    Circuit result;
    result.append_repeat_block(repetitions, *this);
    return result;
}

Circuit &Circuit::operator+=(const Circuit &other) {
    if (&other == this) {
        operations.insert(operations.end(), operations.begin(), operations.end());
        return *this;
    }

    uint32_t block_offset = (uint32_t)blocks.size();
    blocks.insert(blocks.end(), other.blocks.begin(), other.blocks.end());
    for (const auto &op : other.operations) {
        assert(op.gate != nullptr);
        auto target_data = append_operation(op);
        if (op.gate->id == gate_name_to_id("REPEAT")) {
            assert(op.target_data.targets.size() == 3);
            target_data[0].data += block_offset;
        }
    }

    return *this;
}
Circuit &Circuit::operator*=(uint64_t repetitions) {
    if (repetitions == 0) {
        clear();
    } else {
        *this = *this * repetitions;
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

std::string OperationData::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

Circuit Circuit::from_file(FILE *file) {
    Circuit result;
    result.append_from_file(file, false);
    return result;
}

Circuit::Circuit(const char *text) {
    append_from_text(text);
}

DetectorsAndObservables::DetectorsAndObservables(DetectorsAndObservables &&other) noexcept
    : jagged_detector_data(other.jagged_detector_data.total_allocated()),
      detectors(std::move(other.detectors)),
      observables(std::move(other.observables)) {
    // Keep a local copy of the detector data.
    for (PointerRange<uint64_t> &e : detectors) {
        e = jagged_detector_data.take_copy(e);
    }
}

DetectorsAndObservables &DetectorsAndObservables::operator=(DetectorsAndObservables &&other) noexcept {
    observables = std::move(other.observables);
    detectors = std::move(other.detectors);

    // Keep a local copy of the detector data.
    jagged_detector_data = MonotonicBuffer<uint64_t>(other.jagged_detector_data.total_allocated());
    for (PointerRange<uint64_t> &e : detectors) {
        e = jagged_detector_data.take_copy(e);
    }

    return *this;
}

DetectorsAndObservables::DetectorsAndObservables(const DetectorsAndObservables &other)
    : jagged_detector_data(other.jagged_detector_data.total_allocated()),
      detectors(other.detectors),
      observables(other.observables) {
    // Keep a local copy of the detector data.
    for (PointerRange<uint64_t> &e : detectors) {
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
    jagged_detector_data = MonotonicBuffer<uint64_t>(other.jagged_detector_data.total_allocated());
    for (PointerRange<uint64_t> &e : detectors) {
        e = jagged_detector_data.take_copy(e);
    }

    return *this;
}

size_t Circuit::count_qubits() const {
    return (uint32_t)max_operation_property([](const Operation &op) -> uint32_t {
        uint32_t r = 0;
        for (auto t : op.target_data.targets) {
            if (!(t.data & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
                r = std::max(r, t.qubit_value() + uint32_t{1});
            }
        }
        return r;
    });
}

size_t Circuit::max_lookback() const {
    return max_operation_property([](const Operation &op) -> uint32_t {
        uint32_t r = 0;
        for (auto t : op.target_data.targets) {
            if (t.data & TARGET_RECORD_BIT) {
                r = std::max(r, t.qubit_value());
            }
        }
        return r;
    });
}

uint64_t stim::add_saturate(uint64_t a, uint64_t b) {
    uint64_t r = a + b;
    if (r < a) {
        return UINT64_MAX;
    }
    return r;
}

uint64_t stim::mul_saturate(uint64_t a, uint64_t b) {
    if (b && a > UINT64_MAX / b) {
        return UINT64_MAX;
    }
    return a * b;
}

uint64_t Circuit::count_measurements() const {
    return flat_count_operations([=](const Operation &op) -> uint64_t {
        return op.count_measurement_results();
    });
}

uint64_t Circuit::count_detectors() const {
    const Gate *detector = &GATE_DATA.at("DETECTOR");
    return flat_count_operations([=](const Operation &op) -> uint64_t {
        return op.gate == detector;
    });
}

uint64_t Circuit::count_observables() const {
    const Gate *obs = &GATE_DATA.at("OBSERVABLE_INCLUDE");
    return max_operation_property([=](const Operation &op) -> uint64_t {
        return op.gate == obs ? (size_t)op.target_data.args[0] + 1 : 0;
    });
}

size_t Circuit::count_sweep_bits() const {
    return max_operation_property([](const Operation &op) -> uint32_t {
        uint32_t r = 0;
        for (auto t : op.target_data.targets) {
            if (t.data & TARGET_SWEEP_BIT) {
                r = std::max(r, t.qubit_value() + 1);
            }
        }
        return r;
    });
}

Circuit Circuit::py_get_slice(int64_t start, int64_t step, int64_t slice_length) const {
    assert(start >= 0);
    assert(slice_length >= 0);
    Circuit result;
    for (size_t k = 0; k < (size_t)slice_length; k++) {
        const auto &op = operations[start + step * k];
        if (op.gate->id == gate_name_to_id("REPEAT")) {
            result.target_buf.append_tail(GateTarget{(uint32_t)result.blocks.size()});
            result.target_buf.append_tail(op.target_data.targets[1]);
            result.target_buf.append_tail(op.target_data.targets[2]);
            auto targets = result.target_buf.commit_tail();
            result.blocks.push_back(op_data_block_body(*this, op.target_data));
            result.operations.push_back(Operation{op.gate, OperationData{{}, targets}});
        } else {
            auto args = result.arg_buf.take_copy(op.target_data.args);
            auto targets = result.target_buf.take_copy(op.target_data.targets);
            result.operations.push_back(Operation{op.gate, OperationData{args, targets}});
        }
    }
    return result;
}

void Circuit::append_repeat_block(uint64_t repeat_count, Circuit &&body) {
    if (repeat_count == 0) {
        throw std::invalid_argument("Can't repeat 0 times.");
    }
    target_buf.append_tail(GateTarget{(uint32_t)blocks.size()});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count & 0xFFFFFFFFULL)});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count >> 32)});
    blocks.push_back(std::move(body));
    auto targets = target_buf.commit_tail();
    operations.push_back({&GATE_DATA.at("REPEAT"), {{}, targets}});
}

void Circuit::append_repeat_block(uint64_t repeat_count, const Circuit &body) {
    if (repeat_count == 0) {
        throw std::invalid_argument("Can't repeat 0 times.");
    }
    target_buf.append_tail(GateTarget{(uint32_t)blocks.size()});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count & 0xFFFFFFFFULL)});
    target_buf.append_tail(GateTarget{(uint32_t)(repeat_count >> 32)});
    blocks.push_back(body);
    auto targets = target_buf.commit_tail();
    operations.push_back({&GATE_DATA.at("REPEAT"), {{}, targets}});
}

const Circuit Circuit::aliased_noiseless_circuit() const {
    // HACK: result has pointers into `circuit`!
    Circuit result;
    for (const auto &op : operations) {
        if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
            // Drop result flip probability.
            result.operations.push_back(Operation{op.gate, OperationData{{}, op.target_data.targets}});
        } else if (!(op.gate->flags & GATE_IS_NOISE)) {
            // Keep noiseless operations.
            result.operations.push_back(op);
        }
    }
    for (const auto &block : blocks) {
        result.blocks.push_back(block.aliased_noiseless_circuit());
    }
    return result;
}
