#include "stim/util_top/export_quirk_url.h"

#include "stim/circuit/gate_decomposition.h"

using namespace stim;

static void for_each_target_group(
    CircuitInstruction instruction, const std::function<void(CircuitInstruction)> &callback) {
    Gate g = GATE_DATA[instruction.gate_type];
    if (g.flags & GATE_TARGETS_COMBINERS) {
        return for_each_combined_targets_group(instruction, callback);
    } else if (g.flags & GATE_TARGETS_PAIRS) {
        for (size_t k = 0; k < instruction.targets.size(); k += 2) {
            callback({instruction.gate_type, instruction.args, instruction.targets.sub(k, k + 2), instruction.tag});
        }
    } else if (g.flags & GATE_IS_SINGLE_QUBIT_GATE) {
        for (GateTarget t : instruction.targets) {
            callback({instruction.gate_type, instruction.args, &t, instruction.tag});
        }
    } else {
        callback(instruction);
    }
}

struct QuirkExporter {
    size_t num_qubits;
    size_t col_offset = 0;
    std::array<bool, NUM_DEFINED_GATES> used{};
    std::array<std::string_view, NUM_DEFINED_GATES> stim_name_to_quirk_name{};
    std::array<std::pair<std::string_view, std::string_view>, NUM_DEFINED_GATES> control_target_type{};
    std::array<std::string_view, NUM_DEFINED_GATES> phase_type{};
    std::array<std::string_view, NUM_DEFINED_GATES> custom_gate_definition{};
    std::map<size_t, std::map<size_t, std::string>> cols;

