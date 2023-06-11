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

#ifndef _STIM_CIRCUIT_GATE_DATA_H
#define _STIM_CIRCUIT_GATE_DATA_H

#include <array>
#include <cassert>
#include <complex>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "stim/mem/fixed_cap_vector.h"

namespace stim {

struct Tableau;

/// Used for gates' argument count to indicate that a gate takes a variable number of
/// arguments. This is relevant to coordinate data on detectors and qubits, where there may
/// be any number of coordinates.
constexpr uint8_t ARG_COUNT_SYGIL_ANY = uint8_t{0xFF};

/// Used for gates' argument count to indicate that a gate takes 0 parens arguments or 1
/// parens argument. This is relevant to measurement gates, where 0 parens arguments means
/// a noiseless result whereas 1 parens argument is a noisy result.
constexpr uint8_t ARG_COUNT_SYGIL_ZERO_OR_ONE = uint8_t{0xFE};

constexpr inline uint16_t gate_name_to_hash(const char *v, size_t n) {
    // HACK: A collision is considered to be an error.
    // Just do *anything* that makes all the defined gates have different values.

    size_t result = n;
    if (n > 0) {
        auto c_first = v[0] | 0x20;
        auto c_last = v[n - 1] | 0x20;
        result += c_first ^ (c_last << 1);
    }
    if (n > 2) {
        auto c1 = v[1] | 0x20;
        auto c2 = v[2] | 0x20;
        result ^= c1;
        result += c2 * 11;
    }
    if (n > 5) {
        auto c3 = v[3] | 0x20;
        auto c5 = v[5] | 0x20;
        result ^= c3 * 61;
        result += c5 * 27;
    }
    return result & 0x1FF;
}

constexpr inline uint16_t gate_name_to_hash(const char *c) {
    return gate_name_to_hash(c, std::char_traits<char>::length(c));
}

constexpr const size_t NUM_DEFINED_GATES = 65;

enum GateType : uint8_t {
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
    MPP,
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
    // Noise channels
    DEPOLARIZE1,
    DEPOLARIZE2,
    X_ERROR,
    Y_ERROR,
    Z_ERROR,
    PAULI_CHANNEL_1,
    PAULI_CHANNEL_2,
    E,  // alias when parsing: CORRELATED_ERROR
    ELSE_CORRELATED_ERROR,
    // Pauli gates
    I,
    X,
    Y,
    Z,
    // Period 3 gates
    C_XYZ,
    C_ZYX,
    // Period 4 gates
    SQRT_X,
    SQRT_X_DAG,
    SQRT_Y,
    SQRT_Y_DAG,
    S,  // alias when parsing: SQRT_Z
    S_DAG,  // alias when parsing: SQRT_Z_DAG
    // Pauli product gates
    SQRT_XX,
    SQRT_XX_DAG,
    SQRT_YY,
    SQRT_YY_DAG,
    SQRT_ZZ,
    SQRT_ZZ_DAG,
    // Swap gates
    SWAP,
    ISWAP,
    CXSWAP,
    SWAPCX,
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
    GATE_TARGETS_PAULI_STRING = 1 << 7,
    // Controls instructions like DETECTOR taking measurement record targets ("rec[-1]").
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
};

struct ExtraGateData {
    /// A word describing what sort of gate this is.
    const char *category;
    /// Prose summary of what the gate is, how it fits into Stim, and how to use it.
    const char *help;
    /// A unitary matrix describing the gate. (Size 0 if the gate is not unitary.)
    FixedCapVector<FixedCapVector<std::complex<float>, 4>, 4> unitary_data;
    /// A shorthand description of the stabilizer flows of the gate.
    /// For single qubit Cliffords, this should be the output stabilizers for X then Z.
    /// For 2 qubit Cliffords, this should be the output stabilizers for X_, Z_, _X, _Z.
    /// For 2 qubit dissipative gates, this should be flows like "X_ -> XX xor rec[-1]".
    FixedCapVector<const char *, 4> flow_data;
    /// Stim circuit file contents of a decomposition into H+S+CX+M+R operations. (nullptr if not decomposable.)
    const char *h_s_cx_m_r_decomposition;

