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

#ifndef _STIM_SIMULATORS_ERROR_ANALYZER_H
#define _STIM_SIMULATORS_ERROR_ANALYZER_H

#include <algorithm>
#include <map>
#include <memory>
#include <stim/stabilizers/pauli_string.h>
#include <vector>

#include "sparse_rev_frame_tracker.h"
#include "stim/circuit/circuit.h"
#include "stim/dem/detector_error_model.h"
#include "stim/mem/fixed_cap_vector.h"
#include "stim/mem/monotonic_buffer.h"
#include "stim/mem/sparse_xor_vec.h"

namespace stim {

struct ErrorEquivalenceClass {
    SpanRef<const DemTarget> targets;
    std::string_view tag;

    inline bool operator==(const ErrorEquivalenceClass &other) const {
        return targets == other.targets && tag == other.tag;
    }
    inline bool operator!=(const ErrorEquivalenceClass &other) const {
        return !(*this == other);
    }
    inline bool operator<(const ErrorEquivalenceClass &other) const {
        if (targets != other.targets) {
            return targets < other.targets;
        }
        if (tag != other.tag) {
            return tag < other.tag;
        }
        return false;
    }
};

/// This class is responsible for iterating backwards over a circuit, tracking which detectors are currently
/// sensitive to an X or Z error on each qubit. This is done by having a SparseXorVec for the X and Z
/// sensitivities of each qubit, and transforming these collections in response to operations.
/// For example, applying a CNOT gate from qubit A to qubit B will xor the X sensitivity of A into B and the
/// Z sensitivity of B into A.
///
/// Y error sensitivity is always implicit in the xor of the X and Z components.
///
/// Note that the class iterates over the circuit *backward*, so that DETECTOR instructions are seen before
/// the measurement operations that the detector depends on. When a DETECTOR is seen, it is noted which
/// measurements it depends on and then when those measurements are seen the detector is added as one of the
/// things sensitive to errors that anticommute with the measurement.
///
/// Be wary that this class is definitely one of the more complex things in Stim. For example:
/// - There is a monobuf that is often used as a temporary buffer to avoid allocations, meaning
///     the state and meaning of the monobuf's tail is highly coupled to what method is currently
///     executing.
/// - The class recursively uses itself when performing period finding on loops. It is seriously
///     hard to directly inspect and understand the current state when period finding is happening
///     inside of period finding.
/// - When period finding succeeds it flushes the recorded errors to avoid crosstalk between the
///     in-loop errors and out-of-loop errors. The state of the buffers is coupled across features.
/// - Error decomposition is done using heuristics that are not guaranteed to work, and prone to
///     being tweaked, creating churn. Also the decomposition code itself is quite complex in order
///     to make it fast (e.g. reducing the explicit detectors into bitmasks and then working with
///     the bitmasks).
/// - I guess what I'm saying is... have fun!
struct ErrorAnalyzer {
    SparseUnsignedRevFrameTracker tracker;

    /// When false, no error decomposition is performed.
    /// When true, must decompose any non-graphlike error into graphlike components or fail.
    bool decompose_errors;

    /// When false, errors are skipped over instead of recorded.
    /// When true, errors are recorded into the error_class_probabilities dictionary.
    bool accumulate_errors;

    /// When false, loops are flattened and directly iterated over.
    /// When true, a tortoise-and-hare algorithm is used to notice periodicity in the errors.
    /// If periodicity is found, the rest of the loop becomes a loop in the output error model.
    bool fold_loops;

    /// When false, detectors with non-deterministic parities under noiseless execution cause failure.
    /// When true, the non-determinism is translated into a 50/50 random error mechanisms.
    bool allow_gauge_detectors;

    /// Determines how small the probabilities in disjoint error mechanisms like PAULI_CHANNEL_2 must be
    /// before they can be approximated as being independent. Any larger probabilities cause failure.
    double approximate_disjoint_errors_threshold;

    /// When true, errors that fail to decompose are inserted into the output
    /// undecomposed instead of raising an exception that terminates the
    /// conversion from circuit to detector error model.
    ///
    /// Only relevant when decompose_errors=True.
    bool ignore_decomposition_failures;

    /// When true, decomposition is permitted to split A B C D into A B ^ C D
    /// when only one of A B or C D exists elsewhere, instead of requiring both
    /// to exist. This can reduce the code distance of the decoding graph, but
    /// is sometimes necessary.
    ///
    /// Only relevant when decompose_errors=True.
    bool block_decomposition_from_introducing_remnant_edges;