    QuirkExporter(size_t num_qubits) : num_qubits(num_qubits) {
        custom_gate_definition[(int)GateType::H_XY] =
            R"URL({"id":"~Hxy","name":"Hxy","matrix":"{{0,-√½-√½i},{√½-√½i,0}}"})URL";
        custom_gate_definition[(int)GateType::H_YZ] =
            R"URL({"id":"~Hyz","name":"Hyz","matrix":"{{-√½i,-√½},{√½,√½i}}"})URL";
        custom_gate_definition[(int)GateType::H_NXY] =
            R"URL({"id":"~Hnxy","name":"Hnxy","matrix":"{{0,√½+√½i},{√½-√½i,0}}"})URL";
        custom_gate_definition[(int)GateType::H_NXZ] =
            R"URL({"id":"~Hnxz","name":"Hnxz","matrix":"{{-√½,√½},{√½,√½}}"})URL";
        custom_gate_definition[(int)GateType::H_NYZ] =
            R"URL({"id":"~Hnyz","name":"Hnyz","matrix":"{{-√½,-√½i},{√½i,√½}}"})URL";

        custom_gate_definition[(int)GateType::C_XYZ] =
            R"URL({"id":"~Cxyz","name":"Cxyz","matrix":"{{½-½i,-½-½i},{½-½i,½+½i}}"})URL";
        custom_gate_definition[(int)GateType::C_NXYZ] =
            R"URL({"id":"~Cnxyz","name":"Cnxyz","matrix":"{{½+½i,½-½i},{-½-½i,½-½i}}"})URL";
        custom_gate_definition[(int)GateType::C_XNYZ] =
            R"URL({"id":"~Cxnyz","name":"Cxnyz","matrix":"{{½+½i,-½+½i},{½+½i,½-½i}}"})URL";
        custom_gate_definition[(int)GateType::C_XYNZ] =
            R"URL({"id":"~Cxynz","name":"Cxynz","matrix":"{{½-½i,½+½i},{-½+½i,½+½i}}"})URL";

        custom_gate_definition[(int)GateType::C_ZYX] =
            R"URL({"id":"~Czyx","name":"Czyx","matrix":"{{½+½i,½+½i},{-½+½i,½-½i}}"})URL";
        custom_gate_definition[(int)GateType::C_ZYNX] =
            R"URL({"id":"~Czynx","name":"Czynx","matrix":"{{½-½i,-½+½i},{½+½i,½+½i}}"})URL";
        custom_gate_definition[(int)GateType::C_ZNYX] =
            R"URL({"id":"~Cznyx","name":"Cznyx","matrix":"{{½-½i,½-½i},{-½-½i,½+½i}}"})URL";
        custom_gate_definition[(int)GateType::C_NZYX] =
            R"URL({"id":"~Cnzyx","name":"Cnzyx","matrix":"{{½+½i,-½-½i},{½-½i,½-½i}}"})URL";

        stim_name_to_quirk_name[(int)GateType::H] = "H";
        stim_name_to_quirk_name[(int)GateType::H_XY] = "~Hxy";
        stim_name_to_quirk_name[(int)GateType::H_YZ] = "~Hyz";
        stim_name_to_quirk_name[(int)GateType::H_NXY] = "~Hnxy";
        stim_name_to_quirk_name[(int)GateType::H_NXZ] = "~Hnxz";
        stim_name_to_quirk_name[(int)GateType::H_NYZ] = "~Hnyz";
        stim_name_to_quirk_name[(int)GateType::I] = "…";
        stim_name_to_quirk_name[(int)GateType::X] = "X";
        stim_name_to_quirk_name[(int)GateType::Y] = "Y";
        stim_name_to_quirk_name[(int)GateType::Z] = "Z";
        stim_name_to_quirk_name[(int)GateType::C_XYZ] = "~Cxyz";
        stim_name_to_quirk_name[(int)GateType::C_NXYZ] = "~Cnxyz";
        stim_name_to_quirk_name[(int)GateType::C_XNYZ] = "~Cxnyz";
        stim_name_to_quirk_name[(int)GateType::C_XYNZ] = "~Cxynz";
        stim_name_to_quirk_name[(int)GateType::C_ZYX] = "~Czyx";
        stim_name_to_quirk_name[(int)GateType::C_NZYX] = "~Cnzyx";
        stim_name_to_quirk_name[(int)GateType::C_ZNYX] = "~Cznyx";
        stim_name_to_quirk_name[(int)GateType::C_ZYNX] = "~Czynx";
        stim_name_to_quirk_name[(int)GateType::SQRT_X] = "X^½";
        stim_name_to_quirk_name[(int)GateType::SQRT_X_DAG] = "X^-½";
        stim_name_to_quirk_name[(int)GateType::SQRT_Y] = "Y^½";
        stim_name_to_quirk_name[(int)GateType::SQRT_Y_DAG] = "Y^-½";
        stim_name_to_quirk_name[(int)GateType::S] = "Z^½";
        stim_name_to_quirk_name[(int)GateType::S_DAG] = "Z^-½";
        stim_name_to_quirk_name[(int)GateType::MX] = "XDetector";
        stim_name_to_quirk_name[(int)GateType::MY] = "YDetector";
        stim_name_to_quirk_name[(int)GateType::M] = "ZDetector";
        stim_name_to_quirk_name[(int)GateType::MRX] = "XDetectControlReset";
        stim_name_to_quirk_name[(int)GateType::MRY] = "YDetectControlReset";
        stim_name_to_quirk_name[(int)GateType::MR] = "ZDetectControlReset";
        stim_name_to_quirk_name[(int)GateType::RX] = "XDetectControlReset";
        stim_name_to_quirk_name[(int)GateType::RY] = "YDetectControlReset";
        stim_name_to_quirk_name[(int)GateType::R] = "ZDetectControlReset";

        std::string_view x_control = "⊖";
        std::string_view y_control = "(/)";
        std::string_view z_control = "•";
        control_target_type[(int)GateType::XCX] = {x_control, "X"};
        control_target_type[(int)GateType::XCY] = {x_control, "Y"};
        control_target_type[(int)GateType::XCZ] = {x_control, "Z"};
        control_target_type[(int)GateType::YCX] = {y_control, "X"};
        control_target_type[(int)GateType::YCY] = {y_control, "Y"};
        control_target_type[(int)GateType::YCZ] = {y_control, "Z"};
        control_target_type[(int)GateType::CX] = {z_control, "X"};
        control_target_type[(int)GateType::CY] = {z_control, "Y"};
        control_target_type[(int)GateType::CZ] = {z_control, "Z"};
        control_target_type[(int)GateType::SWAPCX] = {z_control, "X"};
        control_target_type[(int)GateType::CXSWAP] = {x_control, "Z"};
        control_target_type[(int)GateType::CZSWAP] = {z_control, "Z"};
        control_target_type[(int)GateType::ISWAP] = {"zpar", "zpar"};
        control_target_type[(int)GateType::ISWAP_DAG] = {"zpar", "zpar"};
        control_target_type[(int)GateType::SQRT_XX] = {"xpar", "xpar"};
        control_target_type[(int)GateType::SQRT_YY] = {"ypar", "ypar"};
        control_target_type[(int)GateType::SQRT_ZZ] = {"zpar", "zpar"};
        control_target_type[(int)GateType::SQRT_XX_DAG] = {"xpar", "xpar"};
        control_target_type[(int)GateType::SQRT_YY_DAG] = {"ypar", "ypar"};
        control_target_type[(int)GateType::SQRT_ZZ_DAG] = {"zpar", "zpar"};
        control_target_type[(int)GateType::MXX] = {"xpar", "xpar"};
        control_target_type[(int)GateType::MYY] = {"xpar", "xpar"};
        control_target_type[(int)GateType::MZZ] = {"xpar", "xpar"};

        phase_type[(int)GateType::SQRT_XX] = "i";
        phase_type[(int)GateType::SQRT_YY] = "i";
        phase_type[(int)GateType::SQRT_ZZ] = "i";
        phase_type[(int)GateType::SQRT_XX_DAG] = "-i";
        phase_type[(int)GateType::SQRT_YY_DAG] = "-i";
        phase_type[(int)GateType::SQRT_ZZ_DAG] = "-i";
        phase_type[(int)GateType::SPP] = "i";
        phase_type[(int)GateType::SPP_DAG] = "-i";
        phase_type[(int)GateType::ISWAP] = "i";
        phase_type[(int)GateType::ISWAP_DAG] = "-i";
    }

