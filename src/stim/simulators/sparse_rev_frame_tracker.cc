// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/simulators/sparse_rev_frame_tracker.h"

using namespace stim;

SparseUnsignedRevFrameTracker::SparseUnsignedRevFrameTracker(uint64_t num_qubits, uint64_t num_measurements_in_past, uint64_t num_detectors_in_past)
    : xs(num_qubits),
      zs(num_qubits),
      rec_bits(),
      num_measurements_in_past(num_measurements_in_past),
      num_detectors_in_past(num_detectors_in_past) {
}

PauliString SparseUnsignedRevFrameTracker::current_error_sensitivity_for(DemTarget target) const {
    PauliString result(xs.size());
    for (size_t q = 0; q < xs.size(); q++) {
        result.xs[q] = std::find(xs[q].begin(), xs[q].end(), target) != xs[q].end();
        result.zs[q] = std::find(zs[q].begin(), zs[q].end(), target) != zs[q].end();
    }
    return result;
}

void SparseUnsignedRevFrameTracker::handle_xor_gauge(ConstPointerRange<DemTarget> sorted1, ConstPointerRange<DemTarget> sorted2) {
    if (sorted1 == sorted2) {
        return;
    }
    throw std::invalid_argument("A detector or observable anticommuted with a dissipative operation.");
}

void SparseUnsignedRevFrameTracker::handle_gauge(ConstPointerRange<DemTarget> sorted) {
    if (sorted.empty()) {
        return;
    }
    throw std::invalid_argument("A detector or observable anticommuted with a dissipative operation.");
}