    /// A buffer containing the growing output error model as the circuit is traversed.
    /// The buffer is in reverse order because the circuit is traversed back to front.
    /// Certain events during period solving of loops can cause the error probabilities
    /// to flush into this buffer.
    DetectorErrorModel flushed_reversed_model;

    /// Recorded errors. Independent probabilities of flipping various sets of detectors.
    std::map<ErrorEquivalenceClass, double> error_class_probabilities;
    /// Backing datastore for values in error_class_probabilities.
    MonotonicBuffer<DemTarget> mono_buf;

    /// Counts the number of tick operations, for better debug messages.
    uint64_t num_ticks_in_past = 0;

    /// Used for producing debug information when errors occur.
    const Circuit *current_circuit_being_analyzed = nullptr;

    /// Creates an instance ready to start processing instructions from a circuit of known size.
    ErrorAnalyzer(
        uint64_t num_measurements,
        uint64_t num_detectors,
        size_t num_qubits,
        uint64_t num_ticks,
        bool decompose_errors,
        bool fold_loops,
        bool allow_gauge_detectors,
        double approximate_disjoint_errors_threshold,
        bool ignore_decomposition_failures,
        bool block_decomposition_from_introducing_remnant_edges);

    /// Returns the detector error model of the given circuit.
    ///
    /// Args:
    ///     circuit: The circuit to analyze.
    ///     decompose_errors: When true, complex errors must be split into graphlike components.
    ///     fold_loops: When true, use a tortoise-and-hare algorithm to solve loops instead of flattening them.
    ///     allow_gauge_detectors: When true, replace non-deterministic detectors with 50/50 error mechanisms instead
    ///         of failing.
    ///     approximate_disjoint_errors_threshold: When larger than 0, allows disjoint errors like PAULI_CHANNEL_2 to
    ///         be present in the circuit, as long as their probabilities are not larger than this.
    ///     ignore_decomposition_failures: Determines whether errors that that fail to decompose are inserted into the
    ///         output, or cause the conversion to fail and raise an exception.
    ///     block_decomposition_from_introducing_remnant_edges: When true, it is not permitted to decompose A B C D
    ///         into A B ^ C D unless both A B and C D appear elsewhere in the error model. When false, only one has
    ///         to appear elsewhere.
    ///
    /// Returns:
    ///     The detector error model.
    static DetectorErrorModel circuit_to_detector_error_model(
        const Circuit &circuit,
        bool decompose_errors,
        bool fold_loops,
        bool allow_gauge_detectors,
        double approximate_disjoint_errors_threshold,
        bool ignore_decomposition_failures,
        bool block_decomposition_from_introducing_remnant_edges);

    /// Copying is unsafe because `error_class_probabilities` has overlapping pointers to `monobuf`'s internals.
    ErrorAnalyzer(const ErrorAnalyzer &analyzer) = delete;
    ErrorAnalyzer(ErrorAnalyzer &&analyzer) noexcept = delete;
    ErrorAnalyzer &operator=(ErrorAnalyzer &&analyzer) noexcept = delete;
    ErrorAnalyzer &operator=(const ErrorAnalyzer &analyzer) = delete;

