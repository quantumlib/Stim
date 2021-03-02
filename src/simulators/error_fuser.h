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

#include <map>
#include <memory>
#include <queue>
#include <vector>

#include "../circuit/circuit.h"
#include "../simd/sparse_xor_vec.h"

struct ErrorFuser {
    std::map<uint32_t, std::vector<uint32_t>> measurement_to_detectors;
    uint32_t num_found_detectors = 0;
    uint32_t num_found_observables = 0;
    /// For each qubit, at the current time, the set of detectors with X dependence on that qubit.
    std::vector<SparseXorVec<uint32_t>> xs;
    /// For each qubit, at the current time, the set of detectors with Z dependence on that qubit.
    std::vector<SparseXorVec<uint32_t>> zs;
    size_t scheduled_measurement_time = 0;

    /// The final result. Independent probabilities of flipping various sets of detectors.
    ///
    /// The backing data for the vector views is in the `jagged_data` field.
    std::map<VectorView<uint32_t>, double> error_class_probabilities;
    JaggedDataArena<uint32_t> jagged_detector_sets;

    ErrorFuser(size_t num_qubits);

    static void convert_circuit_out(const Circuit &circuit, FILE *out);

    /// Moving is deadly due to the map containing pointers to the jagged data.
    ErrorFuser(const ErrorFuser &fuser) = delete;
    ErrorFuser(ErrorFuser &&fuser) noexcept = delete;
    ErrorFuser &operator=(ErrorFuser &&fuser) noexcept = delete;
    ErrorFuser &operator=(const ErrorFuser &fuser) = delete;

    void R(const OperationData &dat);
    void M(const OperationData &dat);
    void MR(const OperationData &dat);
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
    void independent_error_1(double probability, const SparseXorVec<uint32_t> &detector_set);
    void independent_error_1(double probability, const uint32_t *begin, size_t size);
    void independent_error_2(double probability, const SparseXorVec<uint32_t> &d1, const SparseXorVec<uint32_t> &d2);
    void independent_error_placed_tail(double probability, VectorView<uint32_t> detector_set);
    void single_cx(uint32_t c, uint32_t t);
    void single_cy(uint32_t c, uint32_t t);
    void single_cz(uint32_t c, uint32_t t);
    void feedback(uint32_t record_control, size_t target, bool x, bool z);
    void run_circuit(const Circuit &circuit);
};

#endif
