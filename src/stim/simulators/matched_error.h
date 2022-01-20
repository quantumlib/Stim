/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_SIMULATORS_MATCHED_ERROR_H
#define _STIM_SIMULATORS_MATCHED_ERROR_H

#include "stim/circuit/gate_target.h"
#include "stim/dem/detector_error_model.h"

namespace stim {

struct CircuitErrorLocationStackFrame {
    uint64_t instruction_offset;
    uint64_t iteration_index;
    uint64_t repeat_count;
    bool operator==(const CircuitErrorLocationStackFrame &other) const;
    bool operator!=(const CircuitErrorLocationStackFrame &other) const;
};

struct CircuitTargetsInsideInstruction {
    /// The name of the instruction that was executing.
    /// (This should always point into static data from GATE_DATA.)
    const Gate *gate;
    size_t target_range_start;
    size_t target_range_end;
    std::vector<GateTarget> targets_in_range;
    std::vector<std::vector<float>> target_coords;
    std::vector<double> args;

    Operation viewed_as_operation() const;

    void fill_in_data(const OperationData &actual_op,
                      const std::map<uint64_t, std::vector<float>> qubit_coords) {
        targets_in_range.clear();
        targets_in_range.insert(
            targets_in_range.begin(),
            &actual_op.targets[target_range_start],
            &actual_op.targets[target_range_end]);
        for (const auto &t : targets_in_range) {
            if (t.data & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT | TARGET_COMBINER)) {
                target_coords.push_back({});
            } else {
                auto entry = qubit_coords.find(t.qubit_value());
                if (entry == qubit_coords.end()) {
                    target_coords.push_back({});
                }
                target_coords.push_back(entry->second);
            }
        }
    }

    bool operator==(const CircuitTargetsInsideInstruction &other) const;
    bool operator!=(const CircuitTargetsInsideInstruction &other) const;
    std::string str() const;
};

struct FlippedMeasurement {
    uint64_t measurement_record_index;
    std::vector<GateTarget> measured_observable;
};

struct CircuitErrorLocation {
    /// The number of ticks that have been executed by this point.
    uint64_t tick_offset;

    /// The pauli terms corresponding to the circuit error.
    /// For non-measurement errors, this is the actual pauli error that triggers the problem.
    /// For measurement errors, this is the observable that was being measured.
    std::vector<GateTarget> flipped_pauli_product;

    /// Determines if the circuit error was a measurement error.
    /// UINT64_MAX means NOT a measurement error.
    /// Other values refer to a specific measurement index in the measurement record.
    FlippedMeasurement flipped_measurement;

    /// These two values identify a specific range of targets within the executing instruction.
    CircuitTargetsInsideInstruction instruction_targets;

    // Stack trace within the circuit and nested loop blocks.
    std::vector<CircuitErrorLocationStackFrame> stack_frames;

    std::string str() const;
    bool operator==(const CircuitErrorLocation &other) const;
    bool operator!=(const CircuitErrorLocation &other) const;
};

/// Explains how an error from a detector error model matches error(s) from a circuit.
struct MatchedError {
    /// A sorted list of detector and observable targets flipped by the error.
    /// This list should never contain target separators.
    std::vector<DemTarget> dem_error_terms;

    /// Locations of matching errors in the circuit.
    std::vector<CircuitErrorLocation> circuit_error_locations;

    std::string str() const;
    bool operator==(const MatchedError &other) const;
    bool operator!=(const MatchedError &other) const;
};

std::ostream &operator<<(std::ostream &out, const CircuitTargetsInsideInstruction &e);
std::ostream &operator<<(std::ostream &out, const CircuitErrorLocation &e);
std::ostream &operator<<(std::ostream &out, const MatchedError &e);

}  // namespace stim

#endif
