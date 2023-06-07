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

constexpr uint8_t ARG_COUNT_SYGIL_ANY = uint8_t{0xFF};
constexpr uint8_t ARG_COUNT_SYGIL_ZERO_OR_ONE = uint8_t{0xFE};

constexpr inline uint8_t gate_name_to_hash(const char *v, size_t n) {
    // HACK: A collision is considered to be an error.
    // Just do *anything* that makes all the defined gates have different values.

    uint8_t result = 0;
    if (n > 0) {
        uint8_t c_first = v[0] | 0x20;
        uint8_t c_last = v[n - 1] | 0x20;
        c_last = (c_last << 1) | (c_last >> 7);
        result += c_first ^ c_last;
    }
    if (n > 2) {
        char c1 = (char)(v[1] | 0x20);
        char c2 = (char)(v[2] | 0x20);
        result ^= c1;
        result += c2 * 9;
    }
    if (n > 5) {
        char c3 = (char)(v[3] | 0x20);
        char c5 = (char)(v[5] | 0x20);
        result ^= c3 * 61;
        result += c5 * 223;
    }
    result &= 0x1F;
    result ^= n << 5;
    result ^= n >> 3;
    if (n > 6) {
        result += 157;
    }
    return result;
}

constexpr inline uint8_t gate_name_to_hash(const char *c) {
    return gate_name_to_hash(c, std::char_traits<char>::length(c));
}

enum GateType : uint8_t {
    // Annotations
    DETECTOR = gate_name_to_hash("DETECTOR"),
    OBSERVABLE_INCLUDE = gate_name_to_hash("OBSERVABLE_INCLUDE"),
    TICK = gate_name_to_hash("TICK"),
    QUBIT_COORDS = gate_name_to_hash("QUBIT_COORDS"),
    SHIFT_COORDS = gate_name_to_hash("SHIFT_COORDS"),
    // Control flow
    REPEAT = gate_name_to_hash("REPEAT"),
    // Collapsing gates
    MX = gate_name_to_hash("MX"),
    MY = gate_name_to_hash("MY"),
    M = gate_name_to_hash("M"),  // alias when parsing: MZ
    MRX = gate_name_to_hash("MRX"),
    MRY = gate_name_to_hash("MRY"),
    MR = gate_name_to_hash("MR"),  // alias when parsing: MRZ
    RX = gate_name_to_hash("RX"),
    RY = gate_name_to_hash("RY"),
    R = gate_name_to_hash("R"),  // alias when parsing: RZ
    MPP = gate_name_to_hash("MPP"),
    // Controlled gates
    XCX = gate_name_to_hash("XCX"),
    XCY = gate_name_to_hash("XCY"),
    XCZ = gate_name_to_hash("XCZ"),
    YCX = gate_name_to_hash("YCX"),
    YCY = gate_name_to_hash("YCY"),
    YCZ = gate_name_to_hash("YCZ"),
    CX = gate_name_to_hash("CX"),  // alias when parsing: CNOT, ZCX
    CY = gate_name_to_hash("CY"),  // alias when parsing: ZCY
    CZ = gate_name_to_hash("CZ"),  // alias when parsing: ZCZ
    // Hadamard-like gates
    H = gate_name_to_hash("H"),  // alias when parsing: H_XZ
    H_XY = gate_name_to_hash("H_XY"),
    H_YZ = gate_name_to_hash("H_YZ"),
    // Noise channels
    DEPOLARIZE1 = gate_name_to_hash("DEPOLARIZE1"),
    DEPOLARIZE2 = gate_name_to_hash("DEPOLARIZE2"),
    X_ERROR = gate_name_to_hash("X_ERROR"),
    Y_ERROR = gate_name_to_hash("Y_ERROR"),
    Z_ERROR = gate_name_to_hash("Z_ERROR"),
    PAULI_CHANNEL_1 = gate_name_to_hash("PAULI_CHANNEL_1"),
    PAULI_CHANNEL_2 = gate_name_to_hash("PAULI_CHANNEL_2"),
    E = gate_name_to_hash("E"),  // alias when parsing: CORRELATED_ERROR
    ELSE_CORRELATED_ERROR = gate_name_to_hash("ELSE_CORRELATED_ERROR"),
    // Pauli gates
    I = gate_name_to_hash("I"),
    X = gate_name_to_hash("X"),
    Y = gate_name_to_hash("Y"),
    Z = gate_name_to_hash("Z"),
    // Period 3 gates
    C_XYZ = gate_name_to_hash("C_XYZ"),
    C_ZYX = gate_name_to_hash("C_ZYX"),
    // Period 4 gates
    SQRT_X = gate_name_to_hash("SQRT_X"),
    SQRT_X_DAG = gate_name_to_hash("SQRT_X_DAG"),
    SQRT_Y = gate_name_to_hash("SQRT_Y"),
    SQRT_Y_DAG = gate_name_to_hash("SQRT_Y_DAG"),
    S = gate_name_to_hash("S"),          // alias when parsing: SQRT_Z
    S_DAG = gate_name_to_hash("S_DAG"),  // alias when parsing: SQRT_Z_DAG
    // Pauli product gates
    SQRT_XX = gate_name_to_hash("SQRT_XX"),
    SQRT_XX_DAG = gate_name_to_hash("SQRT_XX_DAG"),
    SQRT_YY = gate_name_to_hash("SQRT_YY"),
    SQRT_YY_DAG = gate_name_to_hash("SQRT_YY_DAG"),
    SQRT_ZZ = gate_name_to_hash("SQRT_ZZ"),
    SQRT_ZZ_DAG = gate_name_to_hash("SQRT_ZZ_DAG"),
    // Swap gates
    SWAP = gate_name_to_hash("SWAP"),
    ISWAP = gate_name_to_hash("ISWAP"),
    CXSWAP = gate_name_to_hash("CXSWAP"),
    SWAPCX = gate_name_to_hash("SWAPCX"),
    ISWAP_DAG = gate_name_to_hash("ISWAP_DAG"),
};