    void write_pauli_par_controls(GateType g, size_t col, std::span<const GateTarget> targets) {
        for (auto t : targets) {
            if (t.has_qubit_value()) {
                bool x = t.data & TARGET_PAULI_X_BIT;
                bool z = t.data & TARGET_PAULI_Z_BIT;
                uint8_t p = x + z * 2;
                if (!p) {
                    cols[col][t.value()] = control_target_type[(int)g].first;
                } else {
                    cols[col][t.value()] = std::array<std::string_view, 4>{"", "xpar", "zpar", "ypar"}[p];
                }
            }
        }
    }

    size_t pick_free_qubit(std::span<const GateTarget> targets) {
        if (num_qubits <= 16) {
            return num_qubits;
        }
        std::set<size_t> qs;
        for (auto t : targets) {
            if (t.has_qubit_value()) {
                qs.insert(t.value());
            }
        }
        size_t q = 0;
        while (qs.contains(q)) {
            q += 1;
        }
        return q;
    }

    size_t pick_merge_qubit(std::span<const GateTarget> targets) {
        if (num_qubits <= 16) {
            return num_qubits;
        }
        for (auto t : targets) {
            uint8_t p = t.pauli_type();
            if (p && t.has_qubit_value() && t.qubit_value() <= 16) {
                return t.qubit_value();
            }
        }

        return num_qubits;
    }