    void undo_SHIFT_COORDS(const CircuitInstruction &inst);
    void undo_RX(const CircuitInstruction &inst);
    void undo_RY(const CircuitInstruction &inst);
    void undo_RZ(const CircuitInstruction &inst);
    void undo_MX(const CircuitInstruction &inst);
    void undo_MY(const CircuitInstruction &inst);
    void undo_MZ(const CircuitInstruction &inst);
    void undo_MPP(const CircuitInstruction &inst);
    void undo_SPP(const CircuitInstruction &inst);
    void undo_MXX(const CircuitInstruction &inst);
    void undo_MYY(const CircuitInstruction &inst);
    void undo_MZZ(const CircuitInstruction &inst);
    void undo_MPAD(const CircuitInstruction &inst);
    void undo_HERALDED_ERASE(const CircuitInstruction &inst);
    void undo_HERALDED_PAULI_CHANNEL_1(const CircuitInstruction &inst);
    void undo_MRX(const CircuitInstruction &inst);
    void undo_MRY(const CircuitInstruction &inst);
    void undo_MRZ(const CircuitInstruction &inst);
    void undo_H_XZ(const CircuitInstruction &inst);
    void undo_H_XY(const CircuitInstruction &inst);
    void undo_H_YZ(const CircuitInstruction &inst);
    void undo_C_XYZ(const CircuitInstruction &inst);
    void undo_C_ZYX(const CircuitInstruction &inst);
    void undo_XCX(const CircuitInstruction &inst);
    void undo_XCY(const CircuitInstruction &inst);
    void undo_XCZ(const CircuitInstruction &inst);
    void undo_YCX(const CircuitInstruction &inst);
    void undo_YCY(const CircuitInstruction &inst);
    void undo_YCZ(const CircuitInstruction &inst);
    void undo_ZCX(const CircuitInstruction &inst);
    void undo_ZCY(const CircuitInstruction &inst);
    void undo_ZCZ(const CircuitInstruction &inst);
    void undo_I(const CircuitInstruction &inst);
    void undo_TICK(const CircuitInstruction &inst);
    void undo_SQRT_XX(const CircuitInstruction &inst);
    void undo_SQRT_YY(const CircuitInstruction &inst);
    void undo_SQRT_ZZ(const CircuitInstruction &inst);
    void undo_SWAP(const CircuitInstruction &inst);
    void undo_DETECTOR(const CircuitInstruction &inst);
    void undo_OBSERVABLE_INCLUDE(const CircuitInstruction &inst);
    void undo_X_ERROR(const CircuitInstruction &inst);
    void undo_Y_ERROR(const CircuitInstruction &inst);
    void undo_Z_ERROR(const CircuitInstruction &inst);
    void undo_CORRELATED_ERROR(const CircuitInstruction &inst);
    void undo_DEPOLARIZE1(const CircuitInstruction &inst);
    void undo_DEPOLARIZE2(const CircuitInstruction &inst);
    void undo_ELSE_CORRELATED_ERROR(const CircuitInstruction &inst);
    void undo_PAULI_CHANNEL_1(const CircuitInstruction &inst);
    void undo_PAULI_CHANNEL_2(const CircuitInstruction &inst);
    void undo_ISWAP(const CircuitInstruction &inst);
    void undo_CXSWAP(const CircuitInstruction &inst);
    void undo_CZSWAP(const CircuitInstruction &inst);
    void undo_SWAPCX(const CircuitInstruction &inst);

    void undo_RX_with_context(const CircuitInstruction &inst, const char *context_op);
    void undo_RY_with_context(const CircuitInstruction &inst, const char *context_op);
    void undo_RZ_with_context(const CircuitInstruction &inst, const char *context_op);
    void undo_MX_with_context(const CircuitInstruction &inst, const char *context_op);
    void undo_MY_with_context(const CircuitInstruction &inst, const char *context_op);
    void undo_MZ_with_context(const CircuitInstruction &inst, const char *context_op);

    /// Processes each of the instructions in the circuit, in reverse order.
    void undo_circuit(const Circuit &circuit);
    /// This is used at the end of the analysis to check that any remaining sensitivities commute
    /// with the implicit Z basis initialization at the start of a circuit.
    void post_check_initialization();

    void undo_gate(const CircuitInstruction &inst);

    /// Returns a PauliString indicating the current error sensitivity of a detector or observable.
    ///
    /// The observable or detector is sensitive to the Pauli error P at q if the Pauli sensitivity
    /// at q anti-commutes with P.
    PauliString<MAX_BITWORD_WIDTH> current_error_sensitivity_for(DemTarget t) const;

    /// Processes the instructions in a circuit multiple times.
    /// If loop folding is enabled, also uses a tortoise-and-hare algorithm to attempt to solve the loop's period.
    void run_loop(const Circuit &loop, uint64_t iterations, std::string_view tag);

   private:
    /// When detectors anti-commute with a reset, that set of detectors becomes a degree of freedom.
    /// Use that degree of freedom to delete the largest detector in the set from the system.
    void remove_gauge(SpanRef<const DemTarget> sorted);
    /// Sorts the targets coming out of the measurement queue, then optionally inserts a measurement error.
    void xor_sorted_measurement_error(SpanRef<const DemTarget> targets, const CircuitInstruction &dat);
    /// Checks if the given sparse vector is empty. If it isn't, something that was supposed to be
    /// deterministic is actually random. Produces an error message with debug information that can be
    /// used to understand what went wrong.
    void check_for_gauge(
        const SparseXorVec<DemTarget> &potential_gauge, const char *context_op, uint64_t context_qubit, std::string_view tag);
    /// Checks if the given sparse vectors are equal. If they aren't, something that was supposed to be
    /// deterministic is actually random. Produces an error message with debug information that can be
    /// used to understand what went wrong.
    void check_for_gauge(
        SparseXorVec<DemTarget> &potential_gauge_summand_1,
        const SparseXorVec<DemTarget> &potential_gauge_summand_2,
        const char *context_op,
        uint64_t context_qubit,
        std::string_view tag);

