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

#ifndef _STIM_SIMULATORS_SPARSE_REV_FRAME_TRACKER_H
#define _STIM_SIMULATORS_SPARSE_REV_FRAME_TRACKER_H

#include "stim/circuit/circuit.h"
#include "stim/mem/sparse_xor_vec.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau.h"
#include "stim/dem/detector_error_model.h"

namespace stim {

/// Tracks pauli frames through the circuit, assuming few frames depend on any one qubit.
struct SparseUnsignedRevFrameTracker {
    /// Per qubit, what terms have X basis dependence on this qubit.
    std::vector<SparseXorVec<DemTarget>> xs;
    /// Per qubit, what terms have Z basis dependence on this qubit.
    std::vector<SparseXorVec<DemTarget>> zs;
    /// Per classical bit, what terms have dependence on measurement bits.
    std::map<uint64_t, SparseXorVec<DemTarget>> rec_bits;
    /// Number of measurements that have not yet been processed.
    uint64_t num_measurements_in_past;
    /// Number of detectors that have not yet been processed.
    uint64_t num_detectors_in_past;

    SparseUnsignedRevFrameTracker(
        uint64_t num_qubits,
        uint64_t num_measurements_in_past,
        uint64_t num_detectors_in_past);

    PauliString current_error_sensitivity_for(DemTarget target) const;

    void handle_xor_gauge(ConstPointerRange<DemTarget> sorted1, ConstPointerRange<DemTarget> sorted2);
    void handle_gauge(ConstPointerRange<DemTarget> sorted);
    void undo_classical_pauli(GateTarget classical_control, GateTarget target);
    void undo_ZCX_single(GateTarget c, GateTarget t);
    void undo_ZCY_single(GateTarget c, GateTarget t);
    void undo_ZCZ_single(GateTarget c, GateTarget t);
    void undo_operation(const Operation &op);
    void undo_operation(const Operation &op, const Circuit &parent);
    void undo_circuit(const Circuit &circuit);
    void undo_loop(const Circuit &loop, uint64_t repetitions);
    void undo_loop_by_unrolling(const Circuit &loop, uint64_t repetitions);
    void clear_qubits(const OperationData &dat);
    void handle_x_gauges(const OperationData &dat);
    void handle_y_gauges(const OperationData &dat);
    void handle_z_gauges(const OperationData &dat);

    void undo_DETECTOR(const OperationData &dat);
    void undo_OBSERVABLE_INCLUDE(const OperationData &dat);
    void undo_RX(const OperationData &dat);
    void undo_RY(const OperationData &dat);
    void undo_RZ(const OperationData &dat);
    void undo_MX(const OperationData &dat);
    void undo_MY(const OperationData &dat);
    void undo_MZ(const OperationData &dat);
    void undo_MPP(const OperationData &dat);
    void undo_MRX(const OperationData &dat);
    void undo_MRY(const OperationData &dat);
    void undo_MRZ(const OperationData &dat);
    void undo_H_XZ(const OperationData &dat);
    void undo_H_XY(const OperationData &dat);
    void undo_H_YZ(const OperationData &dat);
    void undo_C_XYZ(const OperationData &dat);
    void undo_C_ZYX(const OperationData &dat);
    void undo_XCX(const OperationData &dat);
    void undo_XCY(const OperationData &dat);
    void undo_XCZ(const OperationData &dat);
    void undo_YCX(const OperationData &dat);
    void undo_YCY(const OperationData &dat);
    void undo_YCZ(const OperationData &dat);
    void undo_ZCX(const OperationData &dat);
    void undo_ZCY(const OperationData &dat);
    void undo_ZCZ(const OperationData &dat);
    void undo_I(const OperationData &dat);
    void undo_SQRT_XX(const OperationData &dat);
    void undo_SQRT_YY(const OperationData &dat);
    void undo_SQRT_ZZ(const OperationData &dat);
    void undo_SWAP(const OperationData &dat);
    void undo_ISWAP(const OperationData &dat);
    void undo_tableau(const Tableau &tableau, ConstPointerRange<uint32_t> targets);

    bool is_shifted_copy(const SparseUnsignedRevFrameTracker &other) const;
    void shift(int64_t measurement_offset, int64_t detector_offset);
    bool operator==(const SparseUnsignedRevFrameTracker &other) const;
    bool operator!=(const SparseUnsignedRevFrameTracker &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const SparseUnsignedRevFrameTracker &tracker);

}

#endif