enum GateFlags : uint16_t {
    // All gates must have at least one flag set.
    NO_GATE_FLAG = 0,

    // Indicates whether unitary and tableau data is available for the gate, so it can be tested more easily.
    GATE_IS_UNITARY = 1 << 0,
    // Determines whether or not the gate is omitted when computing a reference sample.
    GATE_IS_NOISE = 1 << 1,
    // Controls validation of probability arguments like X_ERROR(0.01).
    GATE_ARGS_ARE_DISJOINT_PROBABILITIES = 1 << 2,
    // Indicates whether the gate puts data into the measurement record or not.
    // Also determines whether or not inverted targets (like "!3") are permitted.
    GATE_PRODUCES_NOISY_RESULTS = 1 << 3,
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
    /// A shorthand description of the Clifford tableau describing the gate. (Size 0 if the gate is not Clifford.)
    FixedCapVector<const char *, 4> tableau_data;
    /// Stim circuit file contents of a decomposition into H+S+CX+M+R operations. (nullptr if not decomposable.)
    const char *h_s_cx_m_r_decomposition;

    ExtraGateData(
        const char *category,
        const char *help,
        FixedCapVector<FixedCapVector<std::complex<float>, 4>, 4> unitary_data,
        FixedCapVector<const char *, 4> tableau_data,
        const char *h_s_cx_m_r_decomposition);
};

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

   public:
    std::array<Gate, 256> items;
    GateDataMap();

    std::vector<Gate> gates(bool include_aliases = false) const;

    inline const Gate &at(const char *text, size_t text_len) const {
        uint8_t h = gate_name_to_hash(text, text_len);
        const Gate &gate = items[h];
        if (_case_insensitive_mismatch(text, text_len, gate.name, gate.name_len)) {
            throw std::out_of_range("Gate not found: '" + std::string(text, text_len) + "'");
        }
        // Canonicalize.
        return items[static_cast<uint8_t>(gate.id)];
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
        uint8_t h = gate_name_to_hash(text.data(), text.size());
        const Gate &gate = items[h];
        return !_case_insensitive_mismatch(text.data(), text.size(), gate.name, gate.name_len);
    }
};

extern const GateDataMap GATE_DATA;

}  // namespace stim

#endif
