#include "stim/util_top/stabilizers_to_tableau.h"

namespace stim {

template <size_t W>
Tableau<W> stabilizers_to_tableau(
    const std::vector<PauliString<W>> &stabilizers, bool allow_redundant, bool allow_underconstrained, bool invert) {
    std::cerr << "inside A1\n";
    size_t num_qubits = 0;
    for (const auto &e : stabilizers) {
        num_qubits = std::max(num_qubits, e.num_qubits);
    }

    std::cerr << "inside A2\n";
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

    std::cerr << "inside A3\n";

    std::cerr << "inside A4\n";
    size_t used = 0;
    {
        size_t k = 0;
        for (size_t k1 = 0; k1 < 5; k1++) {
            std::cerr << "k=" << k << ", k1=" << k1 << ": ";
            for (size_t k2 = 0; k2 < 5; k2++) {
                std::cerr << "_XZY"[buf_xs[k1][k2] + 2*buf_zs[k1][k2]];
            }
            std::cerr << "\n";
        }

        // Find a non-identity term in the Pauli string past the region used by other stabilizers.
        size_t pivot = 0;
        for (pivot = used; pivot < num_qubits; pivot++) {
            if (buf_xs[pivot][k] || buf_zs[pivot][k]) {
                break;
            }
        }
        std::cerr << "chose pivot " << pivot << " in iter " << k << "\n";

        // Change pivot basis to the Z axis.
        if (buf_xs[pivot][k]) {
            GateType g = buf_zs[pivot][k] ? GateType::H_YZ : GateType::H;
            size_t q = pivot;
            simd_bits_range_ref<W> xs1 = buf_xs[q];
            simd_bits_range_ref<W> zs1 = buf_zs[q];
            simd_bits_range_ref<W> ss = buf_signs;
            switch (g) {
                case GateType::H_YZ:
                    std::cerr << "    pivot change basis Y " << k << "\n";
                    ss.for_each_word(xs1, zs1, [](auto &s, auto &x, auto &z) {
                        x ^= z;
                        s ^= z.andnot(x);
                    });
                    break;
                case GateType::H:
                    std::cerr << "    pivot change basis X " << k << "\n";
                    // for (size_t k1 = 0; k1 < 5; k1++) {
                    //     std::cerr << "BEFORE k=" << k << ", k1=" << k1 << ": ";
                    //     for (size_t k2 = 0; k2 < 5; k2++) {
                    //         std::cerr << "_XZY"[buf_xs[k1][k2] + 2*buf_zs[k1][k2]];
                    //     }
                    //     std::cerr << "\n";
                    // }
                    ss.for_each_word(xs1, zs1, [](auto &s, auto &x, auto &z) {
                        std::swap(x, z);
                        s ^= x & z;
                    });
                    // for (size_t k1 = 0; k1 < 5; k1++) {
                    //     std::cerr << "AFTER k=" << k << ", k1=" << k1 << ": ";
                    //     for (size_t k2 = 0; k2 < 5; k2++) {
                    //         std::cerr << "_XZY"[buf_xs[k1][k2] + 2*buf_zs[k1][k2]];
                    //     }
                    //     std::cerr << "\n";
                    // }
                    break;
                default:
                    throw std::invalid_argument("Unrecognized gate type.");
            }
        }


        std::cerr << "    pivot done\n";
        used++;
    }

    {
        size_t k = 1;
        for (size_t k1 = 0; k1 < 5; k1++) {
            std::cerr << "k=" << k << ", k1=" << k1 << ": ";
            for (size_t k2 = 0; k2 < 5; k2++) {
                std::cerr << "_XZY"[buf_xs[k1][k2] + 2*buf_zs[k1][k2]];
            }
            std::cerr << "\n";
        }
    }
    std::cerr << "done\n";
    return Tableau<W>(3);
}

}  // namespace stim
