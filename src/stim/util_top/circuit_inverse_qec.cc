#include "stim/util_top/circuit_inverse_qec.h"

using namespace stim;
using namespace stim::internal;

CircuitFlowReverser::CircuitFlowReverser(CircuitStats stats, bool dont_turn_measurements_into_resets)
    : stats(stats),
      dont_turn_measurements_into_resets(dont_turn_measurements_into_resets),
      rev(stats.num_qubits, stats.num_measurements, stats.num_detectors, true),
      qubit_workspace(stats.num_qubits),
      num_new_measurements(0) {
}

void CircuitFlowReverser::recompute_active_terms() {
    active_terms.clear();
    for (const auto &ds : rev.xs) {
        for (const auto &t : ds) {
            active_terms.insert(t);
        }
    }
    for (const auto &ds : rev.zs) {
        for (const auto &t : ds) {
            active_terms.insert(t);
        }
    }
    for (const auto &d : rev.rec_bits) {
        for (const auto &e : d.second) {
            active_terms.insert(e);
        }
    }
}

void CircuitFlowReverser::do_rp_mrp_instruction(const CircuitInstruction &inst) {
    Gate g = GATE_DATA[inst.gate_type];
    for_each_disjoint_target_segment_in_instruction_reversed(inst, qubit_workspace, [&](CircuitInstruction segment) {
        // Each reset effect becomes a measurement effect in the inverted circuit. Index these
        // measurements.
        for (size_t k = inst.targets.size(); k-- > 0;) {
            auto q = inst.targets[k].qubit_value();
            for (auto d : rev.xs[q]) {
                d2ms[d].insert(num_new_measurements);
            }
            for (auto d : rev.zs[q]) {
                d2ms[d].insert(num_new_measurements);
            }
            num_new_measurements++;
        }

        // Undo the gate, ignoring measurement noise.
        rev.undo_gate(segment);
        inverted_circuit.safe_append_reversed_targets(
            CircuitInstruction(g.best_candidate_inverse_id, {}, segment.targets, inst.tag), false);

        // Measurement noise becomes noise-after-reset in the reversed circuit.
        if (!inst.args.empty()) {
            GateType ejected_noise;
            if (inst.gate_type == GateType::MRX) {
                ejected_noise = GateType::Z_ERROR;
            } else if (inst.gate_type == GateType::MRY) {
                ejected_noise = GateType::Z_ERROR;
            } else if (inst.gate_type == GateType::MR) {
                ejected_noise = GateType::X_ERROR;
            } else {
                throw std::invalid_argument("Don't know how to invert " + inst.str());
            }
            inverted_circuit.safe_append_reversed_targets(
                CircuitInstruction(ejected_noise, segment.args, segment.targets, inst.tag), false);
        }
    });
}

void CircuitFlowReverser::do_m2r_instruction(const CircuitInstruction &inst) {
    // Figure out the type of reset each measurement might be turned into.
    GateType reset;
    if (inst.gate_type == GateType::MX) {
        reset = GateType::RX;
    } else if (inst.gate_type == GateType::MY) {
        reset = GateType::RY;
    } else if (inst.gate_type == GateType::M) {
        reset = GateType::R;
    } else {
        throw std::invalid_argument("Don't know how to invert " + inst.str());
    }

    Gate g = GATE_DATA[inst.gate_type];
    for (size_t k = inst.targets.size(); k-- > 0;) {
        GateTarget t = inst.targets[k];
        auto q = t.qubit_value();
        if (!dont_turn_measurements_into_resets && rev.xs[q].empty() && rev.zs[q].empty() &&
            rev.rec_bits.contains(rev.num_measurements_in_past - 1) && inst.args.empty()) {
            // Noiseless measurements with past-dependence and no future-dependence become resets.
            inverted_circuit.safe_append(CircuitInstruction(reset, inst.args, &t, inst.tag));
        } else {
            // Measurements that aren't turned into resets need to be re-indexed.
            auto f = rev.rec_bits.find(rev.num_measurements_in_past - 1);
            if (f != rev.rec_bits.end()) {
                for (auto &dem_target : f->second) {
                    d2ms[dem_target].insert(num_new_measurements);
                }
            }
            num_new_measurements++;
            inverted_circuit.safe_append(CircuitInstruction(g.best_candidate_inverse_id, inst.args, &t, inst.tag));
        }

        rev.undo_gate(CircuitInstruction{g.id, {}, &t, inst.tag});
    }
}

