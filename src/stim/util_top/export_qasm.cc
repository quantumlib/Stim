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

#include "stim/util_top/export_qasm.h"

#include <bitset>

#include "stim/simulators/tableau_simulator.h"

using namespace stim;

struct QasmExporter {
    std::ostream &out;
    CircuitStats stats;
    int open_qasm_version;
    bool skip_dets_and_obs;
    simd_bits<64> reference_sample;
    uint64_t measurement_offset;
    uint64_t detector_offset;
    std::array<const char *, NUM_DEFINED_GATES> qasm_names{};
    std::bitset<NUM_DEFINED_GATES> used_gates{};
    std::stringstream buf_q1;
    std::stringstream buf_q2;
    std::stringstream buf_m;

    QasmExporter() = delete;
    QasmExporter(const QasmExporter &) = delete;
    QasmExporter(QasmExporter &&) = delete;
    QasmExporter(std::ostream &out, const Circuit &circuit, int open_qasm_version, bool skip_dets_and_obs)
        : out(out),
          stats(circuit.compute_stats()),
          open_qasm_version(open_qasm_version),
          skip_dets_and_obs(skip_dets_and_obs),
          reference_sample(stats.num_measurements),
          measurement_offset(0),
          detector_offset(0) {
        // Init used_gates.
        collect_used_gates(circuit);

        // Init reference_sample.
        if (stats.num_detectors > 0 || stats.num_observables > 0) {
            reference_sample = TableauSimulator<64>::reference_sample_circuit(circuit);
        }
    }

    void output_measurement(bool invert_measurement_result, const char *q_name, const char *m_name) {
        if (invert_measurement_result) {
            if (open_qasm_version == 3) {
                out << "measure " << q_name << " -> " << m_name << ";";
                out << m_name << " = " << m_name << " ^ 1;";
            } else {
                out << "x " << q_name << ";";
                out << "measure " << q_name << " -> " << m_name << ";";
                out << "x " << q_name << ";";
            }
        } else {
            out << "measure " << q_name << " -> " << m_name << ";";
        }
    }

    void output_decomposed_operation(
        bool invert_measurement_result, GateType g, const char *q0_name, const char *q1_name, const char *m_name) {
        auto q2n = [&](GateTarget t) {
            return t.qubit_value() == 0 ? q0_name : q1_name;
        };
        bool first = true;
        for (const auto &inst : Circuit(GATE_DATA[g].h_s_cx_m_r_decomposition).operations) {
            switch (inst.gate_type) {
                case GateType::S:
                    for (const auto &t : inst.targets) {
                        if (!first) {
                            out << " ";
                        }
                        first = false;
                        out << "s " << q2n(t) << ";";
                    }
                    break;
                case GateType::H:
                    for (const auto &t : inst.targets) {
                        if (!first) {
                            out << " ";
                        }
                        first = false;
                        out << "h " << q2n(t) << ";";
                    }
                    break;
                case GateType::R:
                    for (const auto &t : inst.targets) {
                        if (!first) {
                            out << " ";
                        }
                        first = false;
                        out << "reset " << q2n(t) << ";";
                    }
                    break;
                case GateType::CX:
                    for (size_t k = 0; k < inst.targets.size(); k += 2) {
                        if (!first) {
                            out << " ";
                        }
                        first = false;
                        auto t1 = inst.targets[k];
                        auto t2 = inst.targets[k + 1];
                        out << "cx " << q2n(t1) << ", " << q2n(t2) << ";";
                    }
                    break;
                case GateType::M:
                    for (const auto &t : inst.targets) {
                        if (!first) {
                            out << " ";
                        }
                        first = false;
                        output_measurement(invert_measurement_result, q2n(t), m_name);
                    }
                    break;
                default:
                    throw std::invalid_argument("Unhandled: " + inst.str());
            }
        }
    }

    void output_decomposed_mpp_operation(const CircuitInstruction &inst) {
        out << "// --- begin decomposed " << inst << "\n";
        decompose_mpp_operation(inst, stats.num_qubits, [&](const CircuitInstruction &inst) {
            output_instruction(inst);
        });
        out << "// --- end decomposed MPP\n";
    }

    void output_decomposed_spp_or_spp_dag_operation(const CircuitInstruction &inst) {
        out << "// --- begin decomposed " << inst << "\n";
        decompose_spp_or_spp_dag_operation(inst, stats.num_qubits, false, [&](const CircuitInstruction &inst) {
            output_instruction(inst);
        });
        out << "// --- end decomposed SPP\n";
    }

