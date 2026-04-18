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

#include "stim/circuit/circuit.h"
#include "stim/gates/gates.h"
#include "stim/mem/simd_bits.h"

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

constexpr std::array<GateType, 64> INT_TO_SINGLE_QUBIT_CLIFFORD_TABLE{
    GateType::I,          GateType::X,          GateType::Z,          GateType::Y,

    GateType::NOT_A_GATE,  // These should be impossible if the class is in a good state.
    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::S,          GateType::H_XY,       GateType::S_DAG,      GateType::H_NXY,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::SQRT_X_DAG, GateType::SQRT_X,     GateType::H_YZ,       GateType::H_NYZ,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::C_ZYX,      GateType::C_ZNYX,     GateType::C_ZYNX,     GateType::C_NZYX,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE, GateType::NOT_A_GATE,

    GateType::C_XYZ,      GateType::C_XYNZ,     GateType::C_XNYZ,     GateType::C_NXYZ,

    GateType::H,          GateType::SQRT_Y_DAG, GateType::SQRT_Y,     GateType::H_NXZ,
};

inline GateType bits2gate(std::array<bool, 6> bits) {
    int k = (bits[0] << 0) | (bits[1] << 1) | (bits[2] << 2) | (bits[3] << 3) | (bits[4] << 4) | (bits[5] << 5);
    return INT_TO_SINGLE_QUBIT_CLIFFORD_TABLE[k];
}