    void do_single_qubit_gate(GateType g, GateTarget t) {
        if (t.has_qubit_value()) {
            if (cols[col_offset].contains(t.value()) || cols[col_offset + 1].contains(t.value()) ||
                cols[col_offset + 2].contains(t.value())) {
                col_offset += 3;
            }
            auto n = stim_name_to_quirk_name[(int)g];
            if (n == "XDetectControlReset") {
                cols[col_offset][t.value()] = n;
                cols[col_offset + 1][t.value()] = "H";
            } else if (n == "YDetectControlReset") {
                cols[col_offset][t.value()] = n;
                cols[col_offset + 1][t.value()] = "~Hyz";
                used[(int)GateType::H_YZ] = true;
            } else if (n == "ZDetectControlReset") {
                cols[col_offset][t.value()] = n;
            } else {
                cols[col_offset + 1][t.value()] = n;
            }
        }
    }

    void do_multi_phase_gate(GateType g, std::span<const GateTarget> group) {
        col_offset += 3;
        size_t q_free = pick_free_qubit(group);
        write_pauli_par_controls(g, col_offset, group);
        cols[col_offset][q_free] = phase_type[(int)g];
        col_offset += 3;
    }

    void do_multi_measure_gate(GateType g, std::span<const GateTarget> group) {
        col_offset += 3;
        size_t q_free = pick_merge_qubit(group);
        write_pauli_par_controls(g, col_offset, group);
        if (q_free == num_qubits) {
            cols[col_offset][q_free] = "X";
            cols[col_offset + 1][q_free] = "ZDetectControlReset";
        } else {
            write_pauli_par_controls(g, col_offset + 2, group);

            auto r = cols[col_offset][q_free];
            if (r == "xpar") {
                cols[col_offset][q_free] = "Z";
                cols[col_offset + 1][q_free] = "XDetector";
                cols[col_offset + 2][q_free] = "Z";
            } else if (r == "ypar") {
                cols[col_offset][q_free] = "X";
                cols[col_offset + 1][q_free] = "YDetector";
                cols[col_offset + 2][q_free] = "X";
            } else {
                cols[col_offset][q_free] = "X";
                cols[col_offset + 1][q_free] = "ZDetector";
                cols[col_offset + 2][q_free] = "X";
            }
        }
        col_offset += 3;
    }

    void do_controlled_gate(GateType g, GateTarget t1, GateTarget t2) {
        if (t1.has_qubit_value() && t2.has_qubit_value()) {
            col_offset += 3;
            auto [c, t] = control_target_type[(int)g];
            cols[col_offset][t1.qubit_value()] = c;
            cols[col_offset][t2.qubit_value()] = t;
            col_offset += 3;
        }
    }

    void do_swap_plus_gate(GateType g, GateTarget t1, GateTarget t2) {
        if (t1.has_qubit_value() && t2.has_qubit_value()) {
            col_offset += 3;
            cols[col_offset][t1.qubit_value()] = "Swap";
            cols[col_offset][t2.qubit_value()] = "Swap";
            if (g == GateType::ISWAP || g == GateType::ISWAP_DAG) {
                std::array<GateTarget, 2> group{t1, t2};
                do_multi_phase_gate(g, group);
            } else {
                do_controlled_gate(g, t1, t2);
            }
            col_offset += 3;
        }
    }

