#include "stim/util_top/circuit_inverse_qec.h"
#include "stim/stabilizers/flow.h"

namespace stim {

template <size_t W>
Circuit circuit_inverse_qec(const Circuit &circuit, std::span<const Flow<W>> flows) {
    CircuitStats stats = circuit.compute_stats();
    SparseUnsignedRevFrameTracker rev(stats.num_qubits, stats.num_measurements, stats.num_detectors, true);

    // Inject flow outputs as detectors-past-end-of-circuit.
//    for (size_t k = 0; k < flows.size(); k++) {
//        const auto &flow = flows[k];
//        flow.output.ref().for_each_active_pauli([&](size_t q) {
//            bool x = flow.output.xs[q];
//            bool z = flow.output.zs[q];
//            DemTarget t = DemTarget::relative_detector_id(stats.num_detectors + q);
//            if (x) {
//                rev.xs[k].xor_item(t);
//            }
//            if (z) {
//                rev.zs[k].xor_item(t);
//            }
//        });
//    }

    SparseXorVec<DemTarget> buf_x;
    SparseXorVec<DemTarget> buf_z;
    std::map<DemTarget, std::set<size_t>> d2ms;
    std::map<DemTarget, std::vector<double>> d2coords;
    size_t num_new_measurements = 0;
    Circuit result;

    std::vector<GateTarget> buf;
    simd_bits<64> qubit_workspace(stats.num_qubits);

    circuit.for_each_operation_reverse([&](const CircuitInstruction &inst) {
        Gate g = GATE_DATA[inst.gate_type];

        switch (inst.gate_type) {
            case GateType::DETECTOR: {
                rev.undo_gate(inst);
                auto &v = d2coords[DemTarget::relative_detector_id(rev.num_detectors_in_past)];
                v.insert(v.end(), inst.args.begin(), inst.args.end());
                break;
            }
            case GateType::OBSERVABLE_INCLUDE:
                rev.undo_gate(inst);
                break;
            case GateType::TICK:
            case GateType::I:
            case GateType::X:
            case GateType::Y:
            case GateType::Z:
            case GateType::C_XYZ:
            case GateType::C_ZYX:
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
            case GateType::XCZ:
            case GateType::YCX:
            case GateType::YCY:
            case GateType::YCZ:
            case GateType::CX:
            case GateType::CY:
            case GateType::CZ:
            case GateType::H:
            case GateType::H_XY:
            case GateType::H_YZ:
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
                rev.undo_gate(inst);
                result.safe_append_reversed_targets(
                    g.best_candidate_inverse_id, inst.targets, inst.args, g.flags & GATE_TARGETS_PAIRS);
                return;
            case GateType::MRX:
            case GateType::MRY:
            case GateType::MR:
            case GateType::RX:
            case GateType::RY:
            case GateType::R: {
                for_each_disjoint_target_segment_in_instruction_reversed(inst, qubit_workspace, [&](CircuitInstruction segment) {
                    // Each reset effect becomes a measurement effect in the inverted circuit. Index these measurements.
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
                    result.safe_append_reversed_targets(g.best_candidate_inverse_id, segment.targets, {}, false);

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
                        result.safe_append_reversed_targets(ejected_noise, segment.targets, segment.args, false);
                    }
                });
                break;
            }

            case GateType::MX:
            case GateType::MY:
            case GateType::M: {
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

                for (size_t k = inst.targets.size(); k-- > 0;) {
                    GateTarget t = inst.targets[k];
                    auto q = t.qubit_value();
                    if (rev.xs[q].empty() && rev.zs[q].empty() && rev.rec_bits.contains(rev.num_measurements_in_past - 1) && inst.args.empty()) {
                        // Noiseless measurements with past-dependence and no future-dependence become resets.
                        result.safe_append(reset, &t, inst.args);
                    } else {
                        // Measurements that aren't turned into resets need to be re-indexed.
                        auto f = rev.rec_bits.find(rev.num_measurements_in_past - k - 1);
                        if (f != rev.rec_bits.end()) {
                            for (auto &dem_target : f->second) {
                                d2ms[dem_target].insert(num_new_measurements);
                            }
                        }
                        num_new_measurements++;
                        result.safe_append(g.best_candidate_inverse_id, &t, inst.args);
                    }

                    rev.undo_gate(CircuitInstruction{g.id, {}, &t});
                }
                break;
            }
            case GateType::MPAD:
            case GateType::MPP:
            case GateType::MXX:
            case GateType::MYY:
            case GateType::MZZ: {
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
                result.safe_append_reversed_targets(g.best_candidate_inverse_id, inst.targets, inst.args, false);

                rev.undo_gate(inst);
                break;
            }

            case GateType::QUBIT_COORDS:
            case GateType::SHIFT_COORDS:
            case GateType::ELSE_CORRELATED_ERROR:
            default:
                throw std::invalid_argument("Don't know how to invert " + inst.str());
        }

        if (g.flags & (GATE_IS_RESET | GATE_PRODUCES_RESULTS)) {
            std::set<DemTarget> active_detectors;
            for (const auto &ds : rev.xs) {
                for (const auto &t : ds) {
                    active_detectors.insert(t);
                }
            }
            for (const auto &ds : rev.zs) {
                for (const auto &t : ds) {
                    active_detectors.insert(t);
                }
            }
            for (const auto &d : rev.rec_bits) {
                for (const auto &e : d.second) {
                    active_detectors.insert(e);
                }
            }

            for (auto d : d2ms) {
                SpanRef<const double> out_args{};
                GateType out_gate = GateType::DETECTOR;
                double id = 0;

                if (d.first.is_observable_id()) {
                    id = d.first.raw_id();
                    out_args = &id;
                } else if (active_detectors.contains(d.first)) {
                    continue;
                } else {
                    out_args = d2coords[d.first];
                }
                buf.clear();
                for (auto e : d.second) {
                    buf.push_back(GateTarget::rec((int32_t)e - (int32_t)num_new_measurements));
                }
                result.safe_append(out_gate, buf, out_args);
                d2coords.erase(d.first);
                d2ms.erase(d.first);
            }
        }
    });

    return result;
}

}  // namespace stim