void CircuitFlowReverser::do_measuring_instruction(const CircuitInstruction &inst) {
    Gate g = GATE_DATA[inst.gate_type];
    auto m = inst.count_measurement_results();

    // Re-index the measurements for the reversed detectors.
    for (size_t k = 0; k < m; k++) {
        auto f = rev.rec_bits.find(rev.num_measurements_in_past - k - 1);
        if (f != rev.rec_bits.end()) {
            for (auto &dem_target : f->second) {
                d2ms[dem_target].insert(num_new_measurements);
            }
        }
        num_new_measurements++;
    }
    inverted_circuit.safe_append_reversed_targets(
        CircuitInstruction(g.best_candidate_inverse_id, inst.args, inst.targets, inst.tag),
        g.flags & GATE_TARGETS_PAIRS);

    rev.undo_gate(inst);
}

void CircuitFlowReverser::do_feedback_capable_instruction(const CircuitInstruction &inst) {
    for (GateTarget t : inst.targets) {
        if (t.is_measurement_record_target()) {
            throw std::invalid_argument(
                "Time-reversing feedback isn't supported yet. Found feedback in: " + inst.str());
        }
    }
    do_simple_instruction(inst);
}

void CircuitFlowReverser::do_simple_instruction(const CircuitInstruction &inst) {
    Gate g = GATE_DATA[inst.gate_type];
    rev.undo_gate(inst);
    inverted_circuit.safe_append_reversed_targets(
        CircuitInstruction(g.best_candidate_inverse_id, inst.args, inst.targets, inst.tag),
        g.flags & GATE_TARGETS_PAIRS);
}

void CircuitFlowReverser::flush_detectors_and_observables() {
    recompute_active_terms();

    terms_to_erase.clear();
    for (auto d : d2ms) {
        SpanRef<const double> out_args{};
        GateType out_gate = GateType::DETECTOR;
        double id = 0;

        if (d.first.is_observable_id()) {
            if (d.first.raw_id() >= stats.num_observables) {
                continue;
            }
            id = d.first.raw_id();
            out_args = &id;
            out_gate = GateType::OBSERVABLE_INCLUDE;
        } else if (active_terms.contains(d.first)) {
            continue;
        } else {
            out_args = d2coords[d.first];
        }
        buf.clear();
        for (auto e : d.second) {
            buf.push_back(GateTarget::rec((int32_t)e - (int32_t)num_new_measurements));
        }
        inverted_circuit.safe_append(CircuitInstruction(out_gate, out_args, buf, d2tag[d.first]));
        terms_to_erase.push_back(d.first);
    }
    for (auto e : terms_to_erase) {
        d2coords.erase(e);
        d2ms.erase(e);
        d2tag.erase(e);
    }
}

