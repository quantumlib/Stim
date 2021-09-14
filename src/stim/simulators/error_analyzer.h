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
#include <queue>
#include <set>
#include <vector>

#include "stim/circuit/circuit.h"
#include "stim/dem/detector_error_model.h"
#include "stim/mem/fixed_cap_vector.h"
#include "stim/mem/monotonic_buffer.h"
#include "stim/mem/simd_util.h"
#include "stim/mem/sparse_xor_vec.h"

namespace stim {

struct ErrorAnalyzer {
    std::map<uint64_t, std::vector<DemTarget>> measurement_to_detectors;
    uint64_t total_detectors;
    uint64_t used_detectors;
    /// For each qubit, at the current time, the set of detectors with X dependence on that qubit.
    std::vector<SparseXorVec<DemTarget>> xs;
    /// For each qubit, at the current time, the set of detectors with Z dependence on that qubit.
    std::vector<SparseXorVec<DemTarget>> zs;
    size_t scheduled_measurement_time;
    bool decompose_errors;
    bool accumulate_errors;
    bool fold_loops;
    bool allow_gauge_detectors;
    double approximate_disjoint_errors_threshold;
    DetectorErrorModel flushed_reversed_model;

    /// The final result. Independent probabilities of flipping various sets of detectors.
    std::map<ConstPointerRange<DemTarget>, double> error_class_probabilities;
    /// Backing datastore for values in error_class_probabilities.
    MonotonicBuffer<DemTarget> mono_buf;

    ErrorAnalyzer(
        uint64_t num_detectors,
        size_t num_qubits,
        bool decompose_errors,
        bool fold_loops,
        bool allow_gauge_detectors,
        double approximate_disjoint_errors_threshold);

    static DetectorErrorModel circuit_to_detector_error_model(
        const Circuit &circuit,
        bool decompose_errors,
        bool fold_loops,
        bool allow_gauge_detectors,
        double approximate_disjoint_errors_threshold);

    /// Moving is deadly due to the map containing pointers to the jagged data.
    ErrorAnalyzer(const ErrorAnalyzer &analyzer) = delete;
    ErrorAnalyzer(ErrorAnalyzer &&analyzer) noexcept = delete;
    ErrorAnalyzer &operator=(ErrorAnalyzer &&analyzer) noexcept = delete;
    ErrorAnalyzer &operator=(const ErrorAnalyzer &analyzer) = delete;

    void SHIFT_COORDS(const OperationData &dat);
    void RX(const OperationData &dat);
    void RY(const OperationData &dat);
    void RZ(const OperationData &dat);
    void MX(const OperationData &dat);
    void MY(const OperationData &dat);
    void MZ(const OperationData &dat);
    void MPP(const OperationData &dat);
    void MRX(const OperationData &dat);
    void MRY(const OperationData &dat);
    void MRZ(const OperationData &dat);
    void H_XZ(const OperationData &dat);
    void H_XY(const OperationData &dat);
    void H_YZ(const OperationData &dat);
    void C_XYZ(const OperationData &dat);
    void C_ZYX(const OperationData &dat);
    void XCX(const OperationData &dat);
    void XCY(const OperationData &dat);
    void XCZ(const OperationData &dat);
    void YCX(const OperationData &dat);
    void YCY(const OperationData &dat);
    void YCZ(const OperationData &dat);
    void ZCX(const OperationData &dat);
    void ZCY(const OperationData &dat);
    void ZCZ(const OperationData &dat);
    void I(const OperationData &dat);

    void SQRT_XX(const OperationData &dat);
    void SQRT_YY(const OperationData &dat);
    void SQRT_ZZ(const OperationData &dat);

    void SWAP(const OperationData &dat);
    void DETECTOR(const OperationData &dat);
    void OBSERVABLE_INCLUDE(const OperationData &dat);
    void X_ERROR(const OperationData &dat);
    void Y_ERROR(const OperationData &dat);
    void Z_ERROR(const OperationData &dat);
    void CORRELATED_ERROR(const OperationData &dat);
    void DEPOLARIZE1(const OperationData &dat);
    void DEPOLARIZE2(const OperationData &dat);
    void ELSE_CORRELATED_ERROR(const OperationData &dat);
    void PAULI_CHANNEL_1(const OperationData &dat);
    void PAULI_CHANNEL_2(const OperationData &dat);
    void ISWAP(const OperationData &dat);

    void run_circuit(const Circuit &circuit);
    void post_check_initialization();

   private:
    /// When detectors anti-commute with a reset, that set of detectors becomes a degree of freedom.
    /// Use that degree of freedom to delete the largest detector in the set from the system.
    void remove_gauge(ConstPointerRange<DemTarget> sorted);
    /// Sorts the targets coming out of the measurement queue, then optionally inserts a measurement error.
    void xor_sort_measurement_error(std::vector<DemTarget> &queued, const OperationData &dat);
    void check_for_gauge(const SparseXorVec<DemTarget> &potential_gauge, const char *context);
    void check_for_gauge(
        SparseXorVec<DemTarget> &potential_gauge_summand_1,
        SparseXorVec<DemTarget> &potential_gauge_summand_2,
        const char *context);

    void shift_active_detector_ids(int64_t shift);
    void flush();
    void run_loop(const Circuit &loop, uint64_t iterations);
    ConstPointerRange<DemTarget> add_error(double probability, ConstPointerRange<DemTarget> flipped_sorted);
    ConstPointerRange<DemTarget> add_xored_error(
        double probability, ConstPointerRange<DemTarget> flipped1, ConstPointerRange<DemTarget> flipped2);
    ConstPointerRange<DemTarget> add_error_in_sorted_jagged_tail(double probability);
    void single_cx(uint32_t c, uint32_t t);
    void single_cy(uint32_t c, uint32_t t);
    void single_cz(uint32_t c, uint32_t t);
    void feedback(uint32_t record_control, size_t target, bool x, bool z);
    /// Saves the current tail of the monotonic buffer, deduping it to equal already stored data if possible.
    ///
    /// Returns:
    ///    A range over the stored data.
    ConstPointerRange<DemTarget> mono_dedupe_store_tail();
    /// Saves data to the monotonic buffer, deduping it to equal already stored data if possible.
    ///
    /// Args:
    ///     data: A range of data to store.
    ///
    /// Returns:
    ///    A range over the stored data.
    ConstPointerRange<DemTarget> mono_dedupe_store(ConstPointerRange<DemTarget> sorted);

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
        std::array<double, 1 << s> independent_probabilities, std::array<ConstPointerRange<DemTarget>, s> basis_errors);

    template <size_t s>
    void decompose_helper_add_error_combinations(
        const std::array<uint64_t, 1 << s> &detector_masks,
        std::array<ConstPointerRange<DemTarget>, 1 << s> &stored_ids);

    void decompose_and_append_component_to_tail(
        ConstPointerRange<DemTarget> component,
        const std::map<FixedCapVector<DemTarget, 2>, ConstPointerRange<DemTarget>> &known_symptoms);

    /// Performs a final check that all errors are decomposed.
    /// If any aren't, attempts to decompose them using other errors in the system.
    void do_global_error_decomposition_pass();

    // Checks whether there any errors that need decomposing.
    bool has_unflushed_ungraphlike_errors() const;
};

bool is_graphlike(const ConstPointerRange<DemTarget> &components);

}  // namespace stim

#endif
