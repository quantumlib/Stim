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

#include "stim/circuit/gate_decomposition.h"

using namespace stim;

void SparseUnsignedRevFrameTracker::undo_gate(const CircuitInstruction &inst) {
    switch (inst.gate_type) {
        case GateType::DETECTOR:
            undo_DETECTOR(inst);
            break;
        case GateType::OBSERVABLE_INCLUDE:
            undo_OBSERVABLE_INCLUDE(inst);
            break;
        case GateType::MX:
            undo_MX(inst);
            break;
        case GateType::MY:
            undo_MY(inst);
            break;
        case GateType::M:
            undo_MZ(inst);
            break;
        case GateType::MRX:
            undo_MRX(inst);
            break;
        case GateType::MRY:
            undo_MRY(inst);
            break;
        case GateType::MR:
            undo_MRZ(inst);
            break;
        case GateType::RX:
            undo_RX(inst);
            break;
        case GateType::RY:
            undo_RY(inst);
            break;
        case GateType::R:
            undo_RZ(inst);
            break;
        case GateType::MPP:
            undo_MPP(inst);
            break;
        case GateType::SPP:
        case GateType::SPP_DAG:
            undo_SPP(inst);
            break;
        case GateType::XCX:
            undo_XCX(inst);
            break;
        case GateType::XCY:
            undo_XCY(inst);
            break;
        case GateType::XCZ:
            undo_XCZ(inst);
            break;
        case GateType::YCX:
            undo_YCX(inst);
            break;
        case GateType::YCY:
            undo_YCY(inst);
            break;
        case GateType::YCZ:
            undo_YCZ(inst);
            break;
        case GateType::CX:
            undo_ZCX(inst);
            break;
        case GateType::CY:
            undo_ZCY(inst);
            break;
        case GateType::CZ:
            undo_ZCZ(inst);
            break;
        case GateType::C_XYZ:
        case GateType::C_NXYZ:
        case GateType::C_XNYZ:
        case GateType::C_XYNZ:
            undo_C_XYZ(inst);
            break;
        case GateType::C_ZYX:
        case GateType::C_NZYX:
        case GateType::C_ZNYX:
        case GateType::C_ZYNX:
            undo_C_ZYX(inst);
            break;
        case GateType::SWAP:
            undo_SWAP(inst);
            break;
        case GateType::CXSWAP:
            undo_CXSWAP(inst);
            break;
        case GateType::CZSWAP:
            undo_CZSWAP(inst);
            break;
        case GateType::SWAPCX:
            undo_SWAPCX(inst);
            break;
        case GateType::MXX:
            undo_MXX(inst);
            break;
        case GateType::MYY:
            undo_MYY(inst);
            break;
        case GateType::MZZ:
            undo_MZZ(inst);
            break;

        case GateType::SQRT_XX:
        case GateType::SQRT_XX_DAG:
            undo_SQRT_XX(inst);
            break;

        case GateType::SQRT_YY:
        case GateType::SQRT_YY_DAG:
            undo_SQRT_YY(inst);
            break;

        case GateType::SQRT_ZZ:
        case GateType::SQRT_ZZ_DAG:
            undo_SQRT_ZZ(inst);
            break;

        case GateType::SQRT_X:
        case GateType::SQRT_X_DAG:
        case GateType::H_YZ:
        case GateType::H_NYZ:
            undo_H_YZ(inst);
            break;

        case GateType::SQRT_Y:
        case GateType::SQRT_Y_DAG:
        case GateType::H:
        case GateType::H_NXZ:
            undo_H_XZ(inst);
            break;

        case GateType::S:
        case GateType::S_DAG:
        case GateType::H_XY:
        case GateType::H_NXY:
            undo_H_XY(inst);
            break;

        case GateType::ISWAP:
        case GateType::ISWAP_DAG:
            undo_ISWAP(inst);
            break;

        case GateType::TICK:
        case GateType::QUBIT_COORDS:
        case GateType::SHIFT_COORDS:
        case GateType::REPEAT:
        case GateType::DEPOLARIZE1:
        case GateType::DEPOLARIZE2:
        case GateType::X_ERROR:
        case GateType::Y_ERROR:
        case GateType::Z_ERROR:
        case GateType::PAULI_CHANNEL_1:
        case GateType::PAULI_CHANNEL_2:
        case GateType::E:
        case GateType::ELSE_CORRELATED_ERROR:
        case GateType::X:
        case GateType::Y:
        case GateType::Z:
        case GateType::I:
        case GateType::II:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
            undo_I(inst);
            break;

        case GateType::MPAD:
        case GateType::HERALDED_ERASE:
        case GateType::HERALDED_PAULI_CHANNEL_1:
            undo_MPAD(inst);
            break;

        default:
            throw std::invalid_argument(
                "Not implemented by SparseUnsignedRevFrameTracker::undo_gate: " +
                std::string(GATE_DATA[inst.gate_type].name));
    }
}

