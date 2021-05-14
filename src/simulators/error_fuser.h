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
#include "../simd/sparse_xor_vec.h"

namespace stim_internal {

struct ErrorFuser {
    std::map<uint32_t, std::vector<uint32_t>> measurement_to_detectors;
    uint32_t num_found_detectors = 0;
    uint32_t num_found_observables = 0;
    /// For each qubit, at the current time, the set of detectors with X dependence on that qubit.
    std::vector<SparseXorVec<uint32_t>> xs;
    /// For each qubit, at the current time, the set of detectors with Z dependence on that qubit.
    std::vector<SparseXorVec<uint32_t>> zs;
    size_t scheduled_measurement_time = 0;
    bool use_basis_analysis;

    std::set<ConstPointerRange<uint32_t>> edges;

    /// The final result. Independent probabilities of flipping various sets of detectors.
    std::map<PointerRange<uint32_t>, double> error_class_probabilities;
    /// Backing datastore for values in error_class_probabilities.
    MonotonicBuffer<uint32_t> jag_flip_data;

    ErrorFuser(size_t num_qubits, bool use_basis_analysis);

    static void convert_circuit_out(const Circuit &circuit, FILE *out, bool use_basis_analysis);

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

   private:
    PointerRange<uint32_t> add_error(double probability, const SparseXorVec<uint32_t> &data, bool is_basis_error);
    PointerRange<uint32_t> add_xored_error(
        double probability, const SparseXorVec<uint32_t> &flipped1, const SparseXorVec<uint32_t> &flipped2, bool is_basis_error);
    PointerRange<uint32_t> add_error_in_sorted_jagged_tail(double probability, bool is_basis_error);
    void single_cx(uint32_t c, uint32_t t);
    void single_cy(uint32_t c, uint32_t t);
    void single_cz(uint32_t c, uint32_t t);
    void feedback(uint32_t record_control, size_t target, bool x, bool z);
    void run_circuit(const Circuit &circuit);

    template <size_t N>
    void add_irreducible_edges(std::array<ConstPointerRange<uint32_t>, N> &candidates) {
        if (!use_basis_analysis) {
            return;
        }

        std::array<ConstPointerRange<uint32_t>, N> kept;
        size_t num_kept = 0;
        std::sort(candidates.begin(),
                  candidates.end(),
                  [](const ConstPointerRange<uint32_t> &v1, const ConstPointerRange<uint32_t> &v2) {
                      return v1.size() < v2.size();
                  });

        for (const auto &c : candidates) {
            bool found = false;
            for (size_t k1 = 0; k1 < num_kept; k1++) {
                if (c == kept[k1]) {
                    found = true;
                }
                for (size_t k2 = k1 + 1; k2 < num_kept; k2++) {
                    xor_merge_sort_temp_buffer_callback(kept[k1], kept[k2], [&](ConstPointerRange<uint32_t> result){
                        if (c == result) {
                            found = true;
                        }
                    });
                }
            }
            if (!found) {
                kept[num_kept] = c;
                num_kept++;
                edges.insert(c);
            }
        }
    }
};

}

#endif
