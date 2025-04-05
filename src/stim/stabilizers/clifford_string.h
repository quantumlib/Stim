/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_STABILIZERS_CLIFFORD_STRING_H
#define _STIM_STABILIZERS_CLIFFORD_STRING_H

#include "stim/mem/simd_bits.h"
#include "stim/gates/gates.h"
#include "stim/circuit/circuit.h"

namespace stim {

/// A fixed-size list of W single-qubit Clifford rotations.
template <typename Word>
struct CliffordWord {
    Word x_signs;
    Word z_signs;
    Word inv_x2x;  // Inverted so that zero-initializing gives the identity gate.
    Word x2z;
    Word z2x;
    Word inv_z2z;  // Inverted so that zero-initializing gives the identity gate.
};

inline GateType bits2gate(std::array<bool, 6> bits) {
    constexpr std::array<GateType, 64> table{
        GateType::I,
        GateType::X,
        GateType::Z,
        GateType::Y,

        GateType::NOT_A_GATE,  // These should be impossible if the class is in a good state.
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::S,
        GateType::H_XY,
        GateType::S_DAG,
        GateType::H_NXY,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::SQRT_X_DAG,
        GateType::SQRT_X,
        GateType::H_YZ,
        GateType::H_NYZ,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::C_ZYX,
        GateType::C_ZNYX,
        GateType::C_ZYNX,
        GateType::C_NZYX,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,
        GateType::NOT_A_GATE,

        GateType::C_XYZ,
        GateType::C_XYNZ,
        GateType::C_XNYZ,
        GateType::C_NXYZ,

        GateType::H,
        GateType::SQRT_Y_DAG,
        GateType::SQRT_Y,
        GateType::H_NXZ,
    };
    int k = (bits[0] << 0)
          | (bits[1] << 1)
          | (bits[2] << 2)
          | (bits[3] << 3)
          | (bits[4] << 4)
          | (bits[5] << 5);
    return table[k];
}

inline std::array<bool, 6> gate_to_bits(GateType gate_type) {
    Gate g = GATE_DATA[gate_type];
    if (!(g.flags & GATE_IS_SINGLE_QUBIT_GATE) || !(g.flags & GATE_IS_UNITARY)) {
        throw std::invalid_argument("Not a single qubit gate: " + std::string(g.name));
    }
    const auto &flows = g.flow_data;
    std::string_view tx = flows[0];
    std::string_view tz = flows[1];
    bool z_sign = tz[0] == '-';
    bool inv_x2x = !(tx[1] == 'X' || tx[1] == 'Y');
    bool x2z = tx[1] == 'Z' || tx[1] == 'Y';
    bool x_sign = tx[0] == '-';
    bool z2x = tz[1] == 'X' || tz[1] == 'Y';
    bool inv_z2z = !(tz[1] == 'Z' || tz[1] == 'Y');
    return {z_sign, x_sign, inv_x2x, x2z, z2x, inv_z2z};
}

/// Returns the result of multiplying W rotations pair-wise.
template <typename Word>
inline CliffordWord<Word> operator*(const CliffordWord<Word> &lhs, const CliffordWord<Word> &rhs) {
    CliffordWord<Word> result;

    // I don't have a simple explanation of why this is correct. It was produced by starting from something that was
    // obviously correct, having tests to check all 24*24 cases, then iteratively applying simple rewrites to reduce
    // the number of operations. So the result is correct, but somewhat incomprehensible.
    result.inv_x2x = (lhs.inv_x2x | rhs.inv_x2x) ^ (lhs.z2x & rhs.x2z);
    result.x2z = andnot(rhs.inv_x2x, lhs.x2z) ^ andnot(lhs.inv_z2z, rhs.x2z);
    result.z2x = andnot(lhs.inv_x2x, rhs.z2x) ^ andnot(rhs.inv_z2z, lhs.z2x);
    result.inv_z2z = (lhs.x2z & rhs.z2x) ^ (lhs.inv_z2z | rhs.inv_z2z);

    // I *especially* don't have an explanation of why this part is correct. But every case is tested and verified.
    Word rhs_x2y = andnot(rhs.inv_x2x, rhs.x2z);
    Word rhs_z2y = andnot(rhs.inv_z2z, rhs.z2x);
    Word dy = (lhs.x2z & lhs.z2x) ^ lhs.inv_x2x ^ lhs.z2x ^ lhs.x2z ^ lhs.inv_z2z;
    result.x_signs = rhs.x_signs
        ^ andnot(rhs.inv_x2x, lhs.x_signs)
        ^ (rhs_x2y & dy)
        ^ (rhs.x2z & lhs.z_signs);
    result.z_signs = rhs.z_signs
        ^ (rhs.z2x & lhs.x_signs)
        ^ (rhs_z2y & dy)
        ^ andnot(rhs.inv_z2z, lhs.z_signs);

    return result;
}

template <size_t W>
struct CliffordString;

template <size_t W>
std::ostream &operator<<(std::ostream &out, const CliffordString<W> &v);

/// A string of single-qubit Clifford rotations.
template <size_t W>
struct CliffordString {
    size_t num_qubits;

