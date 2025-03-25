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

#ifndef _STIM_GATES_GATE_DATA_H
#define _STIM_GATES_GATE_DATA_H

#include <array>
#include <cassert>
#include <complex>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "stim/mem/fixed_cap_vector.h"

namespace stim {

template <size_t W>
struct Tableau;

template <size_t W>
struct Flow;

template <size_t W>
struct PauliString;

/// Used for gates' argument count to indicate that a gate takes a variable number of
/// arguments. This is relevant to coordinate data on detectors and qubits, where there may
/// be any number of coordinates.
constexpr uint8_t ARG_COUNT_SYGIL_ANY = uint8_t{0xFF};

/// Used for gates' argument count to indicate that a gate takes 0 parens arguments or 1
/// parens argument. This is relevant to measurement gates, where 0 parens arguments means
/// a noiseless result whereas 1 parens argument is a noisy result.
constexpr uint8_t ARG_COUNT_SYGIL_ZERO_OR_ONE = uint8_t{0xFE};

constexpr inline uint16_t gate_name_to_hash(std::string_view text) {
    // HACK: A collision is considered to be an error.
    // Just do *anything* that makes all the defined gates have different values.

    constexpr uint16_t const1 = 2126;
    constexpr uint16_t const2 = 9883;
    constexpr uint16_t const3 = 8039;
    constexpr uint16_t const4 = 9042;
    constexpr uint16_t const5 = 4916;
    constexpr uint16_t const6 = 4048;
    constexpr uint16_t const7 = 7081;

    size_t n = text.size();
    const char *v = text.data();
    size_t result = n;
    if (n > 0) {
        auto c_first = v[0] | 0x20;
        auto c_last = v[n - 1] | 0x20;
        result ^= c_first * const1;
        result += c_last * const2;
    }
    if (n > 2) {
        auto c1 = v[1] | 0x20;
        auto c2 = v[2] | 0x20;
        result ^= c1 * const3;
        result += c2 * const4;
    }
    if (n > 4) {
        auto c3 = v[3] | 0x20;
        auto c4 = v[4] | 0x20;
        result ^= c3 * const5;
        result += c4 * const6;
    }
    if (n > 5) {
        auto c5 = v[5] | 0x20;
        result ^= c5 * const7;
    }
    return result & 0x1FF;
}

constexpr size_t NUM_DEFINED_GATES = 82;

enum class GateType : uint8_t {
    NOT_A_GATE = 0,
    // Annotations
    DETECTOR,
    OBSERVABLE_INCLUDE,
    TICK,
    QUBIT_COORDS,
    SHIFT_COORDS,
    // Control flow
    REPEAT,
    // Collapsing gates
    MPAD,
    MX,
    MY,
    M,  // alias when parsing: MZ
    MRX,
    MRY,
    MR,  // alias when parsing: MRZ
    RX,
    RY,
    R,  // alias when parsing: RZ
    // Controlled gates
    XCX,
    XCY,
    XCZ,
    YCX,
    YCY,
    YCZ,
    CX,  // alias when parsing: CNOT, ZCX
    CY,  // alias when parsing: ZCY
    CZ,  // alias when parsing: ZCZ
    // Hadamard-like gates
    H,  // alias when parsing: H_XZ
    H_XY,
    H_YZ,
    H_NXY,
    H_NXZ,
    H_NYZ,
    // Noise channels
    DEPOLARIZE1,
    DEPOLARIZE2,
    X_ERROR,
    Y_ERROR,
    Z_ERROR,
    I_ERROR,
    II_ERROR,
    PAULI_CHANNEL_1,
    PAULI_CHANNEL_2,
    E,  // alias when parsing: CORRELATED_ERROR
    ELSE_CORRELATED_ERROR,
    // Heralded noise channels
    HERALDED_ERASE,
    HERALDED_PAULI_CHANNEL_1,
    // Pauli gates
    I,
    X,
    Y,
    Z,
    // Period 3 gates
    C_XYZ,
    C_ZYX,
    C_NXYZ,
    C_XNYZ,
    C_XYNZ,
    C_NZYX,
    C_ZNYX,
    C_ZYNX,
    // Period 4 gates
    SQRT_X,
    SQRT_X_DAG,
    SQRT_Y,
    SQRT_Y_DAG,
    S,      // alias when parsing: SQRT_Z
    S_DAG,  // alias when parsing: SQRT_Z_DAG
    // Parity phasing gates.
    II,
    SQRT_XX,
    SQRT_XX_DAG,
    SQRT_YY,
    SQRT_YY_DAG,
    SQRT_ZZ,
    SQRT_ZZ_DAG,
    // Pauli product gates
    MPP,
    SPP,
    SPP_DAG,
    // Swap gates
    SWAP,
    ISWAP,
    CXSWAP,
    SWAPCX,
    CZSWAP,
    ISWAP_DAG,
    // Pair measurement gates
    MXX,
    MYY,
    MZZ,
};

enum GateFlags : uint16_t {
    // All gates must have at least one flag set.
    NO_GATE_FLAG = 0,

