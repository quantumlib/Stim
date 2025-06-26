#include "stim/util_top/stabilizers_to_tableau.h"

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

    size_t used = 0;
    {
        // Change pivot basis to the Z axis.
        // for (size_t k1 = 0; k1 < 5; k1++) {
        //     std::cerr << "BEFORE k=" << k << ", k1=" << k1 << ": ";
        //     for (size_t k2 = 0; k2 < 5; k2++) {
        //         std::cerr << "_XZY"[buf_xs[k1][k2] + 2*buf_zs[k1][k2]];
        //     }
        //     std::cerr << "\n";
        // }
        auto *v0 = buf_signs.ptr_simd;
        auto *v1 = buf_xs[0].ptr_simd;
        auto *v2 = buf_zs[0].ptr_simd;
        auto *v0_end = v0 + buf_signs.num_simd_words;
        while (v0 != v0_end) {
            std::swap(*v1, *v2);
            *v0 ^= *v1 & *v2;
            v0++;
            v1++;
            v2++;
        }
        // for (size_t k1 = 0; k1 < 5; k1++) {
        //     std::cerr << "AFTER k=" << k << ", k1=" << k1 << ": ";
        //     for (size_t k2 = 0; k2 < 5; k2++) {
        //         std::cerr << "_XZY"[buf_xs[k1][k2] + 2*buf_zs[k1][k2]];
        //     }
        //     std::cerr << "\n";
        // }


        std::cerr << "    pivot done\n";
        used++;
    }

    std::cerr << "_XZY"[buf_xs[0][0] + 2*buf_zs[0][0]] << "\n";
    return Tableau<W>(3);
}

}  // namespace stim
