#include "stim/probability_util.h"
#include "stim/simulators/graph_simulator.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/conversions.h"
#include "stim/util_top/circuit_inverse_unitary.h"
#include "stim/util_top/circuit_vs_tableau.h"

namespace stim {

template <size_t W>
Tableau<W> stabilizers_to_tableau(
    const std::vector<PauliString<W>> &stabilizers, bool allow_redundant, bool allow_underconstrained, bool invert) {
    size_t num_qubits = 0;
    for (const auto &e : stabilizers) {
        num_qubits = std::max(num_qubits, e.num_qubits);
    }

    simd_bit_table<W> buf_xs(stabilizers.size(), num_qubits);
    simd_bit_table<W> buf_zs(stabilizers.size(), num_qubits);
    simd_bits<W> buf_signs(stabilizers.size());
    for (size_t k = 0; k < stabilizers.size(); k++) {
        memcpy(buf_xs[k].u8, stabilizers[k].xs.u8, stabilizers[k].xs.num_u8_padded());
        memcpy(buf_zs[k].u8, stabilizers[k].zs.u8, stabilizers[k].zs.num_u8_padded());
        buf_signs[k] = stabilizers[k].sign;
    }
    buf_xs = buf_xs.transposed();
    buf_zs = buf_zs.transposed();

    Circuit elimination_instructions;

    size_t used = 0;
    for (size_t k = 0; k < stabilizers.size(); k++) {
        // Find a non-identity term in the Pauli string past the region used by other stabilizers.
        size_t pivot;
        for (pivot = used; pivot < num_qubits; pivot++) {
            if (buf_xs[pivot][k] || buf_zs[pivot][k]) {
                break;
            }
        }

        // Check for incompatible / redundant stabilizers.
        if (pivot == num_qubits) {
            if (buf_signs[k]) {
                throw std::invalid_argument("Some of the given stabilizers contradict each other.");
            }
            if (!allow_redundant) {
                throw std::invalid_argument(
                    "Didn't specify allow_redundant=True but one of the given stabilizers is a product of the others. "
                    "To allow redundant stabilizers, pass the argument allow_redundant=True.");
            }
            continue;
        }

        // Change pivot basis to the Z axis.
        if (buf_xs[pivot][k]) {
            GateType g = buf_zs[pivot][k] ? GateType::H_YZ : GateType::H;
            GateTarget t = GateTarget::qubit(pivot);
            CircuitInstruction instruction{g, {}, &t};
            elimination_instructions.safe_append(instruction);
            size_t q = pivot;
            simd_bits_range_ref<W> xs1 = buf_xs[q];
            simd_bits_range_ref<W> zs1 = buf_zs[q];
            simd_bits_range_ref<W> ss = buf_signs;
            switch (g) {
                case GateType::H_YZ:
                    ss.for_each_word(xs1, zs1, [](auto &s, auto &x, auto &z) {
                        x ^= z;
                        s ^= z.andnot(x);
                    });
                    break;
                case GateType::H:
                    ss.for_each_word(xs1, zs1, [](auto &s, auto &x, auto &z) {
                        std::swap(x, z);
                        s ^= x & z;
                    });
                    break;
                default:
                    throw std::invalid_argument("Unrecognized gate type.");
            }
        }

        // Cancel other terms in Pauli string.
        for (size_t q = 0; q < num_qubits; q++) {
            int p = buf_xs[q][k] + buf_zs[q][k] * 2;
            if (p && q != pivot) {
                std::array<GateTarget, 2> targets{GateTarget::qubit(pivot), GateTarget::qubit(q)};
                GateType g = p == 1 ? GateType::XCX : p == 2 ? GateType::XCZ : GateType::XCY;
                CircuitInstruction instruction{g, {}, targets};
                elimination_instructions.safe_append(instruction);
                size_t q1 = targets[0].qubit_value();
                size_t q2 = targets[1].qubit_value();
                simd_bits_range_ref<W> ss = buf_signs;
                simd_bits_range_ref<W> xs1 = buf_xs[q1];
                simd_bits_range_ref<W> zs1 = buf_zs[q1];
                simd_bits_range_ref<W> xs2 = buf_xs[q2];
                simd_bits_range_ref<W> zs2 = buf_zs[q2];
                switch (g) {
                    case GateType::XCX:
                        ss.for_each_word(xs1, zs1, xs2, zs2, [](auto &s, auto &x1, auto &z1, auto &x2, auto &z2) {
                            s ^= (x1 ^ x2) & z1 & z2;
                            x1 ^= z2;
                            x2 ^= z1;
                        });
                        break;
                    case GateType::XCY:
                        ss.for_each_word(xs1, zs1, xs2, zs2, [](auto &s, auto &x1, auto &z1, auto &x2, auto &z2) {
                            x1 ^= x2 ^ z2;
                            x2 ^= z1;
                            z2 ^= z1;
                            s ^= x1.andnot(z1) & x2.andnot(z2);
                            s ^= x1 & z1 & z2.andnot(x2);
                        });
                        break;
                    case GateType::XCZ:
                        ss.for_each_word(xs1, zs1, xs2, zs2, [](auto &s, auto &x1, auto &z1, auto &x2, auto &z2) {
                            z2 ^= z1;
                            x1 ^= x2;
                            s ^= (z2 ^ x1).andnot(z1 & x2);
                        });
                        break;
                    default:
                        throw std::invalid_argument("Unrecognized gate type.");
                }
            }
        }

        // Move pivot to diagonal.
        if (pivot != used) {
            std::array<GateTarget, 2> targets{GateTarget::qubit(pivot), GateTarget::qubit(used)};
            CircuitInstruction instruction{GateType::SWAP, {}, targets};
            elimination_instructions.safe_append(instruction);
            buf_xs[pivot].swap_with(buf_xs[used]);
            buf_zs[pivot].swap_with(buf_zs[used]);
        }

        // Fix sign.
        if (buf_signs[k]) {
            GateTarget t = GateTarget::qubit(used);
            CircuitInstruction instruction{GateType::X, {}, &t};
            elimination_instructions.safe_append(instruction);
            buf_signs ^= buf_zs[used];
        }

        used++;
    }

    // All stabilizers will have been mapped into Z products, if they commuted.
    for (size_t q = 0; q < num_qubits; q++) {
        if (buf_xs[q].not_zero()) {
            for (size_t k1 = 0; k1 < stabilizers.size(); k1++) {
                for (size_t k2 = k1 + 1; k2 < stabilizers.size(); k2++) {
                    if (!stabilizers[k1].ref().commutes(stabilizers[k2])) {
                        std::stringstream ss;
                        ss << "Some of the given stabilizers anticommute.\n";
                        ss << "For example:\n    ";
                        ss << stabilizers[k1];
                        ss << "\nanticommutes with\n";
                        ss << stabilizers[k2] << "\n";
                        throw std::invalid_argument(ss.str());
                    }
                }
            }
            throw std::invalid_argument("The given stabilizers commute but the solver failed in a way that suggests they anticommute. Please report this as a bug.");
        }
    }

    if (used < num_qubits) {
        if (!allow_underconstrained) {
            throw std::invalid_argument(
                "There weren't enough stabilizers to uniquely specify the state. "
                "To allow underspecifying the state, pass the argument allow_underconstrained=True.");
        }
    }

    if (num_qubits > 0) {
        // Force size of resulting tableau to be correct.
        GateTarget t = GateTarget::qubit(num_qubits - 1);
        elimination_instructions.safe_append(CircuitInstruction{GateType::X, {}, &t});
        elimination_instructions.safe_append(CircuitInstruction{GateType::X, {}, &t});
    }

    if (invert) {
        return circuit_to_tableau<W>(elimination_instructions.inverse(), false, false, false, true);
    }
    return circuit_to_tableau<W>(elimination_instructions, false, false, false, true);
}

}  // namespace stim
