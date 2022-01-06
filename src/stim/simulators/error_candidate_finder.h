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

#ifndef _STIM_SIMULATORS_ERROR_CANDIDATE_FINDER_H
#define _STIM_SIMULATORS_ERROR_CANDIDATE_FINDER_H

#include "stim/simulators/error_analyzer.h"

namespace stim {

/// Identifies a location in a circuit.
struct MatchedDetectorCircuitError {
    /// A sorted list of detector and observable targets flipped by the error.
    /// This list should never contain target separators.
    std::vector<DemTarget> dem_error_terms;

    /// Determines if the circuit error was a measurement error.
    /// UINT64_MAX means NOT a measurement error.
    /// Other values refer to a specific measurement index in the measurement record.
    uint64_t id_of_flipped_measurement;

    /// The pauli terms corresponding to the circuit error.
    /// For non-measurement errors, this is the actual pauli error that triggers the problem.
    /// For measurement errors, this is the observable that was being measured.
    std::vector<GateTarget> pauli_error_terms;

    /// The number of ticks that have been executed by this point.
    uint64_t ticks_beforehand;

    /// These two values identify a specific range of targets within the executing instruction.
    uint64_t instruction_target_start;
    uint64_t instruction_target_end;

    /// The name of the instruction that was executing.
    /// (This should always point into static data from GATE_DATA.)
    const char *instruction_name;

    // Stack trace within the circuit and nested loop blocks.
    std::vector<uint64_t> instruction_indices;
    std::vector<uint64_t> iteration_indices;

    std::string str() const;
    bool operator==(const MatchedDetectorCircuitError &other) const;
    bool operator!=(const MatchedDetectorCircuitError &other) const;
};

std::ostream &operator<<(std::ostream &out, const MatchedDetectorCircuitError &e);

/// This class handles matching circuit errors to detector-error-model errors.
struct ErrorCandidateFinder {
    /// The error analyzer handles most of converting circuit errors into detector errors.
    ErrorAnalyzer error_analyzer;

    // This value is tweaked and adjusted while iterating through the circuit, tracking
    // where we currently are.
    MatchedDetectorCircuitError loc;

    // Determines which errors to keep.
    //
    // Pointed-to key data is owned by `filter_targets_buf``.
    std::set<ConstPointerRange<DemTarget>> filter;
    MonotonicBuffer<DemTarget> filter_targets_buf;

    // Tracks discovered pairings keyed by their detector-error-model error terms.
    //
    // Key data points to vector data inside the associated value data.
    // This is only correct assuming the vector data stays in place, which should
    // hold as long as the vector is left alone (except it should be safe to std::move it).
    std::map<ConstPointerRange<DemTarget>, MatchedDetectorCircuitError> output_map;

    uint64_t total_measurements_in_circuit;
    uint64_t total_ticks_in_circuit;

    // This class has pointers into its own data. Can't just copy it around!
    ErrorCandidateFinder(const ErrorCandidateFinder &) = delete;

    /// Finds detector-error-model errors while matching them with their source circuit errors.
    ///
    /// Note that detector-error-model errors are not repeated. Each is matched with one
    /// representative circuit error, which is chosen arbitrarily from the available options.
    ///
    /// Args:
    ///     circuit: The noisy circuit to search for detector-error-model+circuit matches.
    ///     filter: Optional. When not empty, any detector-error-model errors that don't appear
    ///         in this filter will not be included in the result. When empty, all
    ///         detector-error-model errors are included.
    ///
    /// Returns:
    ///     A list of detector-error-model-paired-with-explanatory-circuit-error items.
    static std::vector<MatchedDetectorCircuitError> candidate_localized_dem_errors_from_circuit(
            const Circuit &circuit,
            const DetectorErrorModel &filter);

    /// Constructs an error candidate finder based on parameters that are given to
    /// `ErrorCandidateFinder::candidate_localized_dem_errors_from_circuit`.
    ErrorCandidateFinder(const Circuit &circuit, const DetectorErrorModel &filter);

    /// Base case for processing a single-term error mechanism.
    void err_atom(const Operation &effect, const ConstPointerRange<GateTarget> &pauli_terms);
    /// Processes operations with X, Y, Z errors on each target.
    void err_pauli_channel_1(const Operation &op);
    /// Processes operations with 15 two-qubit Pauli product errors on each target pair.
    void err_pauli_channel_2(const Operation &op);
    /// Processes measurement operations.
    void err_m(const Operation &op, uint32_t obs_mask);
    void err_xyz(const Operation &op, uint32_t target_flags);
    void rev_process_instruction(const Operation &op);
    void rev_process_circuit(uint64_t reps, const Circuit &block);
};

}  // namespace stim

#endif