    void output_decomposable_instruction(const CircuitInstruction &instruction, bool decompose_inline) {
        auto f = GATE_DATA[instruction.gate_type].flags;
        auto step = (f & GATE_TARGETS_PAIRS) ? 2 : 1;
        for (size_t k = 0; k < instruction.targets.size(); k += step) {
            auto t0 = instruction.targets[k];
            auto t1 = instruction.targets[k + step - 1];
            bool invert_measurement_result = t0.is_inverted_result_target();
            if (step == 2) {
                invert_measurement_result ^= t1.is_inverted_result_target();
            }
            if (decompose_inline) {
                buf_q1.str("");
                buf_q2.str("");
                buf_q1 << "q[" << t0.qubit_value() << "]";
                buf_q2 << "q[" << t1.qubit_value() << "]";
                if (f & GATE_PRODUCES_RESULTS) {
                    buf_m.str("");
                    buf_m << "rec[" << measurement_offset << "]";
                    measurement_offset++;
                }
                output_decomposed_operation(
                    invert_measurement_result,
                    instruction.gate_type,
                    buf_q1.str().c_str(),
                    buf_q2.str().c_str(),
                    buf_m.str().c_str());
                out << " // decomposed " << GATE_DATA[instruction.gate_type].name << "\n";
            } else {
                if (f & GATE_PRODUCES_RESULTS) {
                    out << "rec[" << measurement_offset << "] = ";
                    measurement_offset++;
                }
                out << qasm_names[(int)instruction.gate_type] << "(";
                out << "q[" << t0.qubit_value() << "]";
                if (step == 2) {
                    out << ", q[" << t1.qubit_value() << "]";
                }
                out << ")";
                if ((f & GATE_PRODUCES_RESULTS) && invert_measurement_result) {
                    out << " ^ 1";
                }
                out << ";\n";
            }
        }
    }

    void output_two_qubit_unitary_instruction_with_possible_feedback(const CircuitInstruction &instruction) {
        for (size_t k = 0; k < instruction.targets.size(); k += 2) {
            auto t1 = instruction.targets[k];
            auto t2 = instruction.targets[k + 1];
            if (t1.is_qubit_target() && t2.is_qubit_target()) {
                out << qasm_names[(int)instruction.gate_type] << " q[" << t1.qubit_value() << "], q["
                    << t2.qubit_value() << "];\n";
            } else if (t1.is_qubit_target() || t2.is_qubit_target()) {
                GateTarget control;
                GateTarget target;
                char basis;
                switch (instruction.gate_type) {
                    case GateType::CX:
                        basis = 'X';
                        control = t1;
                        target = t2;
                        break;
                    case GateType::CY:
                        basis = 'Y';
                        control = t1;
                        target = t2;
                        break;
                    case GateType::CZ:
                        basis = 'Z';
                        control = t1;
                        target = t2;
                        if (control.is_qubit_target()) {
                            std::swap(control, target);
                        }
                        break;
                    case GateType::XCZ:
                        basis = 'X';
                        control = t2;
                        target = t1;
                        break;
                    case GateType::YCZ:
                        basis = 'Y';
                        control = t2;
                        target = t1;
                        break;
                    default:
                        throw std::invalid_argument(
                            "Not implemented in output_two_qubit_unitary_instruction_with_possible_feedback: " +
                            instruction.str());
                }

                out << "if (";
                if (control.is_measurement_record_target()) {
                    if (open_qasm_version == 2) {
                        throw std::invalid_argument(
                            "The circuit contains feedback, but OPENQASM 2 doesn't support feedback.\n"
                            "You can use `stim.Circuit.with_inlined_feedback` to drop feedback operations.\n"
                            "Alternatively, pass the argument `open_qasm_version=3`.");
                    }
                    out << "ms[" << (measurement_offset + control.rec_offset()) << "]";
                } else if (control.is_sweep_bit_target()) {
                    if (open_qasm_version == 2) {
                        throw std::invalid_argument(
                            "The circuit contains sweep operation, but OPENQASM 2 doesn't support feedback.\n"
                            "Remove these operations, or pass the argument `open_qasm_version=3`.");
                    }
                    out << "sweep[" << control.value() << "]";
                } else {
                    throw std::invalid_argument(
                        "Not implemented in output_two_qubit_unitary_instruction_with_possible_feedback: " +
                        instruction.str());
                }
                out << ") {\n";
                out << "    " << basis << " q[" << target.qubit_value() << "];\n";
                out << "}\n";
            }
        }
    }