    // Indicates whether unitary and tableau data is available for the gate, so it can be tested more easily.
    GATE_IS_UNITARY = 1 << 0,
    // Determines whether or not the gate is omitted when computing a reference sample.
    GATE_IS_NOISY = 1 << 1,
    // Controls validation of probability arguments like X_ERROR(0.01).
    GATE_ARGS_ARE_DISJOINT_PROBABILITIES = 1 << 2,
    // Indicates whether the gate puts data into the measurement record or not.
    // Also determines whether or not inverted targets (like "!3") are permitted.
    GATE_PRODUCES_RESULTS = 1 << 3,
    // Prevents the same gate on adjacent lines from being combined into one longer invocation.
    GATE_IS_NOT_FUSABLE = 1 << 4,
    // Controls block functionality for instructions like REPEAT.
    GATE_IS_BLOCK = 1 << 5,
    // Controls validation code checking for arguments coming in pairs.
    GATE_TARGETS_PAIRS = 1 << 6,
    // Controls instructions like CORRELATED_ERROR taking Pauli product targets ("X1 Y2 Z3").
    // Note that this enables the Pauli terms but not the combine terms like X1*Y2.
    GATE_TARGETS_PAULI_STRING = 1 << 7,
    // Controls instructions like DETECTOR taking measurement record targets ("rec[-1]").
    // The "ONLY" refers to the fact that this flag switches the default behavior to not allowing qubit targets.
    // Further flags can then override that default.
    GATE_ONLY_TARGETS_MEASUREMENT_RECORD = 1 << 8,
    // Controls instructions like CX operating allowing measurement record targets and sweep bit targets.
    GATE_CAN_TARGET_BITS = 1 << 9,
    // Controls whether the gate takes qubit/record targets.
    GATE_TAKES_NO_TARGETS = 1 << 10,
    // Controls validation of index arguments like OBSERVABLE_INCLUDE(1).
    GATE_ARGS_ARE_UNSIGNED_INTEGERS = 1 << 11,
    // Controls instructions like MPP taking Pauli product combiners ("X1*Y2 Z3").
    GATE_TARGETS_COMBINERS = 1 << 12,
    // Measurements and resets are dissipative operations.
    GATE_IS_RESET = 1 << 13,
    // Annotations like DETECTOR aren't strictly speaking identity operations, but they can be ignored by code that only
    // cares about effects that happen to qubits (as opposed to in the classical control system).
    GATE_HAS_NO_EFFECT_ON_QUBITS = 1 << 14,
    // Whether or not the gate trivially broadcasts over targets.
    GATE_IS_SINGLE_QUBIT_GATE = 1 << 15,
};

struct Gate {
    /// The canonical name of the gate, used when printing it to a circuit file.
    std::string_view name;
    /// The gate's type, such as stim::GateType::X or stim::GateType::MRZ.
    GateType id;
    /// The id of the gate inverse to this one, or at least the closest thing to an inverse.
    /// Set to GateType::NOT_A_GATE for gates with no inverse.
    GateType best_candidate_inverse_id;
    /// The number of parens arguments the gate expects (e.g. X_ERROR takes 1, PAULI_CHANNEL_1 takes 3).
    /// Set to stim::ARG_COUNT_SYGIL_ANY to indicate any number is allowed (e.g. DETECTOR coordinate data).
    uint8_t arg_count;
    /// Bit-packed data describing details of the gate.
    GateFlags flags;

    /// A word describing what sort of gate this is.
    std::string_view category;
    /// Prose summary of what the gate is, how it fits into Stim, and how to use it.
    std::string_view help;
    /// A unitary matrix describing the gate. (Size 0 if the gate is not unitary.)
    FixedCapVector<FixedCapVector<std::complex<float>, 4>, 4> unitary_data;
    /// A shorthand description of the stabilizer flows of the gate.
    /// For single qubit Cliffords, this should be the output stabilizers for X then Z.
    /// For 2 qubit Cliffords, this should be the output stabilizers for X_, Z_, _X, _Z.
    /// For 2 qubit dissipative gates, this should be flows like "X_ -> XX xor rec[-1]".
    FixedCapVector<const char *, 10> flow_data;
    /// Stim circuit file contents of a decomposition into H+S+CX+M+R operations. (nullptr if not decomposable.)
    const char *h_s_cx_m_r_decomposition;

    inline bool operator==(const Gate &other) const {
        return id == other.id;
    }
    inline bool operator!=(const Gate &other) const {
        return id != other.id;
    }

    const Gate &inverse() const;

