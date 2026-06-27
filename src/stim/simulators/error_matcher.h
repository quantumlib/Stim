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

#ifndef _STIM_SIMULATORS_ERROR_MATCHER_H
#define _STIM_SIMULATORS_ERROR_MATCHER_H

#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/matched_error.h"

namespace stim {

/// This class handles matching circuit errors to detector-error-model errors.
struct ErrorMatcher {
    /// The error analyzer handles most of converting circuit errors into detector errors.
    ErrorAnalyzer error_analyzer;

    // This value is tweaked and adjusted while iterating through the circuit, tracking
    // where we currently are.
    CircuitErrorLocation cur_loc;
    const CircuitInstruction *cur_op;

    // Tracks discovered pairings keyed by their detector-error-model error terms.
    //
    // Pointed-to key data is owned by `dem_targets_buf``.
    std::map<SpanRef<const DemTarget>, ExplainedError> output_map;
    bool allow_adding_new_dem_errors_to_output_map;
    bool reduce_to_one_representative_error;

    std::map<uint64_t, std::vector<double>> dem_coords_map;
    std::map<uint64_t, std::vector<double>> qubit_coords_map;
    std::vector<double> cur_coord_offset;

    MonotonicBuffer<DemTarget> dem_targets_buf;
    uint64_t total_measurements_in_circuit;
    uint64_t total_ticks_in_circuit;

    // This class has pointers into its own data. Can't just copy it around!
    ErrorMatcher(const ErrorMatcher &) = delete;

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
    static std::vector<ExplainedError> explain_errors_from_circuit(
        const Circuit &circuit, const DetectorErrorModel *filter, bool reduce_to_one_representative_error);

    /// Constructs an error candidate finder based on parameters that are given to
    /// `ErrorCandidateFinder::explain_errors_from_circuit`.
    ErrorMatcher(const Circuit &circuit, const DetectorErrorModel *filter, bool reduce_to_one_representative_error);

    /// Looks up the coordinates of qubit/pauli terms, and appends into an output vector.
    void resolve_paulis_into(
        SpanRef<const GateTarget> targets, uint32_t target_flags, std::vector<GateTargetWithCoords> &out);

    /// Base case for processing a single-term error mechanism.
    void err_atom(const CircuitInstruction &effect);
    /// Processes operations with X, Y, Z errors on each target.
    void err_pauli_channel_1(const CircuitInstruction &op);
    /// Processes operations with M, X, Y, Z errors on each target.
    void err_heralded_pauli_channel_1(const CircuitInstruction &op);
    /// Processes operations with 15 two-qubit Pauli product errors on each target pair.
    void err_pauli_channel_2(const CircuitInstruction &op);
    /// Processes measurement operations.
    void err_m(const CircuitInstruction &op, uint32_t obs_mask);
    void err_xyz(const CircuitInstruction &op, uint32_t target_flags);

    /// Processes arbitrary instructions.
    void rev_process_instruction(const CircuitInstruction &op);
    /// Processes entire circuits.
    void rev_process_circuit(uint64_t reps, const Circuit &block);

    void add_dem_error(ErrorEquivalenceClass dem_error);
};

}  // namespace stim

#endif
