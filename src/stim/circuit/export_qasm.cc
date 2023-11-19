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

#include "stim/circuit/export_qasm.h"

#include <bitset>

#include "stim/simulators/tableau_simulator.h"

using namespace stim;

static void do_decomposed(
    GateType g, const char *q0_name, const char *q1_name, const char *m_name, std::ostream &out) {
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
                    out << "measure " << q2n(t) << " -> " << m_name << ";";
                }
                break;
            default:
                throw std::invalid_argument("Unhandled: " + inst.str());
        }
    }
}

static void do_qasm_decompose_mpp(
    const CircuitInstruction &inst, size_t num_qubits, uint64_t &measurement_offset, std::ostream &out) {
    decompose_mpp_operation(
        inst,
        num_qubits,
        [&](const CircuitInstruction &h_xz,
            const CircuitInstruction &h_yz,
            const CircuitInstruction &cnot,
            const CircuitInstruction &meas) {
            for (const auto t : h_xz.targets) {
                out << "h q[" << t.qubit_value() << "];";
            }
            for (const auto t : h_yz.targets) {
                out << "sx q[" << t.qubit_value() << "];";
            }
            for (size_t k = 0; k < cnot.targets.size(); k += 2) {
                auto t1 = cnot.targets[k];
                auto t2 = cnot.targets[k + 1];
                out << "cx q[" << t1.qubit_value() << "],q[" << t2.qubit_value() << "];";
            }
            for (auto t : meas.targets) {
                out << "measure q[" << t.qubit_value() << "] -> rec[" << measurement_offset << "];";
                measurement_offset++;
            }
            for (size_t k = 0; k < cnot.targets.size(); k += 2) {
                auto t1 = cnot.targets[k];
                auto t2 = cnot.targets[k + 1];
                out << "cx q[" << t1.qubit_value() << "],q[" << t2.qubit_value() << "];";
            }
            for (const auto t : h_yz.targets) {
                out << "sxdg q[" << t.qubit_value() << "];";
            }
            for (const auto t : h_xz.targets) {
                out << "h q[" << t.qubit_value() << "];";
            }
            out << " // decomposed MPP\n";
        });
}

static void qasm_output_decomposed_inline(
    const CircuitInstruction &instruction,
    const std::array<const char *, NUM_DEFINED_GATES> &qasm_names,
    uint64_t &measurement_offset,
    std::ostream &out,
    bool decompose_inline) {
    auto f = GATE_DATA[instruction.gate_type].flags;
    std::stringstream q0;
    std::stringstream q1;
    std::stringstream m;
    auto step = (f & GATE_TARGETS_PAIRS) ? 2 : 1;
    for (size_t k = 0; k < instruction.targets.size(); k += step) {
        auto t0 = instruction.targets[k];
        auto t1 = instruction.targets[k + step - 1];
        if (decompose_inline) {
            q0.str("");
            q1.str("");
            q0 << "q[" << t0.qubit_value() << "]";
            q1 << "q[" << t1.qubit_value() << "]";
            if (f & GATE_PRODUCES_RESULTS) {
                m.str("");
                m << "rec[" << measurement_offset << "]";
                measurement_offset++;
            }
            do_decomposed(
                instruction.gate_type, q0.str().data(), q1.str().data(), m.str().data(), out);
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
            out << ");\n";
        }
    }
}