    void do_circuit(const Circuit &circuit) {
        circuit.for_each_operation([&](CircuitInstruction full_instruction) {
            used[(int)full_instruction.gate_type] = true;
            for_each_target_group(full_instruction, [&](CircuitInstruction inst) {
                switch (inst.gate_type) {
                    case GateType::DETECTOR:
                    case GateType::OBSERVABLE_INCLUDE:
                    case GateType::QUBIT_COORDS:
                    case GateType::SHIFT_COORDS:
                    case GateType::MPAD:
                    case GateType::DEPOLARIZE1:
                    case GateType::DEPOLARIZE2:
                    case GateType::X_ERROR:
                    case GateType::Y_ERROR:
                    case GateType::Z_ERROR:
                    case GateType::PAULI_CHANNEL_1:
                    case GateType::PAULI_CHANNEL_2:
                    case GateType::E:
                    case GateType::ELSE_CORRELATED_ERROR:
                    case GateType::HERALDED_ERASE:
                    case GateType::HERALDED_PAULI_CHANNEL_1:
                    case GateType::II:
                    case GateType::I_ERROR:
                    case GateType::II_ERROR:
                        // Ignored.
                        break;

                    case GateType::TICK:
                        col_offset += 3;
                        break;

                    case GateType::MX:
                    case GateType::MY:
                    case GateType::M:
                    case GateType::MRX:
                    case GateType::MRY:
                    case GateType::MR:
                    case GateType::RX:
                    case GateType::RY:
                    case GateType::R:
                    case GateType::H:
                    case GateType::H_XY:
                    case GateType::H_YZ:
                    case GateType::H_NXY:
                    case GateType::H_NXZ:
                    case GateType::H_NYZ:
                    case GateType::I:
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
                        do_single_qubit_gate(inst.gate_type, inst.targets[0]);
                        break;

                    case GateType::SQRT_XX:
                    case GateType::SQRT_YY:
                    case GateType::SQRT_ZZ:
                    case GateType::SQRT_XX_DAG:
                    case GateType::SQRT_YY_DAG:
                    case GateType::SQRT_ZZ_DAG:
                    case GateType::SPP:
                    case GateType::SPP_DAG:
                        do_multi_phase_gate(inst.gate_type, inst.targets);
                        break;

                    case GateType::XCX:
                    case GateType::XCY:
                    case GateType::XCZ:
                    case GateType::YCX:
                    case GateType::YCY:
                    case GateType::YCZ:
                    case GateType::CX:
                    case GateType::CY:
                    case GateType::CZ:
                        do_controlled_gate(inst.gate_type, inst.targets[0], inst.targets[1]);
                        break;

                    case GateType::SWAP:
                    case GateType::ISWAP:
                    case GateType::CXSWAP:
                    case GateType::SWAPCX:
                    case GateType::CZSWAP:
                    case GateType::ISWAP_DAG:
                        do_swap_plus_gate(inst.gate_type, inst.targets[0], inst.targets[1]);
                        break;

                    case GateType::MXX:
                    case GateType::MYY:
                    case GateType::MZZ:
                    case GateType::MPP:
                        do_multi_measure_gate(inst.gate_type, inst.targets);
                        break;

                    default:
                        throw std::invalid_argument("Not supported in export_quirk_url: " + full_instruction.str());
                }
            });
        });
    }
};

std::string stim::export_quirk_url(const Circuit &circuit) {
    QuirkExporter exporter(circuit.count_qubits());
    exporter.do_circuit(circuit);

    std::stringstream out;
    exporter.col_offset += 3;

    out << R"URL(https://algassert.com/quirk#circuit={"cols":[)URL";
    bool has_col = false;
    for (size_t k = 0; k < exporter.col_offset; k++) {
        if (!exporter.cols.contains(k)) {
            continue;
        }
        const auto &col = exporter.cols.at(k);
        if (col.empty()) {
            continue;
        }
        std::vector<std::string> entries;
        for (auto kv : col) {
            while (entries.size() <= kv.first) {
                entries.push_back("");
            }
            entries[kv.first] = kv.second;
        }
        if (has_col) {
            out << ",";
        }
        has_col = true;
        out << "[";
        for (size_t q = 0; q < entries.size(); q++) {
            if (q) {
                out << ",";
            }
            if (entries[q].empty()) {
                out << "1";
            } else {
                out << '"';
                out << entries[q];
                out << '"';
            }
        }
        out << "]";
    }
    out << R"URL(])URL";
    bool has_custom_gates = false;
    for (size_t k = 0; k < NUM_DEFINED_GATES; k++) {
        if (!exporter.custom_gate_definition[k].empty() && exporter.used[k]) {
            if (!has_custom_gates) {
                out << R"URL(,"gates":[)URL";
                has_custom_gates = true;
            } else {
                out << ',';
            }
            out << exporter.custom_gate_definition[k];
        }
    }
    if (has_custom_gates) {
        out << "]";
    }
    out << "}";

    return out.str();
}