void CircuitFlowReverser::do_instruction(const CircuitInstruction &inst) {
    switch (inst.gate_type) {
        case GateType::DETECTOR: {
            rev.undo_gate(inst);
            d2tag[DemTarget::relative_detector_id(rev.num_detectors_in_past)] = inst.tag;
            auto &v = d2coords[DemTarget::relative_detector_id(rev.num_detectors_in_past)];
            for (size_t k = 0; k < inst.args.size(); k++) {
                v.push_back(inst.args[k] + (k < coord_shifts.size() ? coord_shifts[k] : 0));
            }
            break;
        }
        case GateType::OBSERVABLE_INCLUDE:
            rev.undo_gate(inst);
            d2tag[DemTarget::observable_id((uint64_t)inst.args[0])] = inst.tag;
            break;
        case GateType::TICK:
        case GateType::I:
        case GateType::II:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
        case GateType::X:
        case GateType::Y:
        case GateType::Z:
        case GateType::C_XYZ:
        case GateType::C_NXYZ:
        case GateType::C_XNYZ:
        case GateType::C_XYNZ:
        case GateType::C_ZYX:
        case GateType::C_NZYX:
        case GateType::C_ZNYX:
        case GateType::C_ZYNX:
        case GateType::SQRT_X:
        case GateType::SQRT_X_DAG:
        case GateType::SQRT_Y:
        case GateType::SQRT_Y_DAG:
        case GateType::S:
        case GateType::S_DAG:
        case GateType::SQRT_XX:
        case GateType::SQRT_XX_DAG:
        case GateType::SQRT_YY:
        case GateType::SQRT_YY_DAG:
        case GateType::SQRT_ZZ:
        case GateType::SQRT_ZZ_DAG:
        case GateType::SPP:
        case GateType::SPP_DAG:
        case GateType::SWAP:
        case GateType::ISWAP:
        case GateType::CXSWAP:
        case GateType::SWAPCX:
        case GateType::CZSWAP:
        case GateType::ISWAP_DAG:
        case GateType::XCX:
        case GateType::XCY:
        case GateType::YCX:
        case GateType::YCY:
        case GateType::H:
        case GateType::H_XY:
        case GateType::H_YZ:
        case GateType::H_NXZ:
        case GateType::H_NXY:
        case GateType::H_NYZ:
        case GateType::DEPOLARIZE1:
        case GateType::DEPOLARIZE2:
        case GateType::X_ERROR:
        case GateType::Y_ERROR:
        case GateType::Z_ERROR:
        case GateType::PAULI_CHANNEL_1:
        case GateType::PAULI_CHANNEL_2:
        case GateType::E:
        case GateType::HERALDED_ERASE:
        case GateType::HERALDED_PAULI_CHANNEL_1:
            do_simple_instruction(inst);
            return;
        case GateType::XCZ:
        case GateType::YCZ:
        case GateType::CX:
        case GateType::CY:
        case GateType::CZ:
            do_feedback_capable_instruction(inst);
            break;
        case GateType::MRX:
        case GateType::MRY:
        case GateType::MR:
        case GateType::RX:
        case GateType::RY:
        case GateType::R:
            do_rp_mrp_instruction(inst);
            flush_detectors_and_observables();
            break;

        case GateType::MX:
        case GateType::MY:
        case GateType::M:
            do_m2r_instruction(inst);
            flush_detectors_and_observables();
            break;

        case GateType::MPAD:
        case GateType::MPP:
        case GateType::MXX:
        case GateType::MYY:
        case GateType::MZZ:
            do_measuring_instruction(inst);
            flush_detectors_and_observables();
            break;

        case GateType::QUBIT_COORDS:
            for (size_t k = 0; k < inst.args.size(); k++) {
                qubit_coords_circuit.arg_buf.append_tail(
                    inst.args[k] + (k < coord_shifts.size() ? coord_shifts[k] : 0));
            }
            qubit_coords_circuit.operations.push_back(
                CircuitInstruction{
                    inst.gate_type,
                    qubit_coords_circuit.arg_buf.commit_tail(),
                    qubit_coords_circuit.target_buf.take_copy(inst.targets),
                    inst.tag,
                });
            break;
        case GateType::SHIFT_COORDS:
            vec_pad_add_mul(coord_shifts, inst.args);
            break;
        case GateType::ELSE_CORRELATED_ERROR:
        default:
            throw std::invalid_argument("Don't know how to invert " + inst.str());
    }
}

Circuit &&CircuitFlowReverser::build_and_move_final_inverted_circuit() {
    if (qubit_coords_circuit.operations.empty()) {
        return std::move(inverted_circuit);
    }

    std::reverse(qubit_coords_circuit.operations.begin(), qubit_coords_circuit.operations.end());
    qubit_coords_circuit += inverted_circuit;
    return std::move(qubit_coords_circuit);
}
