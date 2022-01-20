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

#include <algorithm>
#include <queue>
#include <sstream>

#include "stim/simulators/matched_error.h"

using namespace stim;

std::string MatchedError::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::string CircuitErrorLocation::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool MatchedError::operator==(const MatchedError &other) const {
    return dem_error_terms == other.dem_error_terms
        && circuit_error_locations == other.circuit_error_locations;
}

bool MatchedError::operator!=(const MatchedError &other) const {
    return !(*this == other);
}

bool CircuitErrorLocationStackFrame::operator==(const CircuitErrorLocationStackFrame &other) const {
    return iteration_index == other.iteration_index
        && instruction_offset == other.instruction_offset
        && repeat_count == other.repeat_count;
}

bool CircuitErrorLocationStackFrame::operator!=(const CircuitErrorLocationStackFrame &other) const {
    return !(*this == other);
}

bool CircuitErrorLocation::operator==(const CircuitErrorLocation &other) const {
    return flipped_measurement.measurement_record_index == other.flipped_measurement.measurement_record_index
        && flipped_measurement.measured_observable == other.flipped_measurement.measured_observable
        && tick_offset == other.tick_offset
        && flipped_pauli_product == other.flipped_pauli_product
        && instruction_targets == other.instruction_targets
        && stack_frames == other.stack_frames;
}

bool CircuitErrorLocation::operator!=(const CircuitErrorLocation &other) const {
    return !(*this == other);
}

void print_pauli_product(std::ostream &out, const std::vector<GateTarget> &pauli_terms) {
    for (size_t k = 0; k < pauli_terms.size(); k++) {
        if (k) {
            out << "*";
        }
        const auto &p = pauli_terms[k];
        if (p.is_x_target()) {
            out << "X";
        } else if (p.is_y_target()) {
            out << "Y";
        } else if (p.is_z_target()) {
            out << "Z";
        }
        out << p.qubit_value();
    }
}

void print_circuit_error_loc_indent(std::ostream &out, const CircuitErrorLocation &e, const char *indent) {
    out << indent << "CircuitErrorLocation {\n";

    if (!e.flipped_pauli_product.empty()) {
        out << indent << "    flipped_pauli_product: ";
        print_pauli_product(out, e.flipped_pauli_product);
        out << "\n";
    }
    if (e.flipped_measurement.measurement_record_index != UINT64_MAX) {
        out << indent << "    flipped_measurement.measurement_record_index: " << e.flipped_measurement.measurement_record_index << "\n";
        out << indent << "    flipped_measurement.measured_observable: ";
        print_pauli_product(out, e.flipped_measurement.measured_observable);
        out << "\n";
    }

    out << indent << "    Circuit location stack trace:\n";
    out << indent << "        (after " << e.tick_offset << " TICKs)\n";
    for (size_t k = 0; k < e.stack_frames.size(); k++) {
        const auto &frame = e.stack_frames[k];
        if (k) {
            out << indent << "        after " << frame.iteration_index << " completed iterations\n";
        }
        out << indent << "        ";
        out << "at instruction #" << (frame.instruction_offset + 1);
        if (k < e.stack_frames.size() - 1) {
            out << " (a REPEAT " << frame.repeat_count << " block)";
        } else if (e.instruction_targets.gate != nullptr) {
            out << " (" << e.instruction_targets.gate->name << ")";
        }
        if (k) {
            out << " in the REPEAT block";
        } else {
            out << " in the circuit";
        }
        out << "\n";
    }
    if (e.instruction_targets.target_range_start + 1 == e.instruction_targets.target_range_end) {
        out << indent << "        at target #" << (e.instruction_targets.target_range_start + 1);
    } else {
        out << indent << "        at targets #" << (e.instruction_targets.target_range_start + 1);
        out << " to #" << e.instruction_targets.target_range_end;
    }
    out << " of the instruction\n";
    out << "        resolving to " << e.instruction_targets << "\n";

    out << indent << "}";
}

std::ostream &stim::operator<<(std::ostream &out, const MatchedError &e) {
    out << "MatchedError {\n";
    out << "    dem_error_terms: " << comma_sep(e.dem_error_terms, " ") << "\n";
    for (const auto &loc : e.circuit_error_locations) {
        print_circuit_error_loc_indent(out, loc, "    ");
    }
    out << "}\n";
    return out;
}

std::ostream &stim::operator<<(std::ostream &out, const CircuitErrorLocation &e) {
    print_circuit_error_loc_indent(out, e, "");
    return out;
}

Operation CircuitTargetsInsideInstruction::viewed_as_operation() const {
    return {
        gate,
        {
            args,
            targets_in_range,
        }
    };
}
bool CircuitTargetsInsideInstruction::operator==(const CircuitTargetsInsideInstruction &other) const {
    return gate == other.gate
        && target_range_start == other.target_range_start
        && target_range_end == other.target_range_end
        && targets_in_range == other.targets_in_range
        && target_coords == other.target_coords
        && args == other.args;
}
bool CircuitTargetsInsideInstruction::operator!=(const CircuitTargetsInsideInstruction &other) const {
    return !(*this == other);
}

std::ostream &stim::operator<<(std::ostream &out, const CircuitTargetsInsideInstruction &e) {
    if (e.gate == nullptr) {
        out << "null";
    } else {
        out << e.gate->name;
    }
    if (!e.args.empty()) {
        out << '(' << comma_sep(e.args) << ')';
    }
    bool was_combiner = false;
    for (size_t k = 0; k < e.targets_in_range.size(); k++) {
        const auto &t = e.targets_in_range[k];
        bool is_combiner = t.is_combiner();
        if (!is_combiner && !was_combiner) {
            out << ' ';
        }
        was_combiner = is_combiner;
        t.write_succinct(out);
        if (k < e.target_coords.size() && !e.target_coords[k].empty()) {
            out << "[coords " << comma_sep(e.target_coords[k], ",") << "]";
        }
    }
    return out;
}