static void qasm_output_two_qubit_unitary(
    const CircuitInstruction &instruction,
    const std::array<const char *, NUM_DEFINED_GATES> &qasm_names,
    uint64_t &measurement_offset,
    int open_qasm_version,
    std::ostream &out) {
    for (size_t k = 0; k < instruction.targets.size(); k += 2) {
        auto t1 = instruction.targets[k];
        auto t2 = instruction.targets[k + 1];
        if (t1.is_qubit_target() && t2.is_qubit_target()) {
            out << qasm_names[(int)instruction.gate_type] << " q[" << t1.qubit_value() << "], q[" << t2.qubit_value()
                << "];\n";
        } else if (t1.is_qubit_target() || t2.is_qubit_target()) {
            if (open_qasm_version == 2) {
                throw std::invalid_argument(
                    "The circuit contains feedback, but OPENQASM 2 doesn't support feedback.\n"
                    "You can use `stim.Circuit.with_inlined_feedback` to drop feedback operations.\n"
                    "Alternatively, pass the argument `open_qasm_version=3`.");
            }
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
                    throw std::invalid_argument("Not implemented: " + instruction.str());
            }
            out << "if (";
            if (t1.is_measurement_record_target()) {
                out << "ms[" << (measurement_offset + t1.rec_offset()) << "]";
            } else if (t1.is_sweep_bit_target()) {
                out << "sweep[" << t1.value() << "]";
            } else {
                throw std::invalid_argument("Not implemented: " + instruction.str());
            }
            out << ") {\n";
            out << "    " << basis << " q[" << target.qubit_value() << "];\n";
            out << "}\n";
        }
    }
}

static void define_custom_single_qubit_gate(
    GateType g,
    const char *name,
    std::array<const char *, NUM_DEFINED_GATES> &qasm_names,
    const std::bitset<NUM_DEFINED_GATES> &used,
    std::ostream &out) {
    const auto &gate = GATE_DATA[g];
    qasm_names[(int)g] = name;
    if (!used[(int)g]) {
        return;
    }

    out << "gate " << name << " q0 { U(";
    auto xyz = gate.to_euler_angles();
    std::array<const char *, 4> angles{"0", "pi/2", "pi", "-pi/2"};
    out << angles[round(xyz[0] / 3.14159265359f)];
    out << ", " << angles[round(xyz[1] / 3.14159265359f)];
    out << ", " << angles[round(xyz[2] / 3.14159265359f)];
    out << ") q0; }\n";
}