SparseUnsignedRevFrameTracker::SparseUnsignedRevFrameTracker(
    uint64_t num_qubits, uint64_t num_measurements_in_past, uint64_t num_detectors_in_past, bool fail_on_anticommute)
    : xs(num_qubits),
      zs(num_qubits),
      rec_bits(),
      num_measurements_in_past(num_measurements_in_past),
      num_detectors_in_past(num_detectors_in_past),
      fail_on_anticommute(fail_on_anticommute),
      anticommutations() {
}

void SparseUnsignedRevFrameTracker::fail_due_to_anticommutation(const CircuitInstruction &inst) {
    std::stringstream ss;
    ss << "While running backwards through the circuit, during reverse-execution of the instruction\n";
    ss << "    " << inst << "\n";
    ss << "the following detecting region vs dissipation anticommutations occurred\n";
    for (auto &[d, g] : anticommutations) {
        ss << "    " << d << " vs " << g << "\n";
    }
    ss << "Therefore invalid detectors/observables are present in the circuit.\n";
    throw std::invalid_argument(ss.str());
}

void SparseUnsignedRevFrameTracker::handle_xor_gauge(
    SpanRef<const DemTarget> sorted1,
    SpanRef<const DemTarget> sorted2,
    const CircuitInstruction &inst,
    GateTarget location) {
    if (sorted1 == sorted2) {
        return;
    }
    SparseXorVec<DemTarget> dif;
    dif.xor_sorted_items(sorted1);
    dif.xor_sorted_items(sorted2);
    for (const auto &d : dif) {
        anticommutations.insert({d, location});
    }
    if (fail_on_anticommute) {
        fail_due_to_anticommutation(inst);
    }
}

void SparseUnsignedRevFrameTracker::handle_gauge(
    SpanRef<const DemTarget> sorted, const CircuitInstruction &inst, GateTarget location) {
    if (sorted.empty()) {
        return;
    }
    for (const auto &d : sorted) {
        anticommutations.insert({d, location});
    }
    if (fail_on_anticommute) {
        fail_due_to_anticommutation(inst);
    }
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
    auto cd = c.data;
    auto td = t.data;
    cd &= ~TARGET_INVERTED_BIT;
    td &= ~TARGET_INVERTED_BIT;
    if (!((cd | td) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        // Pure quantum operation.
        zs[cd] ^= zs[td];
        xs[td] ^= xs[cd];
    } else if (!t.is_qubit_target()) {
        throw std::invalid_argument("CX gate had '" + t.str() + "' as its target, but its target must be a qubit.");
    } else {
        undo_classical_pauli(c, GateTarget::x(td));
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCY_single(GateTarget c, GateTarget t) {
    auto cd = c.data;
    auto td = t.data;
    cd &= ~TARGET_INVERTED_BIT;
    td &= ~TARGET_INVERTED_BIT;
    if (!((cd | td) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        // Pure quantum operation.
        zs[cd] ^= zs[td];
        zs[cd] ^= xs[td];
        xs[td] ^= xs[cd];
        zs[td] ^= xs[cd];
    } else if (!t.is_qubit_target()) {
        throw std::invalid_argument("CY gate had '" + t.str() + "' as its target, but its target must be a qubit.");
    } else {
        undo_classical_pauli(c, GateTarget::y(td));
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCZ_single(GateTarget c, GateTarget t) {
    auto cd = c.data;
    auto td = t.data;
    cd &= ~TARGET_INVERTED_BIT;
    td &= ~TARGET_INVERTED_BIT;
    if (!((cd | td) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        // Pure quantum operation.
        zs[cd] ^= xs[td];
        zs[td] ^= xs[cd];
    } else if (!(td & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        undo_classical_pauli(c, GateTarget::z(td));
    } else if (!(cd & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        undo_classical_pauli(t, GateTarget::z(cd));
    } else {
        // Both targets are classical. No effect.
    }
}

void SparseUnsignedRevFrameTracker::handle_x_gauges(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        handle_gauge(xs[q].range(), dat, GateTarget::x(q));
    }
}
void SparseUnsignedRevFrameTracker::handle_y_gauges(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        handle_xor_gauge(xs[q].range(), zs[q].range(), dat, GateTarget::y(q));
    }
}
void SparseUnsignedRevFrameTracker::handle_z_gauges(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        handle_gauge(zs[q].range(), dat, GateTarget::z(q));
    }
}
void SparseUnsignedRevFrameTracker::undo_MPP(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }
    decompose_mpp_operation(
        CircuitInstruction{inst.gate_type, inst.args, reversed_targets, inst.tag},
        xs.size(),
        [&](const CircuitInstruction &inst) {
            if (inst.gate_type == GateType::M) {
                reversed_measure_targets.clear();
                for (size_t k = inst.targets.size(); k--;) {
                    reversed_measure_targets.push_back(inst.targets[k]);
                }
                undo_MZ({GateType::M, inst.args, reversed_measure_targets, inst.tag});
            } else {
                undo_gate(inst);
            }
        });
}

void SparseUnsignedRevFrameTracker::undo_SPP(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }
    decompose_spp_or_spp_dag_operation(
        CircuitInstruction{inst.gate_type, inst.args, reversed_targets, inst.tag},
        xs.size(),
        false,
        [&](const CircuitInstruction &inst) {
            undo_gate(inst);
        });
}

void SparseUnsignedRevFrameTracker::clear_qubits(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        xs[q].clear();
        zs[q].clear();
    }
}

void SparseUnsignedRevFrameTracker::undo_RX(const CircuitInstruction &dat) {
    handle_z_gauges(dat);
    clear_qubits(dat);
}
void SparseUnsignedRevFrameTracker::undo_RY(const CircuitInstruction &dat) {
    handle_y_gauges(dat);
    clear_qubits(dat);
}
void SparseUnsignedRevFrameTracker::undo_RZ(const CircuitInstruction &dat) {
    handle_x_gauges(dat);
    clear_qubits(dat);
}

void SparseUnsignedRevFrameTracker::undo_MPAD(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        num_measurements_in_past--;
        auto f = rec_bits.find(num_measurements_in_past);
        if (f != rec_bits.end()) {
            rec_bits.erase(f);
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_MX(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_MY(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_MZ(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_MXX_disjoint_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    undo_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets, ""});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        undo_MX(CircuitInstruction{GateType::MX, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, ""});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    undo_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets, ""});
}

void SparseUnsignedRevFrameTracker::undo_MYY_disjoint_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    undo_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets, ""});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        undo_MY(CircuitInstruction{GateType::MY, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, ""});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    undo_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets, ""});
}

