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
#include "stim/circuit/gate_data_table.h"
#include "stim/dem/detector_error_model.h"
#include "stim/mem/sparse_xor_vec.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau.h"

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
    // Function vtable for each gate's simulator function
    GateVTable<void (SparseUnsignedRevFrameTracker::*)(const CircuitInstruction&)> gate_vtable;

    SparseUnsignedRevFrameTracker(
        uint64_t num_qubits, uint64_t num_measurements_in_past, uint64_t num_detectors_in_past);

    PauliString current_error_sensitivity_for(DemTarget target) const;

    inline void undo_gate(const CircuitInstruction &data) {
        (this->*(gate_vtable.data[data.gate_type]))(data);
    }
    void undo_gate(const CircuitInstruction &op, const Circuit &parent);

    void handle_xor_gauge(SpanRef<const DemTarget> sorted1, SpanRef<const DemTarget> sorted2);
    void handle_gauge(SpanRef<const DemTarget> sorted);
    void undo_classical_pauli(GateTarget classical_control, GateTarget target);
    void undo_ZCX_single(GateTarget c, GateTarget t);
    void undo_ZCY_single(GateTarget c, GateTarget t);
    void undo_ZCZ_single(GateTarget c, GateTarget t);
    void undo_circuit(const Circuit &circuit);
    void undo_loop(const Circuit &loop, uint64_t repetitions);
    void undo_loop_by_unrolling(const Circuit &loop, uint64_t repetitions);
    void clear_qubits(const CircuitInstruction &dat);
    void handle_x_gauges(const CircuitInstruction &dat);
    void handle_y_gauges(const CircuitInstruction &dat);
    void handle_z_gauges(const CircuitInstruction &dat);

    void undo_DETECTOR(const CircuitInstruction &dat);
    void undo_OBSERVABLE_INCLUDE(const CircuitInstruction &dat);
    void undo_RX(const CircuitInstruction &dat);
    void undo_RY(const CircuitInstruction &dat);
    void undo_RZ(const CircuitInstruction &dat);
    void undo_MX(const CircuitInstruction &dat);
    void undo_MY(const CircuitInstruction &dat);
    void undo_MZ(const CircuitInstruction &dat);
    void undo_MPP(const CircuitInstruction &dat);
    void undo_MRX(const CircuitInstruction &dat);
    void undo_MRY(const CircuitInstruction &dat);
    void undo_MRZ(const CircuitInstruction &dat);
    void undo_H_XZ(const CircuitInstruction &dat);
    void undo_H_XY(const CircuitInstruction &dat);
    void undo_H_YZ(const CircuitInstruction &dat);
    void undo_C_XYZ(const CircuitInstruction &dat);
    void undo_C_ZYX(const CircuitInstruction &dat);
    void undo_XCX(const CircuitInstruction &dat);
    void undo_XCY(const CircuitInstruction &dat);
    void undo_XCZ(const CircuitInstruction &dat);
    void undo_YCX(const CircuitInstruction &dat);
    void undo_YCY(const CircuitInstruction &dat);
    void undo_YCZ(const CircuitInstruction &dat);
    void undo_ZCX(const CircuitInstruction &dat);
    void undo_ZCY(const CircuitInstruction &dat);
    void undo_ZCZ(const CircuitInstruction &dat);
    void undo_I(const CircuitInstruction &dat);
    void undo_SQRT_XX(const CircuitInstruction &dat);
    void undo_SQRT_YY(const CircuitInstruction &dat);
    void undo_SQRT_ZZ(const CircuitInstruction &dat);
    void undo_SWAP(const CircuitInstruction &dat);
    void undo_ISWAP(const CircuitInstruction &dat);
    void undo_CXSWAP(const CircuitInstruction &dat);
    void undo_SWAPCX(const CircuitInstruction &dat);
    void undo_tableau(const Tableau &tableau, SpanRef<const uint32_t> targets);

    bool is_shifted_copy(const SparseUnsignedRevFrameTracker &other) const;
    void shift(int64_t measurement_offset, int64_t detector_offset);
    bool operator==(const SparseUnsignedRevFrameTracker &other) const;
    bool operator!=(const SparseUnsignedRevFrameTracker &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const SparseUnsignedRevFrameTracker &tracker);

}  // namespace stim

#endif
