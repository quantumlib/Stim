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

#ifndef STIM_ERROR_ANALYZER_H
#define STIM_ERROR_ANALYZER_H

#include <algorithm>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

#include "../circuit/circuit.h"
#include "../dem/detector_error_model.h"
#include "../simd/fixed_cap_vector.h"
#include "../simd/monotonic_buffer.h"
#include "../simd/simd_util.h"
#include "../simd/sparse_xor_vec.h"

namespace stim_internal {

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
    void check_for_gauge(const SparseXorVec<DemTarget> &potential_gauge);
    void check_for_gauge(
        SparseXorVec<DemTarget> &potential_gauge_summand_1, SparseXorVec<DemTarget> &potential_gauge_summand_2);

    void shift_active_detector_ids(int64_t shift);
    void flush();
    void run_loop(const Circuit &loop, uint64_t iterations);
    ConstPointerRange<DemTarget> add_error(double probability, ConstPointerRange<DemTarget> data);
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
    ConstPointerRange<DemTarget> mono_dedupe_store(ConstPointerRange<DemTarget> data);

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
        std::array<double, 1 << s> independent_probabilities,
        std::array<ConstPointerRange<DemTarget>, s> basis_errors) {
        // Determine involved detectors while creating basis masks and storing added data.
        FixedCapVector<DemTarget, 16> involved_detectors{};
        std::array<uint64_t, 1 << s> detector_masks{};
        std::array<ConstPointerRange<DemTarget>, 1 << s> stored_ids;
        for (size_t k = 0; k < s; k++) {
            for (const auto &id : basis_errors[k]) {
                if (id.is_relative_detector_id()) {
                    auto r = involved_detectors.find(id);
                    if (r == involved_detectors.end()) {
                        try {
                            involved_detectors.push_back(id);
                        } catch (const std::out_of_range &ex) {
                            throw std::out_of_range(
                                "An error involves too many detectors (>15) to find reducible errors.");
                        }
                    }
                    detector_masks[1 << k] ^= 1 << (r - involved_detectors.begin());
                }
            }
            stored_ids[1 << k] = mono_dedupe_store(basis_errors[k]);
        }

        // Fill in all 2**s - 1 possible combinations from the initial basis values.
        for (size_t k = 3; k < 1 << s; k++) {
            auto c1 = k & (k - 1);
            auto c2 = k ^ c1;
            if (c1) {
                mono_buf.ensure_available(stored_ids[c1].size() + stored_ids[c2].size());
                mono_buf.tail.ptr_end = xor_merge_sort(stored_ids[c1], stored_ids[c2], mono_buf.tail.ptr_end);
                stored_ids[k] = mono_dedupe_store_tail();
                detector_masks[k] = detector_masks[c1] ^ detector_masks[c2];
            }
        }

        if (decompose_errors) {
            // Count number of detectors affected by each error.
            std::array<uint8_t, 1 << s> detector_counts{};
            for (size_t k = 1; k < 1 << s; k++) {
                detector_counts[k] = popcnt64(detector_masks[k]);
            }

            // Find single-detector errors (and empty errors).
            uint64_t solved = 0;
            uint64_t single_detectors_union = 0;
            for (size_t k = 1; k < 1 << s; k++) {
                if (detector_counts[k] == 1) {
                    single_detectors_union |= detector_masks[k];
                    solved |= 1 << k;
                }
            }

            // Find irreducible double-detector errors.
            FixedCapVector<uint8_t, 1 << s> irreducible_pairs{};
            for (size_t k = 1; k < 1 << s; k++) {
                if (detector_counts[k] == 2 && (detector_masks[k] & ~single_detectors_union)) {
                    irreducible_pairs.push_back(k);
                    solved |= 1 << k;
                }
            }

            auto append_involved_pairs_to_jag_tail = [&](size_t goal_k) {
                uint64_t goal = detector_masks[goal_k];

                // If single-detector excitations are sufficient, just use those.
                if ((goal & ~single_detectors_union) == 0) {
                    return goal;
                }

                // Check if one double-detector excitation can get us into the single-detector region.
                for (auto k : irreducible_pairs) {
                    auto m = detector_masks[k];
                    if ((goal & m) == m && (goal & ~(single_detectors_union | m)) == 0) {
                        mono_buf.append_tail(stored_ids[k]);
                        mono_buf.append_tail(DemTarget::separator());
                        return goal & ~m;
                    }
                }

                // Check if two double-detector excitations can get us into the single-detector region.
                for (size_t i1 = 0; i1 < irreducible_pairs.size(); i1++) {
                    auto k1 = irreducible_pairs[i1];
                    auto m1 = detector_masks[k1];
                    for (size_t i2 = i1 + 1; i2 < irreducible_pairs.size(); i2++) {
                        auto k2 = irreducible_pairs[i2];
                        auto m2 = detector_masks[k2];
                        if ((m1 & m2) == 0 && (goal & ~(single_detectors_union | m1 | m2)) == 0) {
                            if (stored_ids[k2] < stored_ids[k1]) {
                                std::swap(k1, k2);
                            }
                            mono_buf.append_tail(stored_ids[k1]);
                            mono_buf.append_tail(DemTarget::separator());
                            mono_buf.append_tail(stored_ids[k2]);
                            mono_buf.append_tail(DemTarget::separator());
                            return goal & ~(m1 | m2);
                        }
                    }
                }

                throw std::out_of_range(
                    "Failed to reduce an error with more than 2 detection events into single-detection errors "
                    "and at most 2 double-detection errors.");
            };

            // Solve the decomposition of each composite case.
            for (size_t k = 1; k < 1 << s; k++) {
                if (detector_counts[k] && ((solved >> k) & 1) == 0) {
                    auto remnants = append_involved_pairs_to_jag_tail(k);

                    // Finish off the solution using single-detector components.
                    for (size_t k2 = 0; remnants && k2 < 1 << s; k2++) {
                        if (detector_counts[k2] == 1 && (detector_masks[k2] & ~remnants) == 0) {
                            remnants &= ~detector_masks[k2];
                            mono_buf.append_tail(stored_ids[k2]);
                            mono_buf.append_tail(DemTarget::separator());
                        }
                    }
                    if (!mono_buf.tail.empty()) {
                        mono_buf.tail.ptr_end -= 1;
                    }
                    stored_ids[k] = mono_dedupe_store_tail();
                }
            }
        }

        // Include errors in the record.
        for (size_t k = 1; k < 1 << s; k++) {
            add_error(independent_probabilities[k], stored_ids[k]);
        }
    }
};

}  // namespace stim_internal

#endif
