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

#include "stim/circuit/circuit_instruction.h"

#include "stim/circuit/circuit.h"
#include "stim/circuit/gate_target.h"
#include "stim/gates/gates.h"
#include "stim/util_bot/str_util.h"

using namespace stim;

uint64_t CircuitInstruction::repeat_block_rep_count() const {
    assert(targets.size() == 3);
    uint64_t low = targets[1].data;
    uint64_t high = targets[2].data;
    return low | (high << 32);
}

Circuit &CircuitInstruction::repeat_block_body(Circuit &host) const {
    assert(targets.size() == 3);
    auto b = targets[0].data;
    assert(b < host.blocks.size());
    return host.blocks[b];
}

CircuitStats CircuitInstruction::compute_stats(const Circuit *host) const {
    CircuitStats out;
    add_stats_to(out, host);
    return out;
}

void CircuitInstruction::add_stats_to(CircuitStats &out, const Circuit *host) const {
    if (gate_type == GateType::REPEAT) {
        if (host == nullptr) {
            throw std::invalid_argument("gate_type == REPEAT && host == nullptr");
        }
        // Recurse into blocks.
        auto sub = repeat_block_body(*host).compute_stats();
        auto reps = repeat_block_rep_count();
        out.num_observables = std::max(out.num_observables, sub.num_observables);
        out.num_qubits = std::max(out.num_qubits, sub.num_qubits);
        out.max_lookback = std::max(out.max_lookback, sub.max_lookback);
        out.num_sweep_bits = std::max(out.num_sweep_bits, sub.num_sweep_bits);
        out.num_detectors = add_saturate(out.num_detectors, mul_saturate(sub.num_detectors, reps));
        out.num_measurements = add_saturate(out.num_measurements, mul_saturate(sub.num_measurements, reps));
        out.num_ticks = add_saturate(out.num_ticks, mul_saturate(sub.num_ticks, reps));
        return;
    }

    for (auto t : targets) {
        auto v = t.data & TARGET_VALUE_MASK;
        // Qubit counting.
        if (gate_type != GateType::MPAD) {
            if (!(t.data & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
                out.num_qubits = std::max(out.num_qubits, v + 1);
            }
        }
        // Lookback counting.
        if (t.data & TARGET_RECORD_BIT) {
            out.max_lookback = std::max(out.max_lookback, v);
        }
        // Sweep bit counting.
        if (t.data & TARGET_SWEEP_BIT) {
            out.num_sweep_bits = std::max(out.num_sweep_bits, v + 1);
        }
    }

    // Measurement counting.
    out.num_measurements += count_measurement_results();

    switch (gate_type) {
        case GateType::DETECTOR:
            // Detector counting.
            out.num_detectors += out.num_detectors < UINT64_MAX;
            break;
        case GateType::OBSERVABLE_INCLUDE:
            // Observable counting.
            out.num_observables = std::max(out.num_observables, (uint64_t)args[0] + 1);
            break;
        case GateType::TICK:
            // Tick counting.
            out.num_ticks++;
            break;
        default:
            break;
    }
}

const Circuit &CircuitInstruction::repeat_block_body(const Circuit &host) const {
    assert(targets.size() == 3);
    auto b = targets[0].data;
    assert(b < host.blocks.size());
    return host.blocks[b];
}

CircuitInstruction::CircuitInstruction(
    GateType gate_type, SpanRef<const double> args, SpanRef<const GateTarget> targets, std::string_view tag)
    : gate_type(gate_type), args(args), targets(targets), tag(tag) {
}

void CircuitInstruction::validate() const {
    const Gate &gate = GATE_DATA[gate_type];

    if (gate.flags == GateFlags::NO_GATE_FLAG) {
        throw std::invalid_argument("Unrecognized gate_type. Associated flag is NO_GATE_FLAG.");
    }

    if (gate.flags & GATE_TARGETS_PAIRS) {
        if (gate.flags & GATE_TARGETS_PAULI_STRING) {
            size_t term_count = targets.size();
            for (auto t : targets) {
                if (t.is_combiner()) {
                    term_count -= 2;
                }
            }
            if (term_count & 1) {
                throw std::invalid_argument(
                    "The gate " + std::string(gate.name) +
                    " requires an even number of products to target, but was given "
                    "(" +
                    comma_sep(args).str() + ").");
            }
        } else {
            if (targets.size() & 1) {
                throw std::invalid_argument(
                    "Two qubit gate " + std::string(gate.name) +
                    " requires an even number of targets but was given "
                    "(" +
                    comma_sep(targets).str() + ").");
            }
            for (size_t k = 0; k < targets.size(); k += 2) {
                if (targets[k] == targets[k + 1]) {
                    throw std::invalid_argument(
                        "The two qubit gate " + std::string(gate.name) +
                        " was applied to a target pair with the same target (" + targets[k].target_str() +
                        ") twice. Gates can't interact targets with themselves.");
                }
            }
        }
    }

    if (gate.arg_count == ARG_COUNT_SYGIL_ZERO_OR_ONE) {
        if (args.size() > 1) {
            throw std::invalid_argument(
                "Gate " + std::string(gate.name) + " was given " + std::to_string(args.size()) + " parens arguments (" +
                comma_sep(args).str() + ") but takes 0 or 1 parens arguments.");
        }
    } else if (args.size() != gate.arg_count && gate.arg_count != ARG_COUNT_SYGIL_ANY) {
        throw std::invalid_argument(
            "Gate " + std::string(gate.name) + " was given " + std::to_string(args.size()) + " parens arguments (" +
            comma_sep(args).str() + ") but takes " + std::to_string(gate.arg_count) + " parens arguments.");
    }

    if ((gate.flags & GATE_TAKES_NO_TARGETS) && !targets.empty()) {
        throw std::invalid_argument(
            "Gate " + std::string(gate.name) + " takes no targets but was given targets" + targets_str(targets) + ".");
    }

    if (gate.flags & GATE_ARGS_ARE_DISJOINT_PROBABILITIES) {
        double total = 0;
        for (const auto p : args) {
            if (!(p >= 0 && p <= 1)) {
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
    if (gate.flags & GATE_PRODUCES_RESULTS) {
        valid_target_mask |= TARGET_INVERTED_BIT;
    }
    if (gate.flags & GATE_CAN_TARGET_BITS) {
        valid_target_mask |= TARGET_RECORD_BIT | TARGET_SWEEP_BIT;
    }
    if (gate.flags & GATE_ONLY_TARGETS_MEASUREMENT_RECORD) {
        if (gate.flags & GATE_TARGETS_PAULI_STRING) {
            for (GateTarget q : targets) {
                if (!q.is_measurement_record_target() && !q.is_pauli_target()) {
                    throw std::invalid_argument("Gate " + std::string(gate.name) + " only takes measurement record targets and Pauli targets (rec[-k], Xk, Yk, Zk).");
                }
            }
        } else {
            for (GateTarget q : targets) {
                if (!q.is_measurement_record_target()) {
                    throw std::invalid_argument("Gate " + std::string(gate.name) + " only takes measurement record targets (rec[-k]).");
                }
            }
        }
    } else if (gate.flags & GATE_TARGETS_PAULI_STRING) {
        if (gate.flags & GATE_CAN_TARGET_BITS) {
            for (GateTarget q : targets) {
                if (!(q.data & (TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT | TARGET_COMBINER | TARGET_SWEEP_BIT |
                                TARGET_RECORD_BIT))) {
                    throw std::invalid_argument(
                        "Gate " + std::string(gate.name) +
                        " only takes Pauli targets or bit targets ('X2', 'Y3', 'Z5', 'rec[-1]', 'sweep[0]', etc).");
                }
            }
        } else {
            for (GateTarget q : targets) {
                if (!(q.data & (TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT | TARGET_COMBINER))) {
                    throw std::invalid_argument(
                        "Gate " + std::string(gate.name) + " only takes Pauli targets ('X2', 'Y3', 'Z5', etc).");
                }
            }
        }
    } else {
        for (GateTarget q : targets) {
            if (q.data != (q.data & valid_target_mask)) {
                std::stringstream ss;
                ss << "Target ";
                q.write_succinct(ss);
                ss << " has invalid modifiers for gate type '" << gate.name << "'.";
                throw std::invalid_argument(ss.str());
            }
        }
    }
    if (gate_type == GateType::MPAD) {
        for (const auto &t : targets) {
            if (t.data > 1) {
                std::stringstream ss;
                ss << "Target ";
                t.write_succinct(ss);
                ss << " is not valid for gate type '" << gate.name << "'.";
                throw std::invalid_argument(ss.str());
            }
        }
    }
}

uint64_t CircuitInstruction::count_measurement_results() const {
    auto flags = GATE_DATA[gate_type].flags;
    if (!(flags & GATE_PRODUCES_RESULTS)) {
        return 0;
    }
    uint64_t n = (uint64_t)targets.size();
    if (flags & GATE_TARGETS_PAIRS) {
        return n >> 1;
    } else if (flags & GATE_TARGETS_COMBINERS) {
        for (auto e : targets) {
            if (e.is_combiner()) {
                n -= 2;
            }
        }
    }
    return n;
}

bool CircuitInstruction::can_fuse(const CircuitInstruction &other) const {
    auto flags = GATE_DATA[gate_type].flags;
    return gate_type == other.gate_type && args == other.args && !(flags & GATE_IS_NOT_FUSABLE) && tag == other.tag;
}

bool CircuitInstruction::operator==(const CircuitInstruction &other) const {
    return gate_type == other.gate_type && args == other.args && targets == other.targets && tag == other.tag;
}
bool CircuitInstruction::approx_equals(const CircuitInstruction &other, double atol) const {
    if (gate_type != other.gate_type || targets != other.targets || args.size() != other.args.size() ||
        tag != other.tag) {
        return false;
    }
    for (size_t k = 0; k < args.size(); k++) {
        if (fabs(args[k] - other.args[k]) > atol) {
            return false;
        }
    }
    return true;
}

bool CircuitInstruction::operator!=(const CircuitInstruction &other) const {
    return !(*this == other);
}

std::ostream &stim::operator<<(std::ostream &out, const CircuitInstruction &instruction) {
    out << GATE_DATA[instruction.gate_type].name;
    if (!instruction.tag.empty()) {
        out << '[';
        write_tag_escaped_string_to(instruction.tag, out);
        out << ']';
    }
    if (!instruction.args.empty()) {
        out << '(';
        bool first = true;
        for (auto e : instruction.args) {
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
    write_targets(out, instruction.targets);
    return out;
}

void stim::write_tag_escaped_string_to(std::string_view tag, std::ostream &out) {
    for (char c : tag) {
        switch (c) {
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\\':
                out << "\\B";
                break;
            case ']':
                out << "\\C";
                break;
            default:
                out << c;
        }
    }
}

std::string CircuitInstruction::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}
