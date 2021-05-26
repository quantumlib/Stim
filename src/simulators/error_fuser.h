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

#ifndef SPARSE_BITS_H
#define SPARSE_BITS_H

#include <algorithm>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <vector>

#include "../circuit/circuit.h"
#include "../simd/monotonic_buffer.h"
#include "../simd/fixed_cap_vector.h"
#include "../simd/sparse_xor_vec.h"
#include "../simd/simd_util.h"

namespace stim_internal {

constexpr uint64_t FIRST_OBSERVABLE_ID = uint64_t{1} << 63;
constexpr uint64_t LAST_DETECTOR_ID = FIRST_OBSERVABLE_ID - 1;
constexpr uint64_t COMPOSITE_ERROR_SYGIL = UINT64_MAX;

bool is_encoded_detector_id(uint64_t id);
bool is_encoded_observable_id(uint64_t id);

struct FusedErrorRepeatBlock;

struct FusedError {
    double probability;
    ConstPointerRange<uint64_t> flipped;
    uint64_t local_time_shift;
    std::unique_ptr<FusedErrorRepeatBlock> block;
    void print(FILE *out, size_t indent, uint64_t num_found_detectors, uint64_t &tick_count) const;
    void skip(uint64_t skipped);
};

struct FusedErrorRepeatBlock {
    uint64_t repetitions;
    size_t total_ticks_per_iteration_including_sub_loops;
    size_t outer_ticks_per_iteration() const;
    std::vector<FusedError> errors;
    void print(FILE *out, size_t indent, uint64_t num_found_detectors, uint64_t &time_offset) const;
    void skip(uint64_t skipped);
};

struct ErrorFuser {
    std::map<uint64_t, std::vector<uint64_t>> measurement_to_detectors;
    uint64_t num_found_detectors = 0;
    uint64_t num_found_observables = 0;
    /// For each qubit, at the current time, the set of detectors with X dependence on that qubit.
    std::vector<SparseXorVec<uint64_t>> xs;
    /// For each qubit, at the current time, the set of detectors with Z dependence on that qubit.
    std::vector<SparseXorVec<uint64_t>> zs;
    size_t scheduled_measurement_time = 0;
    bool find_reducible_errors = false;
    bool accumulate_errors = true;
    bool fold_loops = false;
    std::vector<FusedError> flushed;

    /// The final result. Independent probabilities of flipping various sets of detectors.
    std::map<ConstPointerRange<uint64_t>, double> error_class_probabilities;
    /// Backing datastore for values in error_class_probabilities.
    MonotonicBuffer<uint64_t> mono_buf;

    ErrorFuser(size_t num_qubits, bool find_reducible_errors, bool fold_loops);

    static void convert_circuit_out(const Circuit &circuit, FILE *out, bool find_reducible_errors, bool fold_loops);

    /// Moving is deadly due to the map containing pointers to the jagged data.
    ErrorFuser(const ErrorFuser &fuser) = delete;
    ErrorFuser(ErrorFuser &&fuser) noexcept = delete;
    ErrorFuser &operator=(ErrorFuser &&fuser) noexcept = delete;
    ErrorFuser &operator=(const ErrorFuser &fuser) = delete;

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
    void ISWAP(const OperationData &dat);

    void run_circuit(const Circuit &circuit);

   private:
    void shift_active_detector_ids(int64_t shift);
    void print_flushed(FILE *out) const;
    void flush();
    void run_loop(const Circuit &loop, uint64_t iterations);
    ConstPointerRange<uint64_t> add_error(double probability, ConstPointerRange<uint64_t> data);
    ConstPointerRange<uint64_t> add_xored_error(
        double probability, ConstPointerRange<uint64_t> flipped1, ConstPointerRange<uint64_t> flipped2);
    ConstPointerRange<uint64_t> add_error_in_sorted_jagged_tail(double probability);
    void single_cx(uint32_t c, uint32_t t);
    void single_cy(uint32_t c, uint32_t t);
    void single_cz(uint32_t c, uint32_t t);
    void feedback(uint32_t record_control, size_t target, bool x, bool z);
    /// Saves the current tail of the monotonic buffer, deduping it to equal already stored data if possible.
    ///
    /// Returns:
    ///    A range over the stored data.
    ConstPointerRange<uint64_t> mono_dedupe_store_tail();
    /// Saves data to the monotonic buffer, deduping it to equal already stored data if possible.
    ///
    /// Args:
    ///     data: A range of data to store.
    ///
    /// Returns:
    ///    A range over the stored data.
    ConstPointerRange<uint64_t> mono_dedupe_store(ConstPointerRange<uint64_t> data);

    /// Adds each given error, and also each possible combination of the given errors, to the possible errors.
    ///
    /// Does analysis of which errors reduce to other errors (in the detector basis, not the given basis).
    ///
    /// Args:
    ///     p: Independent probability of each error combination (other than the empty combination) occurring.
    ///     basis_errors: Building blocks for the error combinations.
    template <size_t s>
    void add_error_combinations(double p, std::array<ConstPointerRange<uint64_t>, s> basis_errors) {
        // Determine involved detectors while creating basis masks and storing added data.
        FixedCapVector<uint64_t, 8> involved_detectors{};
        std::array<uint64_t, 1 << s> detector_masks{};
        std::array<ConstPointerRange<uint64_t>, 1 << s> stored_ids;
        for (size_t k = 0; k < s; k++) {
            for (auto id : basis_errors[k]) {
                if (is_encoded_detector_id(id)) {
                    auto r = involved_detectors.find(id);
                    if (r == involved_detectors.end()) {
                        involved_detectors.push_back(id);
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

        if (find_reducible_errors) {
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
                        mono_buf.append_tail(COMPOSITE_ERROR_SYGIL);
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
                            mono_buf.append_tail(COMPOSITE_ERROR_SYGIL);
                            mono_buf.append_tail(stored_ids[k2]);
                            mono_buf.append_tail(COMPOSITE_ERROR_SYGIL);
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
                            mono_buf.append_tail(COMPOSITE_ERROR_SYGIL);
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
            add_error(p, stored_ids[k]);
        }
    }
};

void error_fuser_analyze_loop(const Circuit &behind, uint64_t ahead, ErrorFuser &f);

}

#endif
