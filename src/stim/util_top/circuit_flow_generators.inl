#include "stim/util_top/circuit_inverse_qec.h"

namespace stim {

template <size_t W>
CircuitFlowGeneratorSolver<W>::CircuitFlowGeneratorSolver(CircuitStats stats)
    : table(),
      num_qubits(stats.num_qubits),
      num_measurements(stats.num_measurements),
      num_measurements_in_past(stats.num_measurements),
      measurements_only_table(),
      buf_for_rows_with(),
      buf_for_xor_merge() {
    if (num_measurements_in_past > INT32_MAX) {
        throw std::invalid_argument("Circuit is too large. Max flow measurement index is " + std::to_string(INT32_MAX));
    }
}

template <size_t W>
Flow<W> &CircuitFlowGeneratorSolver<W>::add_row() {
    table.push_back(
        Flow<W>{
            .input = PauliString<W>(num_qubits),
            .output = PauliString<W>(num_qubits),
            .measurements = {},
        });
    return table.back();
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::add_1q_measure_terms(CircuitInstruction inst, bool x, bool z) {
    for (size_t k = inst.targets.size(); k--;) {
        num_measurements_in_past--;

        auto t = inst.targets[k];
        if (!t.is_qubit_target()) {
            throw std::invalid_argument("Bad target in " + inst.str());
        }
        uint32_t q = t.qubit_value();
        auto &row = add_row();
        row.measurements.push_back(num_measurements_in_past);
        row.input.xs[q] = x;
        row.input.zs[q] = z;
        row.input.sign ^= t.is_inverted_result_target();
    }
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::add_2q_measure_terms(CircuitInstruction inst, bool x, bool z) {
    size_t k = inst.targets.size();
    while (k > 0) {
        k -= 2;
        num_measurements_in_past--;

        auto t1 = inst.targets[k];
        auto t2 = inst.targets[k + 1];
        if (!t1.is_qubit_target() || !t2.is_qubit_target()) {
            throw std::invalid_argument("Bad target in " + inst.str());
        }
        uint32_t q1 = t1.qubit_value();
        uint32_t q2 = t2.qubit_value();
        auto &row = add_row();
        row.measurements.push_back(num_measurements_in_past);
        row.input.xs[q1] = x;
        row.input.zs[q1] = z;
        row.input.xs[q2] = x;
        row.input.zs[q2] = z;
        row.input.sign ^= t1.is_inverted_result_target();
        row.input.sign ^= t2.is_inverted_result_target();
    }
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::remove_single_qubit_reset_terms(CircuitInstruction inst) {
    for (auto t : inst.targets) {
        if (!t.is_qubit_target()) {
            throw std::invalid_argument("Bad target in " + inst.str());
        }
        uint32_t q = t.qubit_value();
        for (auto &row : table) {
            row.input.xs[q] = 0;
            row.input.zs[q] = 0;
        }
    }
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::handle_anticommutations(std::span<const size_t> anticommutation_set) {
    if (anticommutation_set.empty()) {
        return;
    }

    // Sacrifice the first anticommutation to save the others.
    for (size_t k = 1; k < buf_for_rows_with.size(); k++) {
        mult_row_into(anticommutation_set[0], anticommutation_set[k]);
    }
    table.erase(table.begin() + anticommutation_set[0]);
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::check_for_2q_anticommutations(CircuitInstruction inst, bool x, bool z) {
    size_t k = inst.targets.size();
    while (k > 0) {
        k -= 2;

        auto t1 = inst.targets[k];
        auto t2 = inst.targets[k + 1];
        if (!t1.is_qubit_target() || !t2.is_qubit_target()) {
            throw std::invalid_argument("Bad target in " + inst.str());
        }
        uint32_t q1 = t1.qubit_value();
        uint32_t q2 = t2.qubit_value();

        auto anticommutations = rows_with([&](const Flow<W> &flow) {
            bool anticommutes = false;
            anticommutes ^= flow.input.xs[q1] & z;
            anticommutes ^= flow.input.zs[q1] & x;
            anticommutes ^= flow.input.xs[q2] & z;
            anticommutes ^= flow.input.zs[q2] & x;
            return anticommutes;
        });

        handle_anticommutations(anticommutations);
    }
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::check_for_1q_anticommutations(CircuitInstruction inst, bool x, bool z) {
    for (auto t : inst.targets) {
        if (!t.is_qubit_target()) {
            throw std::invalid_argument("Bad target in " + inst.str());
        }
        uint32_t q = t.qubit_value();

        handle_anticommutations(rows_anticommuting_with(q, x, z));
    }
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::mult_row_into(size_t src_row, size_t dst_row) {
    auto &src = table[src_row];
    auto &dst = table[dst_row];

    // Combine the pauli strings.
    uint8_t log_i = 0;
    log_i += dst.input.ref().inplace_right_mul_returning_log_i_scalar(src.input);
    log_i -= dst.output.ref().inplace_right_mul_returning_log_i_scalar(src.output);
    if (log_i & 1) {
        throw std::invalid_argument("Unexpected anticommutation while solving for flow generators.");
    }
    if (log_i & 2) {
        dst.input.sign ^= 1;
    }

    // Xor-merge-sort the measurement indices.
    buf_for_xor_merge.resize(std::max(buf_for_xor_merge.size(), dst.measurements.size() + src.measurements.size() + 1));
    const int32_t *end = xor_merge_sort(
        SpanRef<const int32_t>(dst.measurements), SpanRef<const int32_t>(src.measurements), buf_for_xor_merge.data());
    size_t n = end - buf_for_xor_merge.data();
    dst.measurements.resize(n);
    if (n > 0) {
        memcpy(dst.measurements.data(), buf_for_xor_merge.data(), n * sizeof(int32_t));
    }
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::undo_mrb(CircuitInstruction inst, bool x, bool z) {
    check_for_1q_anticommutations(inst, x, z);
    remove_single_qubit_reset_terms(inst);
    add_1q_measure_terms(inst, x, z);
}
template <size_t W>
void CircuitFlowGeneratorSolver<W>::undo_mb(CircuitInstruction inst, bool x, bool z) {
    check_for_1q_anticommutations(inst, x, z);
    add_1q_measure_terms(inst, x, z);
}
template <size_t W>
void CircuitFlowGeneratorSolver<W>::undo_rb(CircuitInstruction inst, bool x, bool z) {
    check_for_1q_anticommutations(inst, x, z);
    remove_single_qubit_reset_terms(inst);
}
template <size_t W>
void CircuitFlowGeneratorSolver<W>::undo_2q_m(CircuitInstruction inst, bool x, bool z) {
    check_for_2q_anticommutations(inst, x, z);
    add_2q_measure_terms(inst, x, z);
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::undo_feedback_capable_instruction(CircuitInstruction inst, bool x, bool z) {
    size_t k = inst.targets.size();
    while (k > 0) {
        k -= 2;

        auto t1 = inst.targets[k];
        auto t2 = inst.targets[k + 1];
        bool m1 = t1.is_measurement_record_target();
        bool m2 = t2.is_measurement_record_target();
        bool f1 = t1.is_qubit_target();
        bool f2 = t2.is_qubit_target();
        if ((m1 && f2) || (m2 && f1)) {
            uint32_t q = f1 ? t1.qubit_value() : t2.qubit_value();
            int32_t t = (m1 ? t1.value() : t2.value()) + num_measurements_in_past;
            if (t < 0) {
                throw std::invalid_argument("Referred to measurement before start of time in " + inst.str());
            }
            for (auto r : rows_anticommuting_with(q, x, z)) {
                xor_item_into_sorted_vec(t, table[r].measurements);
            }
        } else if (f1 && f2) {
            CircuitInstruction sub_inst = CircuitInstruction{inst.gate_type, {}, inst.targets.sub(k, k + 2), inst.tag};
            for (auto &row : table) {
                row.input.ref().undo_instruction(sub_inst);
            }
        }
    }
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::undo_instruction(CircuitInstruction inst) {
    if (table.size() > num_qubits * 3) {
        canonicalize_over_qubits();
    }

    switch (inst.gate_type) {
        case GateType::MRX:
        case GateType::MRY:
        case GateType::MR:
            undo_mrb(inst, inst.gate_type != GateType::MR, inst.gate_type != GateType::MRX);
            break;

        case GateType::MX:
        case GateType::MY:
        case GateType::M:
            undo_mb(inst, inst.gate_type != GateType::M, inst.gate_type != GateType::MX);
            break;

        case GateType::RX:
        case GateType::RY:
        case GateType::R:
            undo_rb(inst, inst.gate_type != GateType::R, inst.gate_type != GateType::RX);
            break;

        case GateType::MXX:
        case GateType::MYY:
        case GateType::MZZ:
            undo_2q_m(inst, inst.gate_type != GateType::MZZ, inst.gate_type != GateType::MXX);
            break;

        case GateType::DETECTOR:
        case GateType::OBSERVABLE_INCLUDE:
        case GateType::TICK:
        case GateType::QUBIT_COORDS:
        case GateType::SHIFT_COORDS:
        case GateType::DEPOLARIZE1:
        case GateType::DEPOLARIZE2:
        case GateType::X_ERROR:
        case GateType::Y_ERROR:
        case GateType::Z_ERROR:
        case GateType::PAULI_CHANNEL_1:
        case GateType::PAULI_CHANNEL_2:
        case GateType::E:
        case GateType::ELSE_CORRELATED_ERROR:
            // Ignored.
            break;

        case GateType::HERALDED_ERASE:
        case GateType::HERALDED_PAULI_CHANNEL_1:
            // Heralds.
            for (auto t : inst.targets) {
                num_measurements_in_past--;
                if (!t.is_qubit_target()) {
                    throw std::invalid_argument("Bad target in " + inst.str());
                }
                auto &row = add_row();
                row.measurements.push_back(num_measurements_in_past);
            }
            break;

        case GateType::MPAD:
            // Pads.
            for (auto t : inst.targets) {
                num_measurements_in_past--;
                if (!t.is_qubit_target()) {
                    throw std::invalid_argument("Bad target in " + inst.str());
                }
                auto &row = add_row();
                row.measurements.push_back(num_measurements_in_past);
                if (t.qubit_value()) {
                    row.output.sign = 1;
                }
            }
            break;

        case GateType::CX:
        case GateType::XCZ:
            undo_feedback_capable_instruction(inst, true, false);
            break;

        case GateType::YCZ:
        case GateType::CY:
            undo_feedback_capable_instruction(inst, true, true);
            break;

        case GateType::CZ:
            undo_feedback_capable_instruction(inst, false, true);
            break;

        case GateType::XCX:
        case GateType::XCY:
        case GateType::YCX:
        case GateType::YCY:
        case GateType::H:
        case GateType::H_XY:
        case GateType::H_YZ:
        case GateType::H_NXY:
        case GateType::H_NXZ:
        case GateType::H_NYZ:
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
            for (auto &row : table) {
                row.input.ref().undo_instruction(inst);
            }
            break;

        case GateType::MPP:
            buf_targets.clear();
            buf_targets.insert(buf_targets.end(), inst.targets.begin(), inst.targets.end());
            std::reverse(buf_targets.begin(), buf_targets.end());
            decompose_mpp_operation(
                CircuitInstruction{inst.gate_type, {}, buf_targets, inst.tag},
                num_qubits,
                [&](CircuitInstruction sub_inst) {
                    undo_instruction(sub_inst);
                });
            break;

        default:
            throw std::invalid_argument("Not handled by circuit flow generators method: " + inst.str());
    }
}

template <size_t W>
std::span<const size_t> CircuitFlowGeneratorSolver<W>::rows_anticommuting_with(uint32_t q, bool x, bool z) {
    return rows_with([&](const Flow<W> &flow) {
        bool anticommutes = false;
        anticommutes ^= flow.input.xs[q] & z;
        anticommutes ^= flow.input.zs[q] & x;
        return anticommutes;
    });
}

template <size_t W>
template <typename PREDICATE>
std::span<const size_t> CircuitFlowGeneratorSolver<W>::rows_with(PREDICATE predicate) {
    buf_for_rows_with.clear();
    for (size_t r = 0; r < table.size(); r++) {
        if (predicate(table[r])) {
            buf_for_rows_with.push_back(r);
        }
    }
    return buf_for_rows_with;
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::elimination_step(std::span<const size_t> elimination_set, size_t &num_eliminated) {
    size_t pivot = SIZE_MAX;
    for (auto p : elimination_set) {
        if (p >= num_eliminated) {
            pivot = p;
            break;
        }
    }
    if (pivot == SIZE_MAX) {
        return;
    }

    for (auto p : elimination_set) {
        if (p != pivot) {
            mult_row_into(pivot, p);
        }
    }
    std::swap(table[pivot], table[num_eliminated]);
    num_eliminated++;
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::canonicalize_over_qubits() {
    size_t num_eliminated = 0;
    for (size_t q = 0; q < num_qubits; q++) {
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return flow.input.xs[q];
            }),
            num_eliminated);
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return flow.input.zs[q];
            }),
            num_eliminated);
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return flow.output.xs[q];
            }),
            num_eliminated);
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return flow.output.zs[q];
            }),
            num_eliminated);
    }

    for (size_t r = 0; r < table.size(); r++) {
        if (table[r].input.ref().has_no_pauli_terms() && table[r].output.ref().has_no_pauli_terms()) {
            measurements_only_table.push_back(std::move(table[r]));
            std::swap(table[r], table.back());
            table.pop_back();
            r--;
        }
    }
}

template <size_t W>
void CircuitFlowGeneratorSolver<W>::final_canonicalize_into_table() {
    for (auto &row : measurements_only_table) {
        table.push_back(std::move(row));
    }

    size_t num_eliminated = 0;
    for (size_t q = 0; q < num_qubits; q++) {
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return flow.input.xs[q];
            }),
            num_eliminated);
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return flow.input.zs[q];
            }),
            num_eliminated);
    }
    for (size_t q = 0; q < num_qubits; q++) {
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return flow.output.xs[q];
            }),
            num_eliminated);
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return flow.output.zs[q];
            }),
            num_eliminated);
    }
    for (size_t m = 0; m < num_measurements; m++) {
        elimination_step(
            rows_with([&](const Flow<W> &flow) {
                return std::find(flow.measurements.begin(), flow.measurements.end(), (int32_t)m) !=
                       flow.measurements.end();
            }),
            num_eliminated);
    }
    for (auto &row : table) {
        row.output.sign ^= row.input.sign;
        row.input.sign = 0;
        if (row.input.ref().has_no_pauli_terms()) {
            row.input.xs.destructive_resize(0);
            row.input.zs.destructive_resize(0);
            row.input.num_qubits = 0;
        }
        if (row.output.ref().has_no_pauli_terms()) {
            row.output.xs.destructive_resize(0);
            row.output.zs.destructive_resize(0);
            row.output.num_qubits = 0;
        }
    }

    std::sort(table.begin(), table.end());
}

template <size_t W>
std::vector<Flow<W>> circuit_flow_generators(const Circuit &circuit) {
    CircuitFlowGeneratorSolver<W> solver(circuit.compute_stats());
    for (size_t q = 0; q < solver.num_qubits; q++) {
        auto &x = solver.add_row();
        x.output.xs[q] = 1;
        x.input.xs[q] = 1;
        auto &z = solver.add_row();
        z.output.zs[q] = 1;
        z.input.zs[q] = 1;
    }
    circuit.for_each_operation_reverse([&](CircuitInstruction inst) {
        solver.undo_instruction(inst);
    });
    solver.final_canonicalize_into_table();
    return solver.table;
}

}  // namespace stim