void SparseUnsignedRevFrameTracker::undo_MZZ_disjoint_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    undo_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets, ""});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        undo_MZ(CircuitInstruction{GateType::M, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, ""});
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    undo_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets, ""});
}

void SparseUnsignedRevFrameTracker::undo_MXX(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }

    decompose_pair_instruction_into_disjoint_segments(
        CircuitInstruction{inst.gate_type, inst.args, reversed_targets, ""},
        xs.size(),
        [&](CircuitInstruction segment) {
            undo_MXX_disjoint_segment(segment);
        });
}

void SparseUnsignedRevFrameTracker::undo_MYY(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }

    decompose_pair_instruction_into_disjoint_segments(
        CircuitInstruction{inst.gate_type, inst.args, reversed_targets, ""},
        xs.size(),
        [&](CircuitInstruction segment) {
            undo_MYY_disjoint_segment(segment);
        });
}

void SparseUnsignedRevFrameTracker::undo_MZZ(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }

    decompose_pair_instruction_into_disjoint_segments(
        CircuitInstruction{inst.gate_type, inst.args, reversed_targets, ""},
        xs.size(),
        [&](CircuitInstruction segment) {
            undo_MZZ_disjoint_segment(segment);
        });
}

void SparseUnsignedRevFrameTracker::undo_MRX(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_MRY(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_MRZ(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_H_XZ(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        std::swap(xs[q], zs[q]);
    }
}

void SparseUnsignedRevFrameTracker::undo_H_XY(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        zs[q] ^= xs[q];
    }
}

void SparseUnsignedRevFrameTracker::undo_H_YZ(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        xs[q] ^= zs[q];
    }
}

void SparseUnsignedRevFrameTracker::undo_C_XYZ(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

void SparseUnsignedRevFrameTracker::undo_C_ZYX(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

void SparseUnsignedRevFrameTracker::undo_XCX(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k].data;
        auto q2 = dat.targets[k + 1].data;
        xs[q1] ^= zs[q2];
        xs[q2] ^= zs[q1];
    }
}

void SparseUnsignedRevFrameTracker::undo_XCY(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k].data;
        auto ty = dat.targets[k + 1].data;
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void SparseUnsignedRevFrameTracker::undo_YCX(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k + 1].data;
        auto ty = dat.targets[k].data;
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCY(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        undo_ZCY_single(c, t);
    }
}

