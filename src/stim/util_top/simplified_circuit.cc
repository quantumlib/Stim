#include "stim/util_top/simplified_circuit.h"

#include <functional>

#include "stim/circuit/gate_decomposition.h"
#include "stim/mem/simd_bits.h"

using namespace stim;

struct Simplifier {
    size_t num_qubits;
    std::function<void(const CircuitInstruction &inst)> yield;
    simd_bits<64> used;
    std::vector<GateTarget> qs1_buf;
    std::vector<GateTarget> qs2_buf;
    std::vector<GateTarget> qs_buf;

    Simplifier(size_t num_qubits, std::function<void(const CircuitInstruction &inst)> init_yield)
        : num_qubits(num_qubits), yield(init_yield), used(num_qubits) {
    }

    void do_xcz(SpanRef<const GateTarget> targets, std::string_view tag) {
        if (targets.empty()) {
            return;
        }

        qs_buf.clear();
        for (size_t k = 0; k < targets.size(); k += 2) {
            qs_buf.push_back(targets[k + 1]);
            qs_buf.push_back(targets[k]);
        }
        yield(CircuitInstruction{GateType::CX, {}, qs_buf, tag});
    }

    void simplify_potentially_overlapping_1q_instruction(const CircuitInstruction &inst) {
        used.clear();

        size_t start = 0;
        for (size_t k = 0; k < inst.targets.size(); k++) {
            auto t = inst.targets[k];
            if (t.has_qubit_value() && used[t.qubit_value()]) {
                CircuitInstruction disjoint =
                    CircuitInstruction{inst.gate_type, inst.args, inst.targets.sub(start, k), inst.tag};
                simplify_disjoint_1q_instruction(disjoint);
                used.clear();
                start = k;
            }
            if (t.has_qubit_value()) {
                used[t.qubit_value()] = true;
            }
        }
        simplify_disjoint_1q_instruction(
            CircuitInstruction{inst.gate_type, inst.args, inst.targets.sub(start, inst.targets.size()), inst.tag});
    }

    void simplify_potentially_overlapping_2q_instruction(const CircuitInstruction &inst) {
        used.clear();

        size_t start = 0;
        for (size_t k = 0; k < inst.targets.size(); k += 2) {
            auto a = inst.targets[k];
            auto b = inst.targets[k + 1];
            if ((a.has_qubit_value() && used[a.qubit_value()]) || (b.has_qubit_value() && used[b.qubit_value()])) {
                CircuitInstruction disjoint =
                    CircuitInstruction{inst.gate_type, inst.args, inst.targets.sub(start, k), inst.tag};
                simplify_disjoint_2q_instruction(disjoint);
                used.clear();
                start = k;
            }
            if (a.has_qubit_value()) {
                used[a.qubit_value()] = true;
            }
            if (b.has_qubit_value()) {
                used[b.qubit_value()] = true;
            }
        }
        simplify_disjoint_2q_instruction(
            CircuitInstruction{inst.gate_type, inst.args, inst.targets.sub(start, inst.targets.size()), inst.tag});
    }