    template <size_t W>
    Tableau<W> tableau() const {
        if (!(flags & GateFlags::GATE_IS_UNITARY)) {
            throw std::invalid_argument(std::string(name) + " isn't unitary so it doesn't have a tableau.");
        }
        const auto &d = flow_data;
        if (flow_data.size() == 2) {
            return Tableau<W>::gate1(d[0], d[1]);
        }
        if (flow_data.size() == 4) {
            return Tableau<W>::gate2(d[0], d[1], d[2], d[3]);
        }
        throw std::out_of_range(std::string(name) + " doesn't have 1q or 2q tableau data.");
    }

    template <size_t W>
    std::vector<Flow<W>> flows() const {
        if (has_known_unitary_matrix()) {
            auto t = tableau<W>();
            if (flags & GateFlags::GATE_TARGETS_PAIRS) {
                return {
                    Flow<W>{stim::PauliString<W>::from_str("X_"), t.xs[0], {}},
                    Flow<W>{stim::PauliString<W>::from_str("Z_"), t.zs[0], {}},
                    Flow<W>{stim::PauliString<W>::from_str("_X"), t.xs[1], {}},
                    Flow<W>{stim::PauliString<W>::from_str("_Z"), t.zs[1], {}},
                };
            }
            return {
                Flow<W>{stim::PauliString<W>::from_str("X"), t.xs[0], {}},
                Flow<W>{stim::PauliString<W>::from_str("Z"), t.zs[0], {}},
            };
        }
        std::vector<Flow<W>> out;
        for (const auto &c : flow_data) {
            out.push_back(Flow<W>::from_str(c));
        }
        return out;
    }

    std::vector<std::vector<std::complex<float>>> unitary() const;

    bool is_symmetric() const;
    GateType hadamard_conjugated(bool ignoring_sign) const;

    /// Determines if the gate has a specified unitary matrix.
    ///
    /// Some unitary gates, such as SPP, don't have a specified matrix because the
    /// matrix depends crucially on the targets.
    bool has_known_unitary_matrix() const;

    /// Converts a single qubit unitary gate into an euler-angles rotation.
    ///
    /// Returns:
    ///     {theta, phi, lambda} using the same convention as qiskit.
    ///     Each angle is in radians.
    ///     For stabilizer operations, every angle will be a multiple of pi/2.
    ///
    ///     The unitary matrix U of the operation can be recovered (up to global phase)
    ///     by computing U = RotZ(phi) * RotY(theta) * RotZ(lambda).
    std::array<float, 3> to_euler_angles() const;

    /// Converts a single qubit unitary gate into an axis-angle rotation.
    ///
    /// Returns:
    ///     An array {x, y, z, a}.
    ///     <x, y, z> is a unit vector indicating the axis of rotation.
    ///     <a> is the angle of rotation in radians.
    std::array<float, 4> to_axis_angle() const;
};

inline bool _case_insensitive_mismatch(std::string_view text1, std::string_view text2) {
    if (text1.size() != text2.size()) {
        return true;
    }
    bool failed = false;
    for (size_t k = 0; k < text1.size(); k++) {
        failed |= toupper(text1[k]) != text2[k];
    }
    return failed;
}

struct GateDataMapHashEntry {
    GateType id = GateType::NOT_A_GATE;
    std::string_view expected_name;
};

struct GateDataMap {
   private:
    void add_gate(bool &failed, const Gate &data);
    void add_gate_alias(bool &failed, const char *alt_name, const char *canon_name);
    void add_gate_data_annotations(bool &failed);
    void add_gate_data_blocks(bool &failed);
    void add_gate_data_heralded(bool &failed);
    void add_gate_data_collapsing(bool &failed);
    void add_gate_data_controlled(bool &failed);
    void add_gate_data_hada(bool &failed);
    void add_gate_data_noisy(bool &failed);
    void add_gate_data_pauli(bool &failed);
    void add_gate_data_period_3(bool &failed);
    void add_gate_data_period_4(bool &failed);
    void add_gate_data_pp(bool &failed);
    void add_gate_data_swaps(bool &failed);
    void add_gate_data_pair_measure(bool &failed);
    void add_gate_data_pauli_product(bool &failed);

   public:
    std::array<GateDataMapHashEntry, 512> hashed_name_to_gate_type_table;
    std::array<Gate, NUM_DEFINED_GATES> items;
    GateDataMap();

    inline const Gate &operator[](GateType g) const {
        return items[(uint64_t)g];
    }

    inline const Gate &at(std::string_view text) const {
        auto h = gate_name_to_hash(text);
        const auto &entry = hashed_name_to_gate_type_table[h];
        if (_case_insensitive_mismatch(text, entry.expected_name)) {
            throw std::out_of_range("Gate not found: '" + std::string(text) + "'");
        }
        // Canonicalize.
        return (*this)[entry.id];
    }

    inline bool has(std::string_view text) const {
        auto h = gate_name_to_hash(text);
        const auto &entry = hashed_name_to_gate_type_table[h];
        return !_case_insensitive_mismatch(text, entry.expected_name);
    }
};

extern const GateDataMap GATE_DATA;

}  // namespace stim

#endif
