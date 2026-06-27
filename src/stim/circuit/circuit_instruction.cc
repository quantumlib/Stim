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
}

uint64_t CircuitInstruction::count_measurement_results() const {
    uint64_t n = (uint64_t)targets.size();
    std::cerr << "counting start ... " << n << "\n";
    for (auto e : targets) {
        if (e.is_combiner()) {
            n -= 2;
        }
    }
    std::cerr << "count final " << n << "\n";
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