void SparseUnsignedRevFrameTracker::undo_classical_pauli(GateTarget classical_control, GateTarget target) {
    if (classical_control.is_sweep_bit_target()) {
        // Sweep bits have no effect on error propagation.
        return;
    }
    assert(classical_control.is_measurement_record_target());

    uint64_t measurement_index = num_measurements_in_past + classical_control.value();
    SparseXorVec<DemTarget> &rec_dst = rec_bits[measurement_index];

    auto q = target.data & TARGET_VALUE_MASK;
    if (target.data & TARGET_PAULI_X_BIT) {
        rec_dst ^= zs[q];
    }
    if (target.data & TARGET_PAULI_Z_BIT) {
        rec_dst ^= xs[q];
    }
    if (rec_dst.empty()) {
        rec_bits.erase(measurement_index);
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCX_single(GateTarget c, GateTarget t) {
    if (!((c.data | t.data) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        // Pure quantum operation.
        zs[c.data] ^= zs[t.data];
        xs[t.data] ^= xs[c.data];
    } else if (!t.is_qubit_target()) {
        throw std::invalid_argument(
            "CX gate had '" + t.str() + "' as its target, but its target must be a qubit.");
    } else {
        undo_classical_pauli(c, GateTarget::x(t.data));
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCY_single(GateTarget c, GateTarget t) {
    if (!((c.data | t.data) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        // Pure quantum operation.
        zs[c.data] ^= zs[t.data];
        zs[c.data] ^= xs[t.data];
        xs[t.data] ^= xs[c.data];
        zs[t.data] ^= xs[c.data];
    } else if (!t.is_qubit_target()) {
        throw std::invalid_argument(
            "CY gate had '" + t.str() + "' as its target, but its target must be a qubit.");
    } else {
        undo_classical_pauli(c, GateTarget::y(t.data));
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCZ_single(GateTarget c, GateTarget t) {
    if (!((c.data | t.data) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        // Pure quantum operation.
        zs[c.data] ^= xs[t.data];
        zs[t.data] ^= xs[c.data];
    } else if (!(t.data & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        undo_classical_pauli(c, GateTarget::z(t.data));
    } else if (!(c.data & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        undo_classical_pauli(t, GateTarget::z(c.data));
    } else {
        // Both targets are classical. No effect.
    }
}

void SparseUnsignedRevFrameTracker::handle_x_gauges(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        handle_gauge(xs[q].range());
    }
}
void SparseUnsignedRevFrameTracker::handle_y_gauges(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        handle_xor_gauge(xs[q].range(), zs[q].range());
    }
}
void SparseUnsignedRevFrameTracker::handle_z_gauges(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        handle_gauge(zs[q].range());
    }
}
void SparseUnsignedRevFrameTracker::undo_MPP(const OperationData &target_data) {
    size_t n = target_data.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = target_data.targets[n - k - 1];
    }
    decompose_mpp_operation(
        OperationData{target_data.args, reversed_targets},
        xs.size(),
        [&](const OperationData &h_xz,
            const OperationData &h_yz,
            const OperationData &cnot,
            const OperationData &meas) {
            undo_H_XZ(h_xz);
            undo_H_YZ(h_yz);
            undo_ZCX(cnot);
            try {
                handle_x_gauges(meas);
            } catch (const std::invalid_argument &ex) {
                undo_ZCX(cnot);
                undo_H_YZ(h_yz);
                undo_H_XZ(h_xz);
                throw ex;
            }

            reversed_measure_targets.clear();
            for (size_t k = meas.targets.size(); k--;) {
                reversed_measure_targets.push_back(meas.targets[k]);
            }
            undo_MZ({meas.args, reversed_measure_targets});
            undo_ZCX(cnot);
            undo_H_YZ(h_yz);
            undo_H_XZ(h_xz);
        });
}

void SparseUnsignedRevFrameTracker::clear_qubits(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        xs[q].clear();
        zs[q].clear();
    }
}

void SparseUnsignedRevFrameTracker::undo_RX(const OperationData &dat) {
    handle_z_gauges(dat);
    clear_qubits(dat);
}
void SparseUnsignedRevFrameTracker::undo_RY(const OperationData &dat) {
    handle_y_gauges(dat);
    clear_qubits(dat);
}
void SparseUnsignedRevFrameTracker::undo_RZ(const OperationData &dat) {
    handle_x_gauges(dat);
    clear_qubits(dat);
}

void SparseUnsignedRevFrameTracker::undo_MX(const OperationData &dat) {
    handle_z_gauges(dat);
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        num_measurements_in_past--;
        auto f = rec_bits.find(num_measurements_in_past);
        if (f != rec_bits.end()) {
            xs[q].xor_sorted_items(f->second.range());
            rec_bits.erase(f);
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_MY(const OperationData &dat) {
    handle_y_gauges(dat);
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        num_measurements_in_past--;
        auto f = rec_bits.find(num_measurements_in_past);
        if (f != rec_bits.end()) {
            xs[q].xor_sorted_items(f->second.range());
            zs[q].xor_sorted_items(f->second.range());
            rec_bits.erase(f);
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_MZ(const OperationData &dat) {
    handle_x_gauges(dat);
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        num_measurements_in_past--;
        auto f = rec_bits.find(num_measurements_in_past);
        if (f != rec_bits.end()) {
            zs[q].xor_sorted_items(f->second.range());
            rec_bits.erase(f);
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_MRX(const OperationData &dat) {
    handle_z_gauges(dat);
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        num_measurements_in_past--;
        xs[q].clear();
        zs[q].clear();
        auto f = rec_bits.find(num_measurements_in_past);
        if (f != rec_bits.end()) {
            xs[q].xor_sorted_items(f->second.range());
            rec_bits.erase(f);
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_MRY(const OperationData &dat) {
    handle_y_gauges(dat);
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        num_measurements_in_past--;
        xs[q].clear();
        zs[q].clear();
        auto f = rec_bits.find(num_measurements_in_past);
        if (f != rec_bits.end()) {
            xs[q].xor_sorted_items(f->second.range());
            zs[q].xor_sorted_items(f->second.range());
            rec_bits.erase(f);
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_MRZ(const OperationData &dat) {
    handle_x_gauges(dat);
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        num_measurements_in_past--;
        xs[q].clear();
        zs[q].clear();
        auto f = rec_bits.find(num_measurements_in_past);
        if (f != rec_bits.end()) {
            zs[q].xor_sorted_items(f->second.range());
            rec_bits.erase(f);
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_H_XZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        std::swap(xs[q], zs[q]);
    }
}

void SparseUnsignedRevFrameTracker::undo_H_XY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        zs[q] ^= xs[q];
    }
}

void SparseUnsignedRevFrameTracker::undo_H_YZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        xs[q] ^= zs[q];
    }
}

void SparseUnsignedRevFrameTracker::undo_C_XYZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

void SparseUnsignedRevFrameTracker::undo_C_ZYX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

void SparseUnsignedRevFrameTracker::undo_XCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k].data;
        auto q2 = dat.targets[k + 1].data;
        xs[q1] ^= zs[q2];
        xs[q2] ^= zs[q1];
    }
}

void SparseUnsignedRevFrameTracker::undo_XCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k].data;
        auto ty = dat.targets[k + 1].data;
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void SparseUnsignedRevFrameTracker::undo_YCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k + 1].data;
        auto ty = dat.targets[k].data;
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        undo_ZCY_single(c, t);
    }
}

void SparseUnsignedRevFrameTracker::undo_YCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        undo_ZCY_single(c, t);
    }
}

void SparseUnsignedRevFrameTracker::undo_YCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[b];
        zs[a] ^= zs[b];
        xs[a] ^= xs[b];
        xs[a] ^= zs[b];

        zs[b] ^= xs[a];
        zs[b] ^= zs[a];
        xs[b] ^= xs[a];
        xs[b] ^= zs[a];
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        undo_ZCX_single(c, t);
    }
}

void SparseUnsignedRevFrameTracker::undo_XCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        undo_ZCX_single(c, t);
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k];
        auto q2 = dat.targets[k + 1];
        undo_ZCZ_single(q1, q2);
    }
}

