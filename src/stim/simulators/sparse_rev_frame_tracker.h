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
    /// If false, anticommuting dets and obs are stored .
    /// If true, an exception is raised if anticommutation is detected.
    bool fail_on_anticommute;
    /// Where anticommuting dets and obs are stored.
    std::set<std::pair<DemTarget, GateTarget>> anticommutations;

    SparseUnsignedRevFrameTracker(
        uint64_t num_qubits,
        uint64_t num_measurements_in_past,
        uint64_t num_detectors_in_past,
        bool fail_on_anticommute = true);

    template <size_t W>
    PauliString<W> current_error_sensitivity_for(DemTarget target) const {
        PauliString<W> result(xs.size());
        for (size_t q = 0; q < xs.size(); q++) {
            result.xs[q] = std::find(xs[q].begin(), xs[q].end(), target) != xs[q].end();
            result.zs[q] = std::find(zs[q].begin(), zs[q].end(), target) != zs[q].end();
        }
        return result;
    }

    void undo_gate(const CircuitInstruction &inst);
    void undo_gate(const CircuitInstruction &op, const Circuit &parent);

    void handle_xor_gauge(
        SpanRef<const DemTarget> sorted1,
        SpanRef<const DemTarget> sorted2,
        const CircuitInstruction &inst,
        GateTarget location);
    void handle_gauge(SpanRef<const DemTarget> sorted, const CircuitInstruction &inst, GateTarget location);
    void fail_due_to_anticommutation(const CircuitInstruction &inst);
    void undo_classical_pauli(GateTarget classical_control, GateTarget target);
    void undo_ZCX_single(GateTarget c, GateTarget t);
    void undo_ZCY_single(GateTarget c, GateTarget t);
    void undo_ZCZ_single(GateTarget c, GateTarget t);
    void undo_circuit(const Circuit &circuit);
    void undo_loop(const Circuit &loop, uint64_t repetitions);
    void undo_loop_by_unrolling(const Circuit &loop, uint64_t repetitions);
    void clear_qubits(const CircuitInstruction &inst);
    void handle_x_gauges(const CircuitInstruction &inst);
    void handle_y_gauges(const CircuitInstruction &inst);
    void handle_z_gauges(const CircuitInstruction &inst);

    void undo_DETECTOR(const CircuitInstruction &inst);
    void undo_OBSERVABLE_INCLUDE(const CircuitInstruction &inst);
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
    void undo_SQRT_XX(const CircuitInstruction &inst);
    void undo_SQRT_YY(const CircuitInstruction &inst);
    void undo_SQRT_ZZ(const CircuitInstruction &inst);
    void undo_SWAP(const CircuitInstruction &inst);
    void undo_ISWAP(const CircuitInstruction &inst);
    void undo_CXSWAP(const CircuitInstruction &inst);
    void undo_CZSWAP(const CircuitInstruction &inst);
    void undo_SWAPCX(const CircuitInstruction &inst);

    template <size_t W>
    void undo_tableau(const Tableau<W> &tableau, SpanRef<const uint32_t> targets) {
        size_t n = tableau.num_qubits;
        if (n != targets.size()) {
            throw new std::invalid_argument("tableau.num_qubits != targets.size()");
        }
        std::set<uint32_t> target_set;
        for (size_t k = 0; k < n; k++) {
            if (!target_set.insert(targets[k]).second) {
                throw new std::invalid_argument("duplicate target");
            }
        }

        std::vector<SparseXorVec<DemTarget>> old_xs;
        std::vector<SparseXorVec<DemTarget>> old_zs;
        old_xs.reserve(n);
        old_zs.reserve(n);
        for (size_t k = 0; k < n; k++) {
            old_xs.push_back(std::move(xs[targets[k]]));
            old_zs.push_back(std::move(zs[targets[k]]));
            xs[targets[k]].clear();
            zs[targets[k]].clear();
        }

        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                uint64_t px = tableau.inverse_x_output_pauli_xyz(j, i);
                if (px == 1 || px == 2) {
                    xs[targets[i]] ^= old_xs[j];
                }
                if (px == 2 || px == 3) {
                    zs[targets[i]] ^= old_xs[j];
                }

                uint64_t pz = tableau.inverse_z_output_pauli_xyz(j, i);
                if (pz == 1 || pz == 2) {
                    xs[targets[i]] ^= old_zs[j];
                }
                if (pz == 2 || pz == 3) {
                    zs[targets[i]] ^= old_zs[j];
                }
            }
        }
    }

    bool is_shifted_copy(const SparseUnsignedRevFrameTracker &other) const;
    void shift(int64_t measurement_offset, int64_t detector_offset);
    bool operator==(const SparseUnsignedRevFrameTracker &other) const;
    bool operator!=(const SparseUnsignedRevFrameTracker &other) const;
    std::string str() const;

   private:
    void undo_MXX_disjoint_segment(const CircuitInstruction &inst);
    void undo_MYY_disjoint_segment(const CircuitInstruction &inst);
    void undo_MZZ_disjoint_segment(const CircuitInstruction &inst);
};
std::ostream &operator<<(std::ostream &out, const SparseUnsignedRevFrameTracker &tracker);

}  // namespace stim

#endif