    ExtraGateData(
        const char *category,
        const char *help,
        FixedCapVector<FixedCapVector<std::complex<float>, 4>, 4> unitary_data,
        FixedCapVector<const char *, 4> tableau_data,
        const char *h_s_cx_m_r_decomposition);
};

struct StabilizerFlow;

struct Gate {
    const char *name;
    ExtraGateData (*extra_data_func)(void);
    GateFlags flags;
    uint8_t arg_count;
    uint8_t name_len;
    GateType id;
    GateType best_candidate_inverse_id;

    Gate();
    Gate(
        const char *name,
        GateType gate_id,
        GateType best_inverse_gate,
        uint8_t arg_count,
        GateFlags flags,
        ExtraGateData (*extra_data_func)(void));

    const Gate &inverse() const;
    Tableau tableau() const;
    std::vector<StabilizerFlow> flows() const;
    std::vector<std::vector<std::complex<float>>> unitary() const;
};

struct StringView {
    const char *c;
    size_t n;

    StringView(const char *c, size_t n) : c(c), n(n) {
    }

    StringView(std::string &v) : c(&v[0]), n(v.size()) {
    }

    inline StringView substr(size_t offset) const {
        return {c + offset, n - offset};
    }

    inline StringView substr(size_t offset, size_t length) const {
        return {c + offset, length};
    }

    inline StringView &operator=(const std::string &other) {
        c = (char *)&other[0];
        n = other.size();
        return *this;
    }

    inline const char &operator[](size_t index) const {
        return c[index];
    }

    inline bool operator==(const std::string &other) const {
        return n == other.size() && memcmp(c, other.data(), n) == 0;
    }

    inline bool operator!=(const std::string &other) const {
        return !(*this == other);
    }

    inline bool operator==(const char *other) const {
        size_t k = 0;
        for (; k < n; k++) {
            if (other[k] != c[k]) {
                return false;
            }
        }
        return other[k] == '\0';
    }

    inline bool operator!=(const char *other) const {
        return !(*this == other);
    }

    inline std::string str() const {
        return std::string(c, n);
    }
};

inline std::string operator+(const StringView &a, const char *b) {
    return a.str() + b;
}

inline std::string operator+(const char *a, const StringView &b) {
    return a + b.str();
}

inline std::string operator+(const StringView &a, const std::string &b) {
    return a.str() + b;
}

inline std::string operator+(const std::string &a, const StringView &b) {
    return a + b.str();
}

inline bool _case_insensitive_mismatch(const char *text, size_t text_len, const char *bucket_name, uint8_t bucket_len) {
    if (bucket_name == nullptr || bucket_len != text_len) {
        return true;
    }
    bool failed = false;
    for (size_t k = 0; k < text_len; k++) {
        failed |= toupper(text[k]) != bucket_name[k];
    }
    return failed;
}

struct GateDataMapHashEntry {
    GateType id;
    const char *expected_name;
    size_t expected_name_len;
};

struct GateDataMap {
   private:
    void add_gate(bool &failed, const Gate &data);
    void add_gate_alias(bool &failed, const char *alt_name, const char *canon_name);
    void add_gate_data_annotations(bool &failed);
    void add_gate_data_blocks(bool &failed);
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

   public:
    std::array<GateDataMapHashEntry, 512> hashed_name_to_gate_type_table;
    std::array<Gate, NUM_DEFINED_GATES> items;
    GateDataMap();

    inline const Gate &at(const char *text, size_t text_len) const {
        auto h = gate_name_to_hash(text, text_len);
        const auto &entry = hashed_name_to_gate_type_table[h];
        if (_case_insensitive_mismatch(text, text_len, entry.expected_name, entry.expected_name_len)) {
            throw std::out_of_range("Gate not found: '" + std::string(text, text_len) + "'");
        }
        // Canonicalize.
        return items[entry.id];
    }

    inline const Gate &at(const char *text) const {
        return at(text, strlen(text));
    }

    inline const Gate &at(StringView text) const {
        return at(text.c, text.n);
    }

    inline const Gate &at(const std::string &text) const {
        return at(text.data(), text.size());
    }

    inline bool has(const std::string &text) const {
        auto h = gate_name_to_hash(text.data(), text.size());
        const auto &entry = hashed_name_to_gate_type_table[h];
        return !_case_insensitive_mismatch(text.data(), text.size(), entry.expected_name, entry.expected_name_len);
    }
};

extern const GateDataMap GATE_DATA;

}  // namespace stim

#endif