    // The 2 sign bits of a single qubit Clifford, packed into arrays for easy processing.
    simd_bits<W> x_signs;
    simd_bits<W> z_signs;

    // The 4 tableau bits of a single qubit Clifford, packed into arrays for easy processing.
    // The x2x and z2z terms are inverted so that zero-initializing produces the identity gate.
    simd_bits<W> inv_x2x;
    simd_bits<W> x2z;
    simd_bits<W> z2x;
    simd_bits<W> inv_z2z;

    /// Constructs an identity CliffordString for the given number of qubits.
    explicit CliffordString(size_t num_qubits)
        : num_qubits(num_qubits),
          x_signs(num_qubits),
          z_signs(num_qubits),
          inv_x2x(num_qubits),
          x2z(num_qubits),
          z2x(num_qubits),
          inv_z2z(num_qubits) {
    }

    /// Extracts rotations k*W through (k+1)*W into a CliffordWord<W>.
    inline CliffordWord<bitword<W>> word_at(size_t k) const {
        return CliffordWord<bitword<W>>{
            x_signs.ptr_simd[k],
            z_signs.ptr_simd[k],
            inv_x2x.ptr_simd[k],
            x2z.ptr_simd[k],
            z2x.ptr_simd[k],
            inv_z2z.ptr_simd[k],
        };
    }
    /// Writes rotations k*W through (k+1)*W from a CliffordWord<W>.
    inline void set_word_at(size_t k, CliffordWord<bitword<W>> new_value) const {
        x_signs.ptr_simd[k] = new_value.x_signs;
        z_signs.ptr_simd[k] = new_value.z_signs;
        inv_x2x.ptr_simd[k] = new_value.inv_x2x;
        x2z.ptr_simd[k] = new_value.x2z;
        z2x.ptr_simd[k] = new_value.z2x;
        inv_z2z.ptr_simd[k] = new_value.inv_z2z;
    }

    /// Converts the internal rotation representation into a GateType.
    GateType gate_at(size_t q) const {
        return bits2gate(std::array<bool, 6>{z_signs[q], x_signs[q], inv_x2x[q], x2z[q], z2x[q], inv_z2z[q]});
    }

    /// Sets an internal rotation from a GateType.
    void set_gate_at(size_t q, GateType gate_type) {
        std::array<bool, 6> bits = gate_to_bits(gate_type);;
        z_signs[q] = bits[0];
        x_signs[q] = bits[1];
        inv_x2x[q] = bits[2];
        x2z[q] = bits[3];
        z2x[q] = bits[4];
        inv_z2z[q] = bits[5];
    }

    /// Inplace right-multiplication of rotations.
    CliffordString &operator*=(const CliffordString &rhs) {
        if (num_qubits < rhs.num_qubits) {
            throw std::invalid_argument("Can't inplace-multiply by a larger Clifford string.");
        }
        for (size_t k = 0; k < rhs.x_signs.num_simd_words; k++) {
            auto lhs_w = word_at(k);
            auto rhs_w = rhs.word_at(k);
            set_word_at(k, lhs_w * rhs_w);
        }
        return *this;
    }