    void define_custom_single_qubit_gate(GateType g, const char *name) {
        const auto &gate = GATE_DATA[g];
        qasm_names[(int)g] = name;
        if (!used_gates[(int)g]) {
            return;
        }

        out << "gate " << name << " q0 { U(";
        auto xyz = gate.to_euler_angles();
        std::array<const char *, 4> angles{"0", "pi/2", "pi", "-pi/2"};
        out << angles[(int)round(xyz[0] / 3.14159265359f) & 3];
        out << ", " << angles[(int)round(xyz[1] / 3.14159265359f) & 3];
        out << ", " << angles[(int)round(xyz[2] / 3.14159265359f) & 3];
        out << ") q0; }\n";
    }

    void define_custom_decomposed_gate(GateType g, const char *name) {
        const auto &gate = GATE_DATA[g];
        qasm_names[(int)g] = name;
        if (!used_gates[(int)g]) {
            return;
        }

        Circuit c(gate.h_s_cx_m_r_decomposition);
        bool is_unitary = true;
        for (const auto &inst : c.operations) {
            is_unitary &= (GATE_DATA[inst.gate_type].flags & GATE_IS_UNITARY) != 0;
        }
        auto num_measurements = c.count_measurements();
        if (is_unitary) {
            out << "gate " << name;
            out << " q0";
            if (gate.flags & GateFlags::GATE_TARGETS_PAIRS) {
                out << ", q1";
            }
            out << " { ";
        } else {
            if (open_qasm_version == 2) {
                // Have to decompose inline in the circuit.
                return;
            }
            out << "def " << name << "(qubit q0";
            if (gate.flags & GateFlags::GATE_TARGETS_PAIRS) {
                out << ", qubit q1";
            }
            out << ")";
            if (num_measurements > 1) {
                throw std::invalid_argument("Multiple measurement gates not supported.");
            } else if (num_measurements == 1) {
                out << " -> bit { bit b; ";
            } else {
                out << " { ";
            }
        }

        output_decomposed_operation(false, g, "q0", "q1", "b");
        if (num_measurements > 0) {
            out << " return b;";
        }
        out << " }\n";
    }

    void collect_used_gates(const Circuit &c) {
        for (const auto &inst : c.operations) {
            used_gates[(int)inst.gate_type] = true;
            if (inst.gate_type == GateType::REPEAT) {
                collect_used_gates(inst.repeat_block_body(c));
            }
        }
    }

    void output_header() {
        if (open_qasm_version == 2) {
            out << "OPENQASM 2.0;\n";
        } else {
            out << "OPENQASM 3.0;\n";
        }
    }

    void output_storage_declarations() {
        if (stats.num_qubits > 0) {
            out << "qreg q[" << stats.num_qubits << "];\n";
        }
        if (stats.num_measurements > 0) {
            out << "creg rec[" << stats.num_measurements << "];\n";
        }
        if (stats.num_detectors > 0 && !skip_dets_and_obs) {
            out << "creg dets[" << stats.num_detectors << "];\n";
        }
        if (stats.num_observables > 0 && !skip_dets_and_obs) {
            out << "creg obs[" << stats.num_observables << "];\n";
        }
        if (stats.num_sweep_bits > 0) {
            out << "creg sweep[" << stats.num_sweep_bits << "];\n";
        }
        out << "\n";
    }