static void define_custom_decomposed_gate(
    GateType g,
    const char *name,
    std::array<const char *, NUM_DEFINED_GATES> &qasm_names,
    const std::bitset<NUM_DEFINED_GATES> &used,
    std::ostream &out,
    int open_qasm_version) {
    const auto &gate = GATE_DATA[g];
    qasm_names[(int)g] = name;
    if (!used[(int)g]) {
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

    do_decomposed(g, "q0", "q1", "b", out);
    if (num_measurements > 0) {
        out << " return b;";
    }
    out << " }\n";
}

static void collect_used_gates(const Circuit &c, std::bitset<NUM_DEFINED_GATES> &used) {
    for (const auto &inst : c.operations) {
        used[(int)inst.gate_type] = true;
        if (inst.gate_type == GateType::REPEAT) {
            collect_used_gates(inst.repeat_block_body(c), used);
        }
    }
}

static std::array<const char *, NUM_DEFINED_GATES> define_qasm_gates(
    std::ostream &out, const std::bitset<NUM_DEFINED_GATES> &used, int open_qasm_version) {
    std::array<const char *, NUM_DEFINED_GATES> qasm_names;

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
    define_custom_single_qubit_gate(GateType::C_XYZ, "cxyz", qasm_names, used, out);
    define_custom_single_qubit_gate(GateType::C_ZYX, "czyx", qasm_names, used, out);
    define_custom_single_qubit_gate(GateType::SQRT_Y, "sy", qasm_names, used, out);
    define_custom_single_qubit_gate(GateType::SQRT_Y_DAG, "sydg", qasm_names, used, out);
    define_custom_single_qubit_gate(GateType::H_XY, "hxy", qasm_names, used, out);
    define_custom_single_qubit_gate(GateType::H_YZ, "hyz", qasm_names, used, out);
    define_custom_decomposed_gate(GateType::XCX, "xcx", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::XCY, "xcy", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::XCZ, "xcz", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::YCX, "ycx", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::YCY, "ycy", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::YCZ, "ycz", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::SQRT_XX, "sxx", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::SQRT_XX_DAG, "sxxdg", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::SQRT_YY, "syy", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::SQRT_YY_DAG, "syydg", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::SQRT_ZZ, "szz", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::SQRT_ZZ_DAG, "szzdg", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::ISWAP, "iswap", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::CXSWAP, "cxswap", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::SWAPCX, "swapcx", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::ISWAP_DAG, "iswapdg", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::MX, "mx", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::MY, "my", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::MRX, "mrx", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::MRY, "mry", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::MR, "mr", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::RX, "rx", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::RY, "ry", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::MXX, "mxx", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::MYY, "myy", qasm_names, used, out, open_qasm_version);
    define_custom_decomposed_gate(GateType::MZZ, "mzz", qasm_names, used, out, open_qasm_version);
    out << "\n";

    return qasm_names;
}

void stim::export_open_qasm(const Circuit &circuit, std::ostream &out, int open_qasm_version, bool skip_dets_and_obs) {
    auto stats = circuit.compute_stats();
    std::bitset<NUM_DEFINED_GATES> used;
    collect_used_gates(circuit, used);
    if (open_qasm_version != 2 && open_qasm_version != 3) {
        throw std::invalid_argument("Only open_qasm_version=2 and open_qasm_version=3 are supported.");
    }

    // Header.
    if (open_qasm_version == 2) {
        out << "OPENQASM 2.0;\n";
    } else {
        out << "OPENQASM 3.0;\n";
    }
    out << "\n";

    // Gate definitions.
    std::array<const char *, NUM_DEFINED_GATES> qasm_names = define_qasm_gates(out, used, open_qasm_version);

    // Storage.
    const char *qubit_decl = "qubit";
    const char *bit_decl = "bit";
    if (open_qasm_version == 2) {
        qubit_decl = "qreg";
        bit_decl = "creg";
    }
    if (stats.num_qubits > 0) {
        out << qubit_decl << " q[" << stats.num_qubits << "];\n";
    }
    if (stats.num_measurements > 0) {
        out << bit_decl << " rec[" << stats.num_measurements << "];\n";
    }
    if (stats.num_detectors > 0 && !skip_dets_and_obs) {
        out << bit_decl << " dets[" << stats.num_detectors << "];\n";
    }
    if (stats.num_observables > 0 && !skip_dets_and_obs) {
        out << bit_decl << " obs[" << stats.num_observables << "];\n";
    }
    if (stats.num_sweep_bits > 0) {
        out << bit_decl << " sweep[" << stats.num_sweep_bits << "];\n";
    }
    out << "\n";

    simd_bits<64> reference_sample(stats.num_measurements);
    if (stats.num_detectors > 0 || stats.num_observables > 0) {
        reference_sample = TableauSimulator<64>::reference_sample_circuit(circuit);
    }

    // Body.
    uint64_t measurement_offset = 0;
    uint64_t detector_offset = 0;
    circuit.for_each_operation([&](const CircuitInstruction &instruction) {
        GateFlags f = GATE_DATA[instruction.gate_type].flags;

        switch (instruction.gate_type) {
            case GateType::QUBIT_COORDS:
            case GateType::SHIFT_COORDS:
                // Skipped.
                return;

            case GateType::MPAD:
                measurement_offset += instruction.count_measurement_results();
                return;

            case GateType::TICK:
                out << "barrier q;\n\n";
                return;

            case GateType::M:
                for (const auto &t : instruction.targets) {
                    out << "measure q[" << t.qubit_value() << "] -> rec[" << measurement_offset << "];\n";
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
                for (auto t : instruction.targets) {
                    assert(t.is_measurement_record_target());
                    auto i = measurement_offset + t.rec_offset();
                    ref_value ^= reference_sample[i];
                    out << "rec[" << (measurement_offset + t.rec_offset()) << "] ^ ";
                }
                out << ref_value << ";\n";
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
                do_qasm_decompose_mpp(instruction, stats.num_qubits, measurement_offset, out);
                return;

            default:
                break;
        }

        if (f & (stim::GATE_IS_RESET | stim::GATE_PRODUCES_RESULTS)) {
            qasm_output_decomposed_inline(instruction, qasm_names, measurement_offset, out, open_qasm_version == 2);
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
                qasm_output_two_qubit_unitary(instruction, qasm_names, measurement_offset, skip_dets_and_obs, out);
                return;
            }
        }

        throw std::invalid_argument("Not implemented: " + instruction.str());
    });
}