void SparseUnsignedRevFrameTracker::undo_SQRT_XX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        xs[a] ^= zs[a];
        xs[a] ^= zs[b];
        xs[b] ^= zs[a];
        xs[b] ^= zs[b];
    }
}

void SparseUnsignedRevFrameTracker::undo_SQRT_YY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[a];
        zs[b] ^= xs[b];
        xs[a] ^= zs[a];
        xs[a] ^= zs[b];
        xs[b] ^= zs[a];
        xs[b] ^= zs[b];
        zs[a] ^= xs[a];
        zs[b] ^= xs[b];
    }
}

void SparseUnsignedRevFrameTracker::undo_SQRT_ZZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
    }
}

void SparseUnsignedRevFrameTracker::undo_I(const OperationData &dat) {
}

void SparseUnsignedRevFrameTracker::undo_SWAP(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void SparseUnsignedRevFrameTracker::undo_ISWAP(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void SparseUnsignedRevFrameTracker::undo_tableau(const Tableau &tableau, ConstPointerRange<uint32_t> targets) {
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

void SparseUnsignedRevFrameTracker::undo_DETECTOR(const OperationData &dat) {
    num_detectors_in_past--;
    auto det = DemTarget::relative_detector_id(num_detectors_in_past);
    for (auto t : dat.targets) {
        int64_t index = t.rec_offset() + (int64_t)num_measurements_in_past;
        if (index < 0) {
            throw std::invalid_argument("Referred to a measurement result before the beginning of time.");
        }
        rec_bits[(size_t)index].xor_item(det);
    }
}

void SparseUnsignedRevFrameTracker::undo_OBSERVABLE_INCLUDE(const OperationData &dat) {
    auto obs = DemTarget::observable_id((int32_t)dat.args[0]);
    for (auto t : dat.targets) {
        int64_t index = t.rec_offset() + (int64_t)num_measurements_in_past;
        if (index < 0) {
            throw std::invalid_argument("Referred to a measurement result before the beginning of time.");
        }
        rec_bits[index].xor_item(obs);
    }
}

void SparseUnsignedRevFrameTracker::undo_operation(const Operation &op, const Circuit &parent) {
    if (op.gate->id == gate_name_to_id("REPEAT")) {
        const auto &loop_body = op_data_block_body(parent, op.target_data);
        uint64_t repeats = op_data_rep_count(op.target_data);
        undo_loop(loop_body, repeats);
    } else {
        (this->*op.gate->sparse_unsigned_rev_frame_tracker_function)(op.target_data);
    }
}

void SparseUnsignedRevFrameTracker::undo_operation(const Operation &op) {
    assert(op.gate->id != gate_name_to_id("REPEAT"));
    (this->*op.gate->sparse_unsigned_rev_frame_tracker_function)(op.target_data);
}

void SparseUnsignedRevFrameTracker::undo_circuit(const Circuit &circuit) {
    for (size_t k = circuit.operations.size(); k--;) {
        undo_operation(circuit.operations[k], circuit);
    }
}

void SparseUnsignedRevFrameTracker::undo_loop_by_unrolling(const Circuit &loop, uint64_t iterations) {
    for (size_t rep = 0; rep < iterations; rep++) {
        undo_circuit(loop);
    }
}

bool _det_vec_is_equal_to_after_shift(ConstPointerRange<DemTarget> unshifted, ConstPointerRange<DemTarget> expected, int64_t detector_shift) {
    if (unshifted.size() != expected.size()) {
        return false;
    }
    for (size_t k = 0; k < unshifted.size(); k++) {
        DemTarget a = unshifted[k];
        DemTarget e = expected[k];
        a.shift_if_detector_id(detector_shift);
        if (a != e) {
            return false;
        }
    }
    return true;
}

bool _rec_to_det_is_equal_to_after_shift(
    const std::map<uint64_t, SparseXorVec<DemTarget>> &unshifted,
    const std::map<uint64_t, SparseXorVec<DemTarget>> &expected,
    int64_t measure_offset,
    int64_t detector_offset) {
    if (unshifted.size() != expected.size()) {
        return false;
    }
    for (const auto &unshifted_entry : unshifted) {
        const auto &shifted_entry = expected.find(unshifted_entry.first + measure_offset);
        if (shifted_entry == expected.end()) {
            return false;
        }
        if (!_det_vec_is_equal_to_after_shift(unshifted_entry.second.range(), shifted_entry->second.range(), detector_offset)) {
            return false;
        }
    }
    return true;
}

bool _vec_to_det_is_equal_to_after_shift(
    const std::vector<SparseXorVec<DemTarget>> &unshifted,
    const std::vector<SparseXorVec<DemTarget>> &expected,
    int64_t detector_offset) {
    if (unshifted.size() != expected.size()) {
        return false;
    }
    for (size_t k = 0; k < unshifted.size(); k++) {
        if (!_det_vec_is_equal_to_after_shift(unshifted[k].range(), expected[k].range(), detector_offset)) {
            return false;
        }
    }
    return true;

}

bool SparseUnsignedRevFrameTracker::is_shifted_copy(const SparseUnsignedRevFrameTracker &other) const {
    int64_t measurement_offset = (int64_t)other.num_measurements_in_past - (int64_t)num_measurements_in_past;
    int64_t detector_offset = (int64_t)other.num_detectors_in_past - (int64_t)num_detectors_in_past;
    return _rec_to_det_is_equal_to_after_shift(rec_bits, other.rec_bits, measurement_offset, detector_offset)
        && _vec_to_det_is_equal_to_after_shift(xs, other.xs, detector_offset)
        && _vec_to_det_is_equal_to_after_shift(zs, other.zs, detector_offset);
}

bool SparseUnsignedRevFrameTracker::operator==(const SparseUnsignedRevFrameTracker &other) const {
    return xs == other.xs
        && zs == other.zs
        && rec_bits == other.rec_bits
        && num_measurements_in_past == other.num_measurements_in_past
        && num_detectors_in_past == other.num_detectors_in_past;
}
bool SparseUnsignedRevFrameTracker::operator!=(const SparseUnsignedRevFrameTracker &other) const {
    return !(*this == other);
}

void SparseUnsignedRevFrameTracker::shift(int64_t measurement_offset, int64_t detector_offset) {
    num_measurements_in_past += measurement_offset;
    num_detectors_in_past += detector_offset;

    std::vector<std::pair<uint64_t, SparseXorVec<DemTarget>>> shifted;
    shifted.reserve(rec_bits.size());
    for (const auto &t : rec_bits) {
        shifted.push_back({t.first + measurement_offset, std::move(t.second)});
        for (auto &e : shifted.back().second) {
            e.shift_if_detector_id(detector_offset);
        }
    }
    rec_bits.clear();
    for (const auto &e : shifted) {
        rec_bits.insert(std::move(e));
    }

    for (auto &x : xs) {
        for (auto &e : x.sorted_items) {
            e.shift_if_detector_id(detector_offset);
        }
    }
    for (auto &z : zs) {
        for (auto &e : z.sorted_items) {
            e.shift_if_detector_id(detector_offset);
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_loop(const Circuit &loop, uint64_t iterations) {
    if (iterations < 5) {
        undo_loop_by_unrolling(loop, iterations);
        return;
    }

    SparseUnsignedRevFrameTracker tortoise(*this);
    uint64_t hare_steps = 0;
    uint64_t tortoise_steps = 0;

    while (true) {
        undo_circuit(loop);
        hare_steps++;
        if (is_shifted_copy(tortoise)) {
            break;
        }

        if (hare_steps > iterations - hare_steps) {
            undo_loop_by_unrolling(loop, iterations - hare_steps);
            return;
        }

        if ((hare_steps & 1) == 0) {
            tortoise.undo_circuit(loop);
            tortoise_steps++;
            if (is_shifted_copy(tortoise)) {
                break;
            }
        }
    }

    uint64_t period = hare_steps - tortoise_steps;
    assert(period > 0);
    uint64_t skipped_iterations = (iterations - hare_steps) / period;
    uint64_t detectors_per_period = tortoise.num_detectors_in_past - num_detectors_in_past;
    uint64_t measurements_per_period = tortoise.num_measurements_in_past - num_measurements_in_past;
    shift(
        -(int64_t)(measurements_per_period * skipped_iterations),
        -(int64_t)(detectors_per_period * skipped_iterations));
    hare_steps += skipped_iterations * period;

    undo_loop_by_unrolling(loop, iterations - hare_steps);
}

std::string SparseUnsignedRevFrameTracker::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream &stim::operator<<(std::ostream &out, const SparseUnsignedRevFrameTracker &tracker) {
    out << "SparseUnsignedRevFrameTracker {\n";
    out << "    num_measurements_in_past=" << tracker.num_measurements_in_past << "\n";
    out << "    num_detectors_in_past=" << tracker.num_detectors_in_past << "\n";
    for (size_t q = 0; q < tracker.xs.size(); q++) {
        out << "    xs[" << q << "]=" << tracker.xs[q] << "\n";
    }
    for (size_t q = 0; q < tracker.zs.size(); q++) {
        out << "    zs[" << q << "]=" << tracker.zs[q] << "\n";
    }
    for (const auto &t : tracker.rec_bits) {
        out << "    rec_bits[" << t.first << "]=" << t.second << "\n";
    }
    out << "}";
    return out;
}