    void inplace_then(CircuitInstruction inst) {
        std::array<bool, 6> v = gate_to_bits(inst.gate_type);
        for (const auto &t : inst.targets) {
            if (!t.is_qubit_target()) {
                continue;
            }
            uint32_t q = t.qubit_value();
            if (q >= num_qubits) {
                throw std::invalid_argument("Circuit acted on qubit past end of string.");
            }
            size_t w = q / W;
            size_t k = q % W;
            CliffordWord<bitword<W>> tmp{};
            bit_ref(&tmp.z_signs, k) ^= v[0];
            bit_ref(&tmp.x_signs, k) ^= v[1];
            bit_ref(&tmp.inv_x2x, k) ^= v[2];
            bit_ref(&tmp.x2z, k) ^= v[3];
            bit_ref(&tmp.z2x, k) ^= v[4];
            bit_ref(&tmp.inv_z2z, k) ^= v[5];
            set_word_at(w, tmp * word_at(w));
        }
    }

    static CliffordString<W> from_circuit(const Circuit &circuit) {
        CliffordString<W> result(circuit.count_qubits());
        circuit.for_each_operation([&](CircuitInstruction inst) {
            result.inplace_then(inst);
        });
        return result;
    }

    Circuit to_circuit() const {
        Circuit result;
        for (size_t q = 0; q < num_qubits; q++) {
            GateType g = gate_at(q);
            if (g != GateType::I || q + 1 == num_qubits) {
                GateTarget t = GateTarget::qubit(q);
                result.safe_append(CircuitInstruction{g, {}, &t, {}});
            }
        }
        return result;
    }

    /// Inplace left-multiplication of rotations.
    CliffordString &inplace_left_mul_by(const CliffordString &lhs) {
        if (num_qubits < lhs.num_qubits) {
            throw std::invalid_argument("Can't inplace-multiply by a larger Clifford string.");
        }
        for (size_t k = 0; k < x_signs.num_simd_words; k++) {
            auto lhs_w = lhs.word_at(k);
            auto rhs_w = word_at(k);
            set_word_at(k, lhs_w * rhs_w);
        }
        return *this;
    }

    /// Out-of-place multiplication of rotations.
    CliffordString operator*(const CliffordString &rhs) const {
        CliffordString<W> result = CliffordString<W>(std::max(num_qubits, rhs.num_qubits));
        size_t min_words = std::min(x_signs.num_simd_words, rhs.x_signs.num_simd_words);
        for (size_t k = 0; k < min_words; k++) {
            auto lhs_w = word_at(k);
            auto rhs_w = rhs.word_at(k);
            result.set_word_at(k, lhs_w * rhs_w);
        }

        // The longer string copies its tail into the result.
        size_t min_qubits = std::min(num_qubits, rhs.num_qubits);
        for (size_t q = min_qubits; q < num_qubits; q++) {
            result.x_signs[q] = x_signs[q];
            result.z_signs[q] = z_signs[q];
            result.inv_x2x[q] = inv_x2x[q];
            result.x2z[q] = x2z[q];
            result.z2x[q] = z2x[q];
            result.inv_z2z[q] = inv_z2z[q];
        }
        for (size_t q = min_qubits; q < rhs.num_qubits; q++) {
            result.x_signs[q] = rhs.x_signs[q];
            result.z_signs[q] = rhs.z_signs[q];
            result.inv_x2x[q] = rhs.inv_x2x[q];
            result.x2z[q] = rhs.x2z[q];
            result.z2x[q] = rhs.z2x[q];
            result.inv_z2z[q] = rhs.inv_z2z[q];
        }

        return result;
    }

    /// Determines if two Clifford strings have the same length and contents.
    bool operator==(const CliffordString<W> &other) const {
        return x_signs == other.x_signs
            && z_signs == other.z_signs
            && inv_x2x == other.inv_x2x
            && x2z == other.x2z
            && z2x == other.z2x
            && inv_z2z == other.inv_z2z;
    }
    /// Determines if two Clifford strings have different lengths or contents.
    bool operator!=(const CliffordString<W> &other) const {
        return !(*this == other);
    }

    /// Returns a description of the Clifford string.
    std::string str() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
};

template <size_t W>
std::ostream &operator<<(std::ostream &out, const CliffordString<W> &v) {
    for (size_t q = 0; q < v.num_qubits; q++) {
        if (q > 0) {
            out << " ";
        }
        int c = v.inv_x2x[q] + v.x2z[q] * 2 + v.z2x[q] * 4 + v.inv_z2z[q] * 8;
        int p = v.z_signs[q] + v.x_signs[q] * 2;
        out << "_?S?V??d??????uH"[c];
        out << "IXZY"[p];
    }
    return out;
}

}  // namespace stim

#endif
