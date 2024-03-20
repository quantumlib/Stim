#include "stim/stabilizers/flow.h"
#include "stim/util_top/circuit_inverse_qec.h"

namespace stim {

template <size_t W>
std::pair<Circuit, std::vector<Flow<W>>> circuit_inverse_qec(
    const Circuit &circuit, std::span<const Flow<W>> flows, bool dont_turn_measurements_into_resets) {
    CircuitStats stats = circuit.compute_stats();
    SparseUnsignedRevFrameTracker rev(stats.num_qubits, stats.num_measurements, stats.num_detectors, true);

    // Inject flow outputs as observables-past-end-of-circuit.
    for (size_t k = 0; k < flows.size(); k++) {
        const auto &flow = flows[k];
        DemTarget flow_target = DemTarget::observable_id(stats.num_observables + k);
        flow.output.ref().for_each_active_pauli([&](size_t q) {
            bool x = flow.output.xs[q];
            bool z = flow.output.zs[q];
            if (x) {
                rev.xs[q].xor_item(flow_target);
            }
            if (z) {
                rev.zs[q].xor_item(flow_target);
            }
        });
        for (int32_t m : flow.measurements) {
            if (m < 0) {
                m += stats.num_measurements;
            }
            if (m < 0 || (uint64_t)m >= stats.num_measurements) {
                std::stringstream ss;
                ss << "Out of range measurement in one of the flows: " << flow;
                throw std::invalid_argument(ss.str());
            }
            rev.rec_bits[m].sorted_items.push_back(flow_target);
        }
    }

    std::map<DemTarget, std::set<size_t>> d2ms;
    std::map<DemTarget, std::vector<double>> d2coords;
    size_t num_new_measurements = 0;
    Circuit inverted_circuit;

    std::vector<GateTarget> buf;
    simd_bits<64> qubit_workspace(stats.num_qubits);
    std::set<DemTarget> active_terms;
    std::vector<DemTarget> terms_to_erase;

    std::vector<double> coord_buf;
    std::vector<double> coord_shifts;
    Circuit qubit_coords_circuit;

    auto recompute_active_terms = [&]() {
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
    };

    circuit.for_each_operation_reverse([&](const CircuitInstruction &inst) {
        Gate g = GATE_DATA[inst.gate_type];

        switch (inst.gate_type) {
            case GateType::DETECTOR: {
                rev.undo_gate(inst);
                auto &v = d2coords[DemTarget::relative_detector_id(rev.num_detectors_in_past)];
                for (size_t k = 0; k < inst.args.size(); k++) {
                    v.push_back(inst.args[k] + (k < coord_shifts.size() ? coord_shifts[k] : 0));
                }
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
                inverted_circuit.safe_append_reversed_targets(
                    g.best_candidate_inverse_id, inst.targets, inst.args, g.flags & GATE_TARGETS_PAIRS);
                return;
            case GateType::MRX:
            case GateType::MRY:
            case GateType::MR:
            case GateType::RX:
            case GateType::RY:
            case GateType::R: {
                for_each_disjoint_target_segment_in_instruction_reversed(
                    inst, qubit_workspace, [&](CircuitInstruction segment) {
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
                            g.best_candidate_inverse_id, segment.targets, {}, false);

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
                                ejected_noise, segment.targets, segment.args, false);
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
                    if (!dont_turn_measurements_into_resets && rev.xs[q].empty() && rev.zs[q].empty() &&
                        rev.rec_bits.contains(rev.num_measurements_in_past - 1) && inst.args.empty()) {
                        // Noiseless measurements with past-dependence and no future-dependence become resets.
                        inverted_circuit.safe_append(reset, &t, inst.args);
                    } else {
                        // Measurements that aren't turned into resets need to be re-indexed.
                        auto f = rev.rec_bits.find(rev.num_measurements_in_past - k - 1);
                        if (f != rev.rec_bits.end()) {
                            for (auto &dem_target : f->second) {
                                d2ms[dem_target].insert(num_new_measurements);
                            }
                        }
                        num_new_measurements++;
                        inverted_circuit.safe_append(g.best_candidate_inverse_id, &t, inst.args);
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
                inverted_circuit.safe_append_reversed_targets(
                    g.best_candidate_inverse_id, inst.targets, inst.args, g.flags & GATE_TARGETS_PAIRS);

                rev.undo_gate(inst);
                break;
            }

            case GateType::QUBIT_COORDS:
                for (size_t k = 0; k < inst.args.size(); k++) {
                    qubit_coords_circuit.arg_buf.append_tail(
                        inst.args[k] + (k < coord_shifts.size() ? coord_shifts[k] : 0));
                }
                qubit_coords_circuit.operations.push_back({
                    inst.gate_type,
                    qubit_coords_circuit.arg_buf.commit_tail(),
                    qubit_coords_circuit.target_buf.take_copy(inst.targets),
                });
                break;
            case GateType::SHIFT_COORDS:
                vec_pad_add_mul(coord_shifts, inst.args);
                break;
            case GateType::ELSE_CORRELATED_ERROR:
            default:
                throw std::invalid_argument("Don't know how to invert " + inst.str());
        }

        if (g.flags & (GATE_IS_RESET | GATE_PRODUCES_RESULTS)) {
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
                inverted_circuit.safe_append(out_gate, buf, out_args);
                terms_to_erase.push_back(d.first);
            }
            for (auto e : terms_to_erase) {
                d2coords.erase(e);
                d2ms.erase(e);
            }
        }
    });

    // Cancel expected flow inputs against actual flow inputs.
    for (size_t k = 0; k < flows.size(); k++) {
        const auto &flow = flows[k];
        flow.input.ref().for_each_active_pauli([&](size_t q) {
            bool x = flow.input.xs[q];
            bool z = flow.input.zs[q];
            DemTarget t = DemTarget::observable_id(stats.num_observables + k);
            if (x) {
                rev.xs[q].xor_item(t);
            }
            if (z) {
                rev.zs[q].xor_item(t);
            }
        });
    }

    // Verify everything went as expected.
    bool failed = false;
    DemTarget example{};
    for (size_t q = 0; q < stats.num_qubits; q++) {
        for (auto &e : rev.xs[q]) {
            failed = true;
            example = e;
        }
        for (auto &e : rev.zs[q]) {
            failed = true;
            example = e;
        }
    }
    if (failed) {
        if (example.is_relative_detector_id() ||
            (example.is_observable_id() && example.raw_id() < stats.num_observables)) {
            std::stringstream ss;
            ss << "The detecting region of " << example << " reached the start of the circuit.\n";
            ss << "Only flows given as arguments are permitted to touch the start or end of the circuit.\n";
            ss << "There are four potential ways to fix this issue, depending on what's wrong:\n";
            ss << "- If " + example.str() +
                      " was relying on implicit initialization into |0> at the start of the circuit, add explicit "
                      "resets to the circuit.\n";
            ss << "- If " + example.str() + " shouldn't be reaching the start of the circuit, fix its declaration.\n";
            ss << "- If " + example.str() + " isn't needed, delete it from the circuit.\n";
            ss << "- If the given circuit is a partial circuit, and " << example
               << " is reaching outside of it, refactor " << example << "into a flow argument.";
            throw std::invalid_argument(ss.str());
        } else {
            std::stringstream ss;
            const auto &flow = flows[example.raw_id() - stats.num_observables];
            ss << "The circuit didn't satisfy one of the given flows (ignoring sign): ";
            ss << flow;
            auto v = rev.current_error_sensitivity_for<W>(example);
            v.xs ^= flow.input.xs;
            v.zs ^= flow.input.zs;
            ss << "\nChanging the flow to '"
               << Flow<W>{.input = v, .output = flow.output, .measurements = flow.measurements}
               << "' would make it a valid flow.";
            throw std::invalid_argument(ss.str());
        }
    }

    // Build output flows.
    std::vector<Flow<W>> inverted_flows;
    for (size_t k = 0; k < flows.size(); k++) {
        const auto &f = flows[k];
        inverted_flows.push_back(Flow<W>{
            .input = f.output,
            .output = f.input,
            .measurements = {},
        });
        auto &f2 = inverted_flows.back();
        f2.input.sign = false;
        f2.output.sign = false;
        for (auto &m : d2ms[DemTarget::observable_id(k + stats.num_observables)]) {
            f2.measurements.push_back((int32_t)m - (int32_t)num_new_measurements);
        }
    }

    if (!qubit_coords_circuit.operations.empty()) {
        std::reverse(qubit_coords_circuit.operations.begin(), qubit_coords_circuit.operations.end());
        inverted_circuit = qubit_coords_circuit + inverted_circuit;
    }

    return {inverted_circuit, inverted_flows};
}

}  // namespace stim