    void define_all_gates_and_output_gate_declarations() {
        if (open_qasm_version == 2) {
            out << "include \"qelib1.inc\";\n";
        } else if (open_qasm_version == 3) {
            out << "include \"stdgates.inc\";\n";
        } else {
            throw std::invalid_argument("Unrecognized open_qasm_version.");
        }
        qasm_names[(int)GateType::I] = "id";
        qasm_names[(int)GateType::X] = "x";
        qasm_names[(int)GateType::Y] = "y";
        qasm_names[(int)GateType::Z] = "z";
        qasm_names[(int)GateType::SQRT_X] = "sx";
        qasm_names[(int)GateType::SQRT_X_DAG] = "sxdg";
        qasm_names[(int)GateType::S] = "s";
        qasm_names[(int)GateType::S_DAG] = "sdg";
        qasm_names[(int)GateType::CX] = "cx";
        qasm_names[(int)GateType::CY] = "cy";
        qasm_names[(int)GateType::CZ] = "cz";
        qasm_names[(int)GateType::SWAP] = "swap";
        qasm_names[(int)GateType::H] = "h";
        define_custom_single_qubit_gate(GateType::C_XYZ, "cxyz");
        define_custom_single_qubit_gate(GateType::C_ZYX, "czyx");
        define_custom_single_qubit_gate(GateType::C_NXYZ, "cnxyz");
        define_custom_single_qubit_gate(GateType::C_XNYZ, "cxnyz");
        define_custom_single_qubit_gate(GateType::C_XYNZ, "cxynz");
        define_custom_single_qubit_gate(GateType::C_NZYX, "cnzyx");
        define_custom_single_qubit_gate(GateType::C_ZNYX, "cznyx");
        define_custom_single_qubit_gate(GateType::C_ZYNX, "czynx");
        define_custom_single_qubit_gate(GateType::H_XY, "hxy");
        define_custom_single_qubit_gate(GateType::H_YZ, "hyz");
        define_custom_single_qubit_gate(GateType::H_NXY, "hnxy");
        define_custom_single_qubit_gate(GateType::H_NXZ, "hnxz");
        define_custom_single_qubit_gate(GateType::H_NYZ, "hnyz");
        define_custom_single_qubit_gate(GateType::SQRT_Y, "sy");
        define_custom_single_qubit_gate(GateType::SQRT_Y_DAG, "sydg");

        define_custom_decomposed_gate(GateType::CXSWAP, "cxswap");
        define_custom_decomposed_gate(GateType::CZSWAP, "czswap");
        define_custom_decomposed_gate(GateType::ISWAP, "iswap");
        define_custom_decomposed_gate(GateType::ISWAP_DAG, "iswapdg");
        define_custom_decomposed_gate(GateType::SQRT_XX, "sxx");
        define_custom_decomposed_gate(GateType::SQRT_XX_DAG, "sxxdg");
        define_custom_decomposed_gate(GateType::SQRT_YY, "syy");
        define_custom_decomposed_gate(GateType::SQRT_YY_DAG, "syydg");
        define_custom_decomposed_gate(GateType::SQRT_ZZ, "szz");
        define_custom_decomposed_gate(GateType::SQRT_ZZ_DAG, "szzdg");
        define_custom_decomposed_gate(GateType::SWAPCX, "swapcx");
        define_custom_decomposed_gate(GateType::XCX, "xcx");
        define_custom_decomposed_gate(GateType::XCY, "xcy");
        define_custom_decomposed_gate(GateType::XCZ, "xcz");
        define_custom_decomposed_gate(GateType::YCX, "ycx");
        define_custom_decomposed_gate(GateType::YCY, "ycy");
        define_custom_decomposed_gate(GateType::YCZ, "ycz");

        define_custom_decomposed_gate(GateType::MR, "mr");
        define_custom_decomposed_gate(GateType::MRX, "mrx");
        define_custom_decomposed_gate(GateType::MRY, "mry");
        define_custom_decomposed_gate(GateType::MX, "mx");
        define_custom_decomposed_gate(GateType::MXX, "mxx");
        define_custom_decomposed_gate(GateType::MY, "my");
        define_custom_decomposed_gate(GateType::MYY, "myy");
        define_custom_decomposed_gate(GateType::MZZ, "mzz");
        define_custom_decomposed_gate(GateType::RX, "rx");
        define_custom_decomposed_gate(GateType::RY, "ry");

        out << "\n";
    }