void SparseUnsignedRevFrameTracker::undo_YCZ(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        undo_ZCY_single(c, t);
    }
}

void SparseUnsignedRevFrameTracker::undo_YCY(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_ZCX(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        undo_ZCX_single(c, t);
    }
}

void SparseUnsignedRevFrameTracker::undo_XCZ(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        undo_ZCX_single(c, t);
    }
}

void SparseUnsignedRevFrameTracker::undo_ZCZ(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k];
        auto q2 = dat.targets[k + 1];
        undo_ZCZ_single(q1, q2);
    }
}

void SparseUnsignedRevFrameTracker::undo_SQRT_XX(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        xs[a] ^= zs[a];
        xs[a] ^= zs[b];
        xs[b] ^= zs[a];
        xs[b] ^= zs[b];
    }
}

void SparseUnsignedRevFrameTracker::undo_SQRT_YY(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_SQRT_ZZ(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
    }
}

void SparseUnsignedRevFrameTracker::undo_I(const CircuitInstruction &dat) {
}

void SparseUnsignedRevFrameTracker::undo_SWAP(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void SparseUnsignedRevFrameTracker::undo_CXSWAP(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= zs[b];
        zs[b] ^= zs[a];
        xs[b] ^= xs[a];
        xs[a] ^= xs[b];
    }
}

void SparseUnsignedRevFrameTracker::undo_CZSWAP(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void SparseUnsignedRevFrameTracker::undo_SWAPCX(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[b] ^= zs[a];
        zs[a] ^= zs[b];
        xs[a] ^= xs[b];
        xs[b] ^= xs[a];
    }
}

void SparseUnsignedRevFrameTracker::undo_ISWAP(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_DETECTOR(const CircuitInstruction &dat) {
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

void SparseUnsignedRevFrameTracker::undo_OBSERVABLE_INCLUDE(const CircuitInstruction &dat) {
    auto obs = DemTarget::observable_id((int32_t)dat.args[0]);
    for (auto t : dat.targets) {
        if (t.is_measurement_record_target()) {
            int64_t index = t.rec_offset() + (int64_t)num_measurements_in_past;
            if (index < 0) {
                throw std::invalid_argument("Referred to a measurement result before the beginning of time.");
            }
            rec_bits[index].xor_item(obs);
        } else if (t.is_pauli_target()) {
            if (t.data & TARGET_PAULI_X_BIT) {
                xs[t.qubit_value()].xor_item(obs);
            }
            if (t.data & TARGET_PAULI_Z_BIT) {
                zs[t.qubit_value()].xor_item(obs);
            }
        } else {
            throw std::invalid_argument("Unexpected target for OBSERVABLE_INCLUDE: " + t.str());
        }
    }
}

void SparseUnsignedRevFrameTracker::undo_gate(const CircuitInstruction &op, const Circuit &parent) {
    if (op.gate_type == GateType::REPEAT) {
        const auto &loop_body = op.repeat_block_body(parent);
        uint64_t repeats = op.repeat_block_rep_count();
        undo_loop(loop_body, repeats);
    } else {
        undo_gate(op);
    }
}

void SparseUnsignedRevFrameTracker::undo_circuit(const Circuit &circuit) {
    for (size_t k = circuit.operations.size(); k--;) {
        undo_gate(circuit.operations[k], circuit);
    }
}

void SparseUnsignedRevFrameTracker::undo_loop_by_unrolling(const Circuit &loop, uint64_t iterations) {
    for (size_t rep = 0; rep < iterations; rep++) {
        undo_circuit(loop);
    }
}

bool _det_vec_is_equal_to_after_shift(
    SpanRef<const DemTarget> unshifted, SpanRef<const DemTarget> expected, int64_t detector_shift) {
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
        if (!_det_vec_is_equal_to_after_shift(
                unshifted_entry.second.range(), shifted_entry->second.range(), detector_offset)) {
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
    return _rec_to_det_is_equal_to_after_shift(rec_bits, other.rec_bits, measurement_offset, detector_offset) &&
           _vec_to_det_is_equal_to_after_shift(xs, other.xs, detector_offset) &&
           _vec_to_det_is_equal_to_after_shift(zs, other.zs, detector_offset);
}

bool SparseUnsignedRevFrameTracker::operator==(const SparseUnsignedRevFrameTracker &other) const {
    return xs == other.xs && zs == other.zs && rec_bits == other.rec_bits &&
           num_measurements_in_past == other.num_measurements_in_past &&
           num_detectors_in_past == other.num_detectors_in_past;
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
