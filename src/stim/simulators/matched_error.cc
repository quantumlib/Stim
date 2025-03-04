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

#include "stim/simulators/matched_error.h"

#include <algorithm>
#include <queue>
#include <sstream>

#include "stim/util_bot/str_util.h"

using namespace stim;

void print_pauli_product(std::ostream &out, const std::vector<GateTargetWithCoords> &pauli_terms) {
    for (size_t k = 0; k < pauli_terms.size(); k++) {
        const auto &p = pauli_terms[k];
        if (k) {
            out << "*";
        }
        out << p;
    }
}

void print_circuit_error_loc_indent(std::ostream &out, const CircuitErrorLocation &e, const char *indent) {
    out << indent << "CircuitErrorLocation {\n";

    if (!e.noise_tag.empty()) {
        out << indent << "    noise_tag: " << e.noise_tag << "\n";
    }
    if (!e.flipped_pauli_product.empty()) {
        out << indent << "    flipped_pauli_product: ";
        print_pauli_product(out, e.flipped_pauli_product);
        out << "\n";
    }
    if (e.flipped_measurement.measurement_record_index != UINT64_MAX) {
        out << indent
            << "    flipped_measurement.measurement_record_index: " << e.flipped_measurement.measurement_record_index
            << "\n";
    }
    if (!e.flipped_measurement.measured_observable.empty()) {
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
        const auto &gate_data = GATE_DATA[e.instruction_targets.gate_type];
        if (k < e.stack_frames.size() - 1) {
            out << " (a REPEAT " << frame.instruction_repetitions_arg << " block)";
        } else {
            out << " (" << gate_data.name << ")";
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
    out << indent << "        resolving to " << e.instruction_targets << "\n";

    out << indent << "}";
}

void CircuitTargetsInsideInstruction::fill_args_and_targets_in_range(
    const CircuitInstruction &actual_op, const std::map<uint64_t, std::vector<double>> &qubit_coords) {
    targets_in_range.clear();
    for (size_t k = target_range_start; k < target_range_end; k++) {
        const auto &t = actual_op.targets[k];
        bool is_non_coord_target = t.data & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT | TARGET_COMBINER);
        auto entry = qubit_coords.find(t.qubit_value());
        if (entry == qubit_coords.end() || is_non_coord_target) {
            targets_in_range.push_back({t, {}});
        } else {
            targets_in_range.push_back({t, entry->second});
        }
    }

    args.clear();
    args.insert(args.begin(), actual_op.args.begin(), actual_op.args.end());
}

void ExplainedError::fill_in_dem_targets(
    SpanRef<const DemTarget> targets, const std::map<uint64_t, std::vector<double>> &dem_coords) {
    dem_error_terms.clear();
    for (const auto &t : targets) {
        auto entry = dem_coords.find(t.raw_id());
        if (t.is_relative_detector_id() && entry != dem_coords.end()) {
            dem_error_terms.push_back({t, entry->second});
        } else {
            dem_error_terms.push_back({t, {}});
        }
    }
}

std::ostream &stim::operator<<(std::ostream &out, const FlippedMeasurement &e) {
    out << "FlippedMeasurement{";
    if (e.measurement_record_index == UINT64_MAX) {
        return out << "none}";
    }
    out << e.measurement_record_index;
    out << ", ";
    print_pauli_product(out, e.measured_observable);
    out << "}";
    return out;
}
std::ostream &stim::operator<<(std::ostream &out, const CircuitErrorLocation &e) {
    print_circuit_error_loc_indent(out, e, "");
    return out;
}
std::ostream &stim::operator<<(std::ostream &out, const CircuitErrorLocationStackFrame &e) {
    out << "CircuitErrorLocationStackFrame";
    out << "{instruction_offset=" << e.instruction_offset;
    out << ", iteration_index=" << e.iteration_index;
    out << ", instruction_repetitions_arg=" << e.instruction_repetitions_arg << "}";
    return out;
}
std::ostream &stim::operator<<(std::ostream &out, const ExplainedError &e) {
    out << "ExplainedError {\n";
    out << "    dem_error_terms: " << comma_sep(e.dem_error_terms, " ");
    if (e.circuit_error_locations.empty()) {
        out << "\n    [no single circuit error had these exact symptoms]";
    }
    for (const auto &loc : e.circuit_error_locations) {
        out << "\n";
        print_circuit_error_loc_indent(out, loc, "    ");
    }
    out << "\n}";
    return out;
}

std::ostream &stim::operator<<(std::ostream &out, const CircuitTargetsInsideInstruction &e) {
    const auto &gate_data = GATE_DATA[e.gate_type];
    if (gate_data.flags == GateFlags::NO_GATE_FLAG) {
        out << "null";
    } else {
        out << gate_data.name;
    }
    if (!e.gate_tag.empty()) {
        out << '[';
        write_tag_escaped_string_to(e.gate_tag, out);
        out << ']';
    }
    if (!e.args.empty()) {
        out << '(' << comma_sep(e.args) << ')';
    }
    bool was_combiner = false;
    for (const auto &t : e.targets_in_range) {
        bool is_combiner = t.gate_target.is_combiner();
        if (!is_combiner && !was_combiner) {
            out << ' ';
        }
        was_combiner = is_combiner;
        out << t;
    }
    return out;
}
std::ostream &stim::operator<<(std::ostream &out, const GateTargetWithCoords &e) {
    e.gate_target.write_succinct(out);
    if (!e.coords.empty()) {
        out << "[coords " << comma_sep(e.coords, ",") << "]";
    }
    return out;
}
std::ostream &stim::operator<<(std::ostream &out, const DemTargetWithCoords &e) {
    out << e.dem_target;
    if (!e.coords.empty()) {
        out << "[coords " << comma_sep(e.coords, ",") << "]";
    }
    return out;
}

bool ExplainedError::operator==(const ExplainedError &other) const {
    return dem_error_terms == other.dem_error_terms && circuit_error_locations == other.circuit_error_locations;
}
bool CircuitErrorLocationStackFrame::operator==(const CircuitErrorLocationStackFrame &other) const {
    return iteration_index == other.iteration_index && instruction_offset == other.instruction_offset &&
           instruction_repetitions_arg == other.instruction_repetitions_arg;
}
bool CircuitErrorLocation::operator==(const CircuitErrorLocation &other) const {
    return flipped_measurement == other.flipped_measurement && tick_offset == other.tick_offset &&
           flipped_pauli_product == other.flipped_pauli_product && instruction_targets == other.instruction_targets &&
           stack_frames == other.stack_frames;
}
bool CircuitTargetsInsideInstruction::operator==(const CircuitTargetsInsideInstruction &other) const {
    return gate_type == other.gate_type && gate_tag == other.gate_tag && target_range_start == other.target_range_start &&
           target_range_end == other.target_range_end && targets_in_range == other.targets_in_range &&
           args == other.args;
}
bool DemTargetWithCoords::operator==(const DemTargetWithCoords &other) const {
    return coords == other.coords && dem_target == other.dem_target;
}
bool FlippedMeasurement::operator==(const FlippedMeasurement &other) const {
    return measured_observable == other.measured_observable &&
           measurement_record_index == other.measurement_record_index;
}
bool GateTargetWithCoords::operator==(const GateTargetWithCoords &other) const {
    return coords == other.coords && gate_target == other.gate_target;
}

bool CircuitErrorLocation::operator!=(const CircuitErrorLocation &other) const {
    return !(*this == other);
}
bool CircuitErrorLocationStackFrame::operator!=(const CircuitErrorLocationStackFrame &other) const {
    return !(*this == other);
}
bool CircuitTargetsInsideInstruction::operator!=(const CircuitTargetsInsideInstruction &other) const {
    return !(*this == other);
}
bool DemTargetWithCoords::operator!=(const DemTargetWithCoords &other) const {
    return !(*this == other);
}
bool FlippedMeasurement::operator!=(const FlippedMeasurement &other) const {
    return !(*this == other);
}
bool GateTargetWithCoords::operator!=(const GateTargetWithCoords &other) const {
    return !(*this == other);
}
bool ExplainedError::operator!=(const ExplainedError &other) const {
    return !(*this == other);
}

std::string CircuitErrorLocation::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
std::string CircuitErrorLocationStackFrame::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
std::string CircuitTargetsInsideInstruction::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
std::string DemTargetWithCoords::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
std::string FlippedMeasurement::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
std::string GateTargetWithCoords::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
std::string ExplainedError::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

void ExplainedError::canonicalize() {
    for (auto &c : circuit_error_locations) {
        c.canonicalize();
    }
    std::sort(dem_error_terms.begin(), dem_error_terms.end());
    std::sort(circuit_error_locations.begin(), circuit_error_locations.end());
}

void CircuitErrorLocation::canonicalize() {
    std::sort(flipped_pauli_product.begin(), flipped_pauli_product.end());
    std::sort(flipped_measurement.measured_observable.begin(), flipped_measurement.measured_observable.end());
}

template <typename T>
bool vec_less_than(const std::vector<T> &vec1, const std::vector<T> &vec2) {
    SpanRef<const T> c1 = vec1;
    SpanRef<const T> c2 = vec2;
    return c1 < c2;
}
bool GateTargetWithCoords::operator<(const GateTargetWithCoords &other) const {
    if (gate_target != other.gate_target) {
        return gate_target < other.gate_target;
    }
    if (coords != other.coords) {
        return vec_less_than(coords, other.coords);
    }

    return false;
}
bool DemTargetWithCoords::operator<(const DemTargetWithCoords &other) const {
    if (dem_target != other.dem_target) {
        return dem_target < other.dem_target;
    }
    if (coords != other.coords) {
        return vec_less_than(coords, other.coords);
    }

    return false;
}
bool CircuitErrorLocation::operator<(const CircuitErrorLocation &other) const {
    if (tick_offset != other.tick_offset) {
        return tick_offset < other.tick_offset;
    }
    if (flipped_pauli_product != other.flipped_pauli_product) {
        return vec_less_than(flipped_pauli_product, other.flipped_pauli_product);
    }
    if (flipped_measurement != other.flipped_measurement) {
        return flipped_measurement < other.flipped_measurement;
    }
    if (instruction_targets != other.instruction_targets) {
        return instruction_targets < other.instruction_targets;
    }
    if (stack_frames != other.stack_frames) {
        return vec_less_than(stack_frames, other.stack_frames);
    }
    return false;
}
bool FlippedMeasurement::operator<(const FlippedMeasurement &other) const {
    if (measurement_record_index != other.measurement_record_index) {
        return measurement_record_index < other.measurement_record_index;
    }
    if (measured_observable != other.measured_observable) {
        return vec_less_than(measured_observable, other.measured_observable);
    }
    return false;
}
bool CircuitErrorLocationStackFrame::operator<(const CircuitErrorLocationStackFrame &other) const {
    if (instruction_offset != other.instruction_offset) {
        return instruction_offset < other.instruction_offset;
    }
    if (iteration_index != other.iteration_index) {
        return iteration_index < other.iteration_index;
    }
    if (instruction_repetitions_arg != other.instruction_repetitions_arg) {
        return instruction_repetitions_arg < other.instruction_repetitions_arg;
    }
    return false;
}
bool CircuitTargetsInsideInstruction::operator<(const CircuitTargetsInsideInstruction &other) const {
    if (target_range_start != other.target_range_start) {
        return target_range_start < other.target_range_start;
    }
    if (target_range_end != other.target_range_end) {
        return target_range_end < other.target_range_end;
    }
    if (targets_in_range != other.targets_in_range) {
        return vec_less_than(targets_in_range, other.targets_in_range);
    }
    if (args != other.args) {
        return vec_less_than(args, other.args);
    }
    if (gate_type == GateType::NOT_A_GATE || other.gate_type == GateType::NOT_A_GATE) {
        return gate_type < other.gate_type;
    }
    return GATE_DATA[gate_type].name < GATE_DATA[other.gate_type].name;
}

bool CircuitErrorLocation::is_simpler_than(const CircuitErrorLocation &other) const {
    if (flipped_measurement.measured_observable.size() != other.flipped_measurement.measured_observable.size()) {
        return other.flipped_measurement.measured_observable.size() <
               other.flipped_measurement.measured_observable.size();
    }
    if (flipped_pauli_product.size() != other.flipped_pauli_product.size()) {
        return flipped_pauli_product.size() < other.flipped_pauli_product.size();
    }
    return *this < other;
}