    void output_instruction(const CircuitInstruction &instruction) {
        GateFlags f = GATE_DATA[instruction.gate_type].flags;

        switch (instruction.gate_type) {
            case GateType::QUBIT_COORDS:
            case GateType::SHIFT_COORDS:
            case GateType::II:
            case GateType::I_ERROR:
            case GateType::II_ERROR:
                // Skipped.
                return;

            case GateType::MPAD:
                for (const auto &t : instruction.targets) {
                    if (open_qasm_version == 3) {
                        out << "rec[" << measurement_offset << "] = " << t.qubit_value() << ";\n";
                    } else {
                        if (t.qubit_value()) {
                            throw std::invalid_argument(
                                "The circuit contains a vacuous measurement with a non-zero result "
                                "(like MPAD 1 or MPP !X1*X1) but OPENQASM 2 doesn't support classical assignment.\n"
                                "Pass the argument `open_qasm_version=3` to fix this.");
                        }
                    }
                    measurement_offset++;
                }
                return;

            case GateType::TICK:
                out << "barrier q;\n\n";
                return;

            case GateType::M:
                for (const auto &t : instruction.targets) {
                    if (t.is_inverted_result_target()) {
                        if (open_qasm_version == 3) {
                            out << "measure q[" << t.qubit_value() << "] -> rec[" << measurement_offset << "];";
                            out << "rec[" << measurement_offset << "] = rec[" << measurement_offset << "] ^ 1;";
                        } else {
                            out << "x q[" << t.qubit_value() << "];";
                            out << "measure q[" << t.qubit_value() << "] -> rec[" << measurement_offset << "];";
                            out << "x q[" << t.qubit_value() << "];";
                        }
                    } else {
                        out << "measure q[" << t.qubit_value() << "] -> rec[" << measurement_offset << "];";
                    }
                    out << "\n";
                    measurement_offset++;
                }
                return;
            case GateType::R:
                for (const auto &t : instruction.targets) {
                    out << "reset q[" << t.qubit_value() << "];\n";
                }
                return;
            case GateType::DETECTOR:
            case GateType::OBSERVABLE_INCLUDE: {
                if (skip_dets_and_obs) {
                    return;
                }
                if (open_qasm_version == 2) {
                    throw std::invalid_argument(
                        "The circuit contains detectors or observables, but OPENQASM 2 doesn't support the operations "
                        "needed for accumulating detector and observable values.\n"
                        "To simply ignore detectors and observables, pass the argument `skip_dets_and_obs=True`.\n"
                        "Alternatively, pass the argument `open_qasm_version=3`.");
                }
                if (instruction.gate_type == GateType::DETECTOR) {
                    out << "dets[" << detector_offset << "] = ";
                    detector_offset++;
                } else {
                    out << "obs[" << (int)instruction.args[0] << "] = obs[" << (int)instruction.args[0] << "] ^ ";
                }

                int ref_value = 0;
                bool had_paulis = false;
                for (auto t : instruction.targets) {
                    if (t.is_measurement_record_target()) {
                        auto i = measurement_offset + t.rec_offset();
                        ref_value ^= reference_sample[i];
                        out << "rec[" << (measurement_offset + t.rec_offset()) << "] ^ ";
                    } else if (t.is_pauli_target()) {
                        had_paulis = true;
                    } else {
                        throw std::invalid_argument("Unexpected target for OBSERVABLE_INCLUDE: " + t.str());
                    }
                }
                out << ref_value << ";\n";

                if (had_paulis) {
                    out << "// Warning: ignored pauli terms in " << instruction << "\n";
                }
                return;
            }

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
                throw std::invalid_argument(
                    "The circuit contains noise, but OPENQASM 2 doesn't support noise operations.\n"
                    "Use `stim.Circuit.without_noise` to get a version of the circuit without noise.");

            case GateType::MPP:
                output_decomposed_mpp_operation(instruction);
                return;
            case GateType::SPP:
            case GateType::SPP_DAG:
                output_decomposed_spp_or_spp_dag_operation(instruction);
                return;

            default:
                break;
        }

        if (f & (stim::GATE_IS_RESET | stim::GATE_PRODUCES_RESULTS)) {
            output_decomposable_instruction(instruction, open_qasm_version == 2);
            return;
        }

        if (f & stim::GATE_IS_UNITARY) {
            if (f & stim::GATE_IS_SINGLE_QUBIT_GATE) {
                for (const auto &t : instruction.targets) {
                    assert(t.is_qubit_target());
                    out << qasm_names[(int)instruction.gate_type] << " q[" << t.qubit_value() << "];\n";
                }
                return;
            }
            if (f & stim::GATE_TARGETS_PAIRS) {
                output_two_qubit_unitary_instruction_with_possible_feedback(instruction);
                return;
            }
        }

        throw std::invalid_argument("Not implemented in QasmExporter::output_instruction: " + instruction.str());
    }
};

void stim::export_open_qasm(const Circuit &circuit, std::ostream &out, int open_qasm_version, bool skip_dets_and_obs) {
    if (open_qasm_version != 2 && open_qasm_version != 3) {
        throw std::invalid_argument("Only open_qasm_version=2 and open_qasm_version=3 are supported.");
    }

    QasmExporter exporter(out, circuit, open_qasm_version, skip_dets_and_obs);
    exporter.output_header();
    exporter.define_all_gates_and_output_gate_declarations();
    exporter.output_storage_declarations();
    circuit.for_each_operation([&](const CircuitInstruction &instruction) {
        exporter.output_instruction(instruction);
    });
}
