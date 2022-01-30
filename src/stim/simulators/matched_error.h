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

/// Describes the location of an instruction being executed within a
/// circuit or loop, distinguishing between separate loop iterations.
///
/// The full location is a stack of these frames, drilling down from
/// the top level circuit to the inner-most loop that the instruction
/// is within.
struct CircuitErrorLocationStackFrame {
    /// The index of the instruction in the circuit (or block).
    uint64_t instruction_offset;
    /// If inside a loop, the number of iterations of the loop that completed
    /// before the moment that we are trying to refer to.
    /// If not in a loop, set to 0.
    uint64_t iteration_index;
    /// If the instruction is a REPEAT block, this is how many times it repeats.
    /// If not, set to 0.
    uint64_t instruction_repetitions_arg;

    bool operator<(const CircuitErrorLocationStackFrame &other) const;

    /// Standard methods for easy testing.
    bool operator==(const CircuitErrorLocationStackFrame &other) const;
    bool operator!=(const CircuitErrorLocationStackFrame &other) const;
    std::string str() const;
};

/// A gate target with qubit coordinate metadata attached to it.
struct GateTargetWithCoords {
    GateTarget gate_target;
    std::vector<double> coords;

    bool operator<(const GateTargetWithCoords &other) const;

    /// Standard methods for easy testing.
    bool operator==(const GateTargetWithCoords &other) const;
    bool operator!=(const GateTargetWithCoords &other) const;
    std::string str() const;
};

/// A dem target with detector coordinate metadata attached to it.
struct DemTargetWithCoords {
    DemTarget dem_target;
    std::vector<double> coords;

    bool operator<(const DemTargetWithCoords &other) const;

    /// Standard methods for easy testing.
    bool operator==(const DemTargetWithCoords &other) const;
    bool operator!=(const DemTargetWithCoords &other) const;
    std::string str() const;
};

/// Stores additional details about measurement errors.
struct FlippedMeasurement {
    /// Which output bit this measurement corresponds to.
    /// UINT64_MAX means "no measurement error occurred".
    uint64_t measurement_record_index;
    /// Which observable this measurement was responsible for measuring.
    std::vector<GateTargetWithCoords> measured_observable;

    bool operator<(const FlippedMeasurement &other) const;

    /// Standard methods for easy testing.
    bool operator==(const FlippedMeasurement &other) const;
    bool operator!=(const FlippedMeasurement &other) const;
    std::string str() const;
};

/// Describes a specific range of targets within a parameterized instruction.
struct CircuitTargetsInsideInstruction {
    /// The instruction type.
    const Gate *gate;

    /// The parens arguments for the instruction.
    std::vector<double> args;

    /// The range of targets within the instruction that were executing.
    size_t target_range_start;
    size_t target_range_end;
    std::vector<GateTargetWithCoords> targets_in_range;

    void fill_args_and_targets_in_range(
        const OperationData &actual_op, const std::map<uint64_t, std::vector<double>> &qubit_coords);

    bool operator<(const CircuitTargetsInsideInstruction &other) const;

    /// Standard methods for easy testing.
    bool operator==(const CircuitTargetsInsideInstruction &other) const;
    bool operator!=(const CircuitTargetsInsideInstruction &other) const;
    std::string str() const;
};

/// Describes the location of an error within a circuit, with as much extra information
/// as possible in order to make it easier for users to grok the location.
struct CircuitErrorLocation {
    /// The number of ticks that have been executed by this point.
    uint64_t tick_offset;

    /// The pauli terms corresponding to the circuit error.
    /// For non-measurement errors, this is the actual pauli error that triggers the problem.
    /// For measurement errors, this is the observable that was being measured.
    std::vector<GateTargetWithCoords> flipped_pauli_product;

    /// Determines if the circuit error was a measurement error.
    /// UINT64_MAX means NOT a measurement error.
    /// Other values refer to a specific measurement index in the measurement record.
    FlippedMeasurement flipped_measurement;

    /// These two values identify a specific range of targets within the executing instruction.
    CircuitTargetsInsideInstruction instruction_targets;

    // Stack trace within the circuit and nested loop blocks.
    std::vector<CircuitErrorLocationStackFrame> stack_frames;

    void canonicalize();
    bool operator<(const CircuitErrorLocation &other) const;
    bool is_simpler_than(const CircuitErrorLocation &other) const;

    /// Standard methods for easy testing.
    bool operator==(const CircuitErrorLocation &other) const;
    bool operator!=(const CircuitErrorLocation &other) const;
    std::string str() const;
};

/// Explains how an error from a detector error model matches error(s) from a circuit.
struct ExplainedError {
    /// A sorted list of detector and observable targets flipped by the error.
    /// This list should never contain target separators.
    std::vector<DemTargetWithCoords> dem_error_terms;

    /// Locations of matching errors in the circuit.
    std::vector<CircuitErrorLocation> circuit_error_locations;

    void fill_in_dem_targets(
        ConstPointerRange<DemTarget> targets, const std::map<uint64_t, std::vector<double>> &dem_coords);

    void canonicalize();

    /// Standard methods for easy testing.
    std::string str() const;
    bool operator==(const ExplainedError &other) const;
    bool operator!=(const ExplainedError &other) const;
};

std::ostream &operator<<(std::ostream &out, const CircuitErrorLocation &e);
std::ostream &operator<<(std::ostream &out, const CircuitErrorLocationStackFrame &e);
std::ostream &operator<<(std::ostream &out, const CircuitTargetsInsideInstruction &e);
std::ostream &operator<<(std::ostream &out, const DemTargetWithCoords &e);
std::ostream &operator<<(std::ostream &out, const FlippedMeasurement &e);
std::ostream &operator<<(std::ostream &out, const GateTargetWithCoords &e);
std::ostream &operator<<(std::ostream &out, const ExplainedError &e);

}  // namespace stim

#endif