    /// Empties error_class_probabilities into flushed_reversed_model.
    void flush();
    /// Adds (or folds) an error mechanism into error_class_probabilities.
    ErrorEquivalenceClass add_error(double probability, SpanRef<const DemTarget> flipped_sorted, std::string_view tag);
    /// Adds (or folds) an error mechanism equal into error_class_probabilities.
    /// The error is defined as the xor of two sparse vectors, because this is a common situation.
    /// Deals with the details of efficiently computing the xor of the vectors with minimal allocations.
    ErrorEquivalenceClass add_xored_error(
        double probability, SpanRef<const DemTarget> flipped1, SpanRef<const DemTarget> flipped2, std::string_view tag);
    /// Adds an error mechanism into error_class_probabilities.
    /// The error mechanism is not passed as an argument but is instead the current tail of `this->mono_buf`.
    ErrorEquivalenceClass add_error_in_sorted_jagged_tail(double probability, std::string_view tag);
    /// Saves the current tail of the monotonic buffer, deduping it to equal already stored data if possible.
    ///
    /// Returns:
    ///    A range over the stored data.
    ErrorEquivalenceClass mono_dedupe_store_tail(std::string_view tag);
    /// Saves data to the monotonic buffer, deduping it to equal already stored data if possible.
    ///
    /// Args:
    ///     data: A range of data to store.
    ///
    /// Returns:
    ///    A range over the stored data.
    ErrorEquivalenceClass mono_dedupe_store(ErrorEquivalenceClass sorted);

    /// Adds each given error, and also each possible combination of the given errors, to the possible errors.
    ///
    /// Does analysis of which errors reduce to other errors (in the detector basis, not the given basis).
    ///
    /// Args:
    ///     independent_probabilities: Probability of each error combination (including but ignoring the empty
    ///         combination) occurring, independent of whether or not the others occurred.
    ///     basis_errors: Building blocks for the error combinations.
    template <size_t s>
    void add_error_combinations(
        std::array<double, 1 << s> probabilities,
        std::array<SpanRef<const DemTarget>, s> basis_errors,
        bool probabilities_are_disjoint,
        std::string_view tag);

    /// Handles local decomposition of errors.
    /// When an error has multiple channels, eg. a DEPOLARIZE2 error, this method attempts to express the more complex
    /// channels (the ones with more symptoms) in terms of the simpler ones that have just 1 or 2 symptoms.
    /// Works by rewriting the `stored_ids` argument.
    template <size_t s>
    void decompose_helper_add_error_combinations(
        const std::array<uint64_t, 1 << s> &detector_masks, std::array<SpanRef<const DemTarget>, 1 << s> &stored_ids, std::string_view tag);

    /// Handles global decomposition of errors.
    /// When an error has more than two symptoms, this method attempts to find other known errors that can be used as
    /// components of this error, so that it is decomposed into graphlike components.
    bool decompose_and_append_component_to_tail(
        SpanRef<const DemTarget> component,
        const std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> &known_symptoms);

    /// Performs a final check that all errors are decomposed.
    /// If any aren't, attempts to decompose them using other errors in the system.
    void do_global_error_decomposition_pass();

    /// Checks whether there any errors that need decomposing.
    bool has_unflushed_ungraphlike_errors() const;

   private:
    void undo_MXX_disjoint_controls_segment(const CircuitInstruction &inst);
    void undo_MYY_disjoint_controls_segment(const CircuitInstruction &inst);
    void undo_MZZ_disjoint_controls_segment(const CircuitInstruction &inst);
    void check_can_approximate_disjoint(
        const char *op_name, SpanRef<const double> probabilities, bool allow_single_component) const;
    void add_composite_error(double probability, SpanRef<const GateTarget> targets, std::string_view tag);
    void correlated_error_block(const std::vector<CircuitInstruction> &dats);
};

/// Determines if an error's targets are graphlike.
///
/// An error is graphlike if it has at most two symptoms (two detectors) per component.
/// For example, error(0.1) D0 D1 ^ D2 D3 L55 is graphlike but error(0.1) D0 D1 ^ D2 D3 D55 is not.
bool is_graphlike(const SpanRef<const DemTarget> &components);

bool brute_force_decomposition_into_known_graphlike_errors(
    SpanRef<const DemTarget> problem,
    const std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> &known_graphlike_errors,
    MonotonicBuffer<DemTarget> &output);

}  // namespace stim

#endif