    void simplify_disjoint_1q_instruction(const CircuitInstruction &inst) {
        const auto &ts = inst.targets;

        switch (inst.gate_type) {
            case GateType::I:
                // Do nothing.
                break;
            case GateType::X:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::Y:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::Z:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::C_XYZ:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::C_NXYZ:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::C_XNYZ:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::C_XYNZ:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::C_ZYX:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::C_ZYNX:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::C_ZNYX:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::C_NZYX:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::H:
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::H_XY:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::H_YZ:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::H_NXY:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::H_NXZ:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::H_NYZ:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::S:
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::SQRT_X:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::SQRT_X_DAG:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::SQRT_Y:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::SQRT_Y_DAG:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::S_DAG:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;

            case GateType::MX:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::M, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::MY:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::M, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::M:
                yield({GateType::M, {}, ts, inst.tag});
                break;
            case GateType::MRX:
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::M, {}, ts, inst.tag});
                yield({GateType::R, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::MRY:
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::M, {}, ts, inst.tag});
                yield({GateType::R, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::MR:
                yield({GateType::M, {}, ts, inst.tag});
                yield({GateType::R, {}, ts, inst.tag});
                break;
            case GateType::RX:
                yield({GateType::R, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                break;
            case GateType::RY:
                yield({GateType::R, {}, ts, inst.tag});
                yield({GateType::H, {}, ts, inst.tag});
                yield({GateType::S, {}, ts, inst.tag});
                break;
            case GateType::R:
                yield({GateType::R, {}, ts, inst.tag});
                break;

            default:
                throw std::invalid_argument("Unhandled in Simplifier::simplify_disjoint_1q_instruction: " + inst.str());
        }
    }

    void simplify_disjoint_2q_instruction(const CircuitInstruction &inst) {
        const auto &ts = inst.targets;
        qs_buf.clear();
        qs1_buf.clear();
        qs2_buf.clear();
        for (size_t k = 0; k < inst.targets.size(); k += 2) {
            auto a = inst.targets[k];
            auto b = inst.targets[k + 1];
            if (a.has_qubit_value()) {
                auto t = GateTarget::qubit(a.qubit_value());
                qs1_buf.push_back(t);
                qs_buf.push_back(t);
            }
            if (b.has_qubit_value()) {
                auto t = GateTarget::qubit(b.qubit_value());
                qs2_buf.push_back(t);
                qs_buf.push_back(t);
            }
        }

        switch (inst.gate_type) {
            case GateType::CX:
                yield({GateType::CX, {}, ts, inst.tag});
                break;
            case GateType::XCZ:
                do_xcz(ts, inst.tag);
                break;
            case GateType::XCX:
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                break;
            case GateType::XCY:
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                break;
            case GateType::YCX:
                yield({GateType::S, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs1_buf, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs1_buf, inst.tag});
                break;
            case GateType::YCY:
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                break;
            case GateType::YCZ:
                yield({GateType::S, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs1_buf, inst.tag});
                do_xcz(ts, inst.tag);
                yield({GateType::S, {}, qs1_buf, inst.tag});
                break;
            case GateType::CY:
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                break;
            case GateType::CZ:
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs2_buf, inst.tag});
                break;
            case GateType::SQRT_XX:
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::H, {}, qs_buf, inst.tag});
                break;
            case GateType::SQRT_XX_DAG:
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::H, {}, qs_buf, inst.tag});
                break;
            case GateType::SQRT_YY:
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::H, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                break;
            case GateType::SQRT_YY_DAG:
                yield({GateType::S, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs1_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::H, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                break;
            case GateType::SQRT_ZZ:
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                break;
            case GateType::SQRT_ZZ_DAG:
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                break;
            case GateType::SWAP:
                yield({GateType::CX, {}, ts, inst.tag});
                do_xcz(ts, inst.tag);
                yield({GateType::CX, {}, ts, inst.tag});
                break;
            case GateType::ISWAP:
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                do_xcz(ts, inst.tag);
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                break;
            case GateType::ISWAP_DAG:
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                do_xcz(ts, inst.tag);
                yield({GateType::H, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                break;
            case GateType::CXSWAP:
                do_xcz(ts, inst.tag);
                yield({GateType::CX, {}, ts, inst.tag});
                break;
            case GateType::SWAPCX:
                yield({GateType::CX, {}, ts, inst.tag});
                do_xcz(ts, inst.tag);
                break;
            case GateType::CZSWAP:
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                do_xcz(ts, inst.tag);
                yield({GateType::H, {}, qs2_buf, inst.tag});
                break;

            case GateType::MXX:
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::M, {}, qs1_buf, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                break;
            case GateType::MYY:
                yield({GateType::S, {}, qs_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::S, {}, qs2_buf, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::M, {}, qs1_buf, inst.tag});
                yield({GateType::H, {}, qs1_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::S, {}, qs_buf, inst.tag});
                break;
            case GateType::MZZ:
                yield({GateType::CX, {}, ts, inst.tag});
                yield({GateType::M, {}, qs2_buf, inst.tag});
                yield({GateType::CX, {}, ts, inst.tag});
                break;

            default:
                throw std::invalid_argument("Unhandled in Simplifier::simplify_instruction: " + inst.str());
        }
    }

    void simplify_instruction(const CircuitInstruction &inst) {
        const Gate &g = GATE_DATA[inst.gate_type];

        switch (inst.gate_type) {
            case GateType::I:
            case GateType::II:
                // Dropped.
                break;

            case GateType::MPP:
                decompose_mpp_operation(inst, num_qubits, [&](const CircuitInstruction sub) {
                    simplify_instruction(sub);
                });
                break;
            case GateType::SPP:
            case GateType::SPP_DAG:
                decompose_spp_or_spp_dag_operation(inst, num_qubits, false, [&](const CircuitInstruction sub) {
                    simplify_instruction(sub);
                });
                break;

            case GateType::MPAD:
                // Can't be easily simplified into M.
                yield(inst);
                break;

            case GateType::DETECTOR:
            case GateType::OBSERVABLE_INCLUDE:
            case GateType::TICK:
            case GateType::QUBIT_COORDS:
            case GateType::SHIFT_COORDS:
                // Annotations can't be simplified.
                yield(inst);
                break;

            case GateType::DEPOLARIZE1:
            case GateType::DEPOLARIZE2:
            case GateType::X_ERROR:
            case GateType::Y_ERROR:
            case GateType::Z_ERROR:
            case GateType::I_ERROR:
            case GateType::II_ERROR:
            case GateType::PAULI_CHANNEL_1:
            case GateType::PAULI_CHANNEL_2:
            case GateType::E:
            case GateType::ELSE_CORRELATED_ERROR:
            case GateType::HERALDED_ERASE:
            case GateType::HERALDED_PAULI_CHANNEL_1:
                // Noise isn't simplified.
                yield(inst);
                break;

            default: {
                if (g.flags & GATE_IS_SINGLE_QUBIT_GATE) {
                    simplify_potentially_overlapping_1q_instruction(inst);
                } else if (g.flags & GATE_TARGETS_PAIRS) {
                    simplify_potentially_overlapping_2q_instruction(inst);
                } else {
                    throw std::invalid_argument(
                        "Unhandled in simplify_potentially_overlapping_instruction: " + inst.str());
                }
            }
        }
    }
};

Circuit stim::simplified_circuit(const Circuit &circuit) {
    Circuit output;
    Simplifier simplifier(circuit.count_qubits(), [&](const CircuitInstruction &inst) {
        output.safe_append(inst);
    });
    for (auto inst : circuit.operations) {
        if (inst.gate_type == GateType::REPEAT) {
            output.append_repeat_block(
                inst.repeat_block_rep_count(), simplified_circuit(inst.repeat_block_body(circuit)), inst.tag);
        } else {
            simplifier.simplify_instruction(inst);
        }
    }
    return output;
}