inline std::array<bool, 6> gate_to_bits(GateType gate_type) {
    Gate g = GATE_DATA[gate_type];
    if (!(g.flags & GATE_IS_SINGLE_QUBIT_GATE) || !(g.flags & GATE_IS_UNITARY)) {
        throw std::invalid_argument("Not a single qubit Clifford gate: " + std::string(g.name));
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
    result.x_signs = rhs.x_signs ^ andnot(rhs.inv_x2x, lhs.x_signs) ^ (rhs_x2y & dy) ^ (rhs.x2z & lhs.z_signs);
    result.z_signs = rhs.z_signs ^ (rhs.z2x & lhs.x_signs) ^ (rhs_z2y & dy) ^ andnot(rhs.inv_z2z, lhs.z_signs);

    return result;
}

template <size_t W>
struct CliffordString;

template <size_t W>
std::ostream &operator<<(std::ostream &out, const CliffordString<W> &v);

template <size_t W>
GateType single_qubit_tableau_to_gate_type(const stim::Tableau<W> &tableau) {
    return bits2gate(std::array<bool, 6>{
        tableau.zs.signs[0],
        tableau.xs.signs[0],
        !tableau.xs.xt[0][0],
        tableau.xs.zt[0][0],
        tableau.zs.xt[0][0],
        !tableau.zs.zt[0][0],
    });
}

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

    void randomize(std::mt19937_64 &rng) {
        CliffordString<W> result(num_qubits);
        x_signs.randomize(num_qubits, rng);
        z_signs.randomize(num_qubits, rng);
        for (size_t k = 0; k < num_qubits; k++) {
            uint64_t v = rng() % 6;
            uint8_t p1 = v % 3 + 1;
            uint8_t p2 = v / 3 + 1;
            p2 += p2 >= p1;
            inv_x2x[k] = !(p1 & 1);
            x2z[k] = p1 & 2;
            z2x[k] = p2 & 1;
            inv_z2z[k] = !(p2 & 2);
        }
    }

    static CliffordString<W> random(size_t num_qubits, std::mt19937_64 &rng) {
        CliffordString<W> result(num_qubits);
        result.randomize(rng);
        return result;
    }

    CliffordString<W> operator+(const CliffordString<W> &other) const {
        if (num_qubits + other.num_qubits < num_qubits) {
            throw std::invalid_argument("Couldn't concatenate Clifford strings due to size overflowing.");
        }
        CliffordString<W> result(num_qubits + other.num_qubits);
        for (size_t k = 0; k < num_qubits; k++) {
            result.set_gate_at(k, gate_at(k));
        }
        for (size_t k = 0; k < other.num_qubits; k++) {
            result.set_gate_at(num_qubits + k, other.gate_at(k));
        }
        return result;
    }

    CliffordString<W> &operator+=(const CliffordString<W> &other) {
        CliffordString<W> tmp = *this + other;
        *this = std::move(tmp);
        return *this;
    }

    CliffordString<W> operator*(size_t repetitions) const {
        if (repetitions == 0) {
            return CliffordString<W>(0);
        }

        size_t new_num_qubits = num_qubits * repetitions;
        if (new_num_qubits / repetitions != num_qubits) {
            throw std::invalid_argument("Couldn't repeat CliffordString due to size overflowing.");
        }

        CliffordString<W> result(new_num_qubits);
        for (size_t k = 0; k < num_qubits; k++) {
            GateType g = gate_at(k);
            for (size_t k2 = k; k2 < new_num_qubits; k2 += num_qubits) {
                result.set_gate_at(k2, g);
            }
        }
        return result;
    }

    CliffordString<W> &operator*=(size_t repetitions) {
        CliffordString<W> tmp = *this * repetitions;
        *this = std::move(tmp);
        return *this;
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
        std::array<bool, 6> bits = gate_to_bits(gate_type);
        z_signs[q] = bits[0];
        x_signs[q] = bits[1];
        inv_x2x[q] = bits[2];
        x2z[q] = bits[3];
        z2x[q] = bits[4];
        inv_z2z[q] = bits[5];
    }

    /// Inplace right-multiplication of rotations.
    CliffordString &operator*=(const CliffordString &rhs) {
        ensure_num_qubits(rhs.num_qubits);
        for (size_t k = 0; k < rhs.x_signs.num_simd_words; k++) {
            auto lhs_w = word_at(k);
            auto rhs_w = rhs.word_at(k);
            set_word_at(k, lhs_w * rhs_w);
        }
        return *this;
    }

    void inplace_then(CircuitInstruction inst) {
        // Ignore annotations.
        switch (inst.gate_type) {
            case GateType::TICK:
            case GateType::QUBIT_COORDS:
            case GateType::SHIFT_COORDS:
            case GateType::DETECTOR:
            case GateType::OBSERVABLE_INCLUDE:
                return;
            default:
                break;
        }

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

    void ensure_num_qubits(size_t min_num_qubits) {
        if (num_qubits < min_num_qubits) {
            num_qubits = min_num_qubits;
            x_signs.preserving_resize(num_qubits);
            z_signs.preserving_resize(num_qubits);
            inv_x2x.preserving_resize(num_qubits);
            x2z.preserving_resize(num_qubits);
            z2x.preserving_resize(num_qubits);
            inv_z2z.preserving_resize(num_qubits);
        }
    }

    /// Inplace left-multiplication of rotations.
    CliffordString &inplace_left_mul_by(const CliffordString &lhs) {
        ensure_num_qubits(lhs.num_qubits);
        for (size_t k = 0; k < x_signs.num_simd_words; k++) {
            auto lhs_w = lhs.word_at(k);
            auto rhs_w = word_at(k);
            set_word_at(k, lhs_w * rhs_w);
        }
        return *this;
    }

    /// Inplace raises to a power.
    void ipow(int64_t power) const {
        power %= 12;
        if (power < 0) {
            power += 12;
        }
        for (size_t k = 0; k < x_signs.num_simd_words; k++) {
            auto delta = word_at(k);
            CliffordWord<bitword<W>> total{};
            for (size_t step = 0; step < (size_t)power; step++) {
                total = total * delta;
            }
            set_word_at(k, total);
        }
    }

    PauliString<W> x_outputs() const {
        PauliString<W> result(num_qubits);
        result.xs = inv_x2x;
        result.zs = x2z;
        result.xs.invert_bits();
        result.xs.clear_bits_past(num_qubits);
        return result;
    }
    PauliString<W> y_outputs_and_signs(simd_bits_range_ref<W> y_signs_out) const {
        PauliString<W> result(num_qubits);
        result.xs = inv_x2x;
        result.zs = x2z;
        y_signs_out = x_signs;
        y_signs_out ^= z_signs;
        simd_bits_range_ref<W>(result.xs).for_each_word(
            result.zs,
            z2x,
            inv_z2z,
            y_signs_out,
            [](simd_word<W> &x_out,
               simd_word<W> &z_out,
               const simd_word<W> &x2,
               const simd_word<W> &inv_z2,
               simd_word<W> &y_sign) {
                y_sign ^= ~x_out & ~inv_z2 & (z_out ^ x2);
                y_sign ^= x_out & inv_z2 & z_out & x2;
                x_out ^= ~x2;
                z_out ^= ~inv_z2;
            });
        result.xs.clear_bits_past(num_qubits);
        result.zs.clear_bits_past(num_qubits);
        return result;
    }
    PauliString<W> z_outputs() const {
        PauliString<W> result(num_qubits);
        result.xs = z2x;
        result.zs = inv_z2z;
        result.zs.invert_bits();
        result.zs.clear_bits_past(num_qubits);
        return result;
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
        return x_signs == other.x_signs && z_signs == other.z_signs && inv_x2x == other.inv_x2x && x2z == other.x2z &&
               z2x == other.z2x && inv_z2z == other.inv_z2z;
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

    std::string py_str() const {
        std::stringstream ss;
        for (size_t q = 0; q < num_qubits; q++) {
            if (q) {
                ss << ',';
            }
            ss << GATE_DATA[gate_at(q)].name;
        }
        return ss.str();
    }

    CliffordString<W> py_get_slice(int64_t start, int64_t step, int64_t slice_length) const {
        assert(slice_length >= 0);
        assert(slice_length == 0 || start >= 0);
        CliffordString<W> result(slice_length);
        for (size_t k = 0; k < (size_t)slice_length; k++) {
            size_t old_k = start + step * k;
            result.x_signs[k] = x_signs[old_k];
            result.z_signs[k] = z_signs[old_k];
            result.inv_x2x[k] = inv_x2x[old_k];
            result.x2z[k] = x2z[old_k];
            result.z2x[k] = z2x[old_k];
            result.inv_z2z[k] = inv_z2z[old_k];
        }
        return result;
    }

    std::string py_repr() const {
        std::stringstream ss;
        ss << "stim.CliffordString(\"";
        for (size_t q = 0; q < num_qubits; q++) {
            if (q) {
                ss << ',';
            }
            ss << GATE_DATA[gate_at(q)].name;
        }
        ss << "\")";
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
