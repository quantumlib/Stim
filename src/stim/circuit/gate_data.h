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

struct TableauSimulator;
struct SparseUnsignedRevFrameTracker;
struct FrameSimulator;
struct OperationData;
struct Tableau;
struct Operation;
struct ErrorAnalyzer;

constexpr uint8_t ARG_COUNT_SYGIL_ANY = uint8_t{0xFF};
constexpr uint8_t ARG_COUNT_SYGIL_ZERO_OR_ONE = uint8_t{0xFE};

enum class Gates : uint8_t {
    // From gate_data_annotations.cc
    DETECTOR, OBSERVABLE_INCLUDE, TICK, QUBIT_COORDS, SHIFT_COORDS,
    // Frome gate_data_blocks.cc
    REPEAT,
    // From gate_data_collapsing.cc
    MX, MY, M, MRX, MRY, MR, RX, RY, R, MPP,
    // From gate_data_controlled.cc
    XCX, XCY, XCZ, YCX, YCY, YCZ, CX, CY, CZ,
    // From gate_data_hada.cc
    H, H_XY, H_YZ,
    // From gate_data_noisy.cc
    DEPOLARIZE1, DEPOLARIZE2, X_ERROR, Y_ERROR, Z_ERROR,
    PAULI_CHANNEL_1, PAULI_CHANNEL_2,
    E, ELSE_CORRELATED_ERROR,
    // From gate_data_pauli.cc
    I, X, Y, Z,
    // From gate_data_period_3.cc
    C_XYZ, C_ZYX,
    // From gate_data_period_4.cc
    SQRT_X, SQRT_X_DAG, SQRT_Y, SQRT_Y_DAG, S, S_DAG,
    // From gate_data_pp.cc
    SQRT_XX, SQRT_XX_DAG, SQRT_YY, SQRT_YY_DAG, SQRT_ZZ, SQRT_ZZ_DAG,
    // From gate_data_swaps.cc
    SWAP, ISWAP, CXSWAP, SWAPCX, ISWAP_DAG,

    /// GATE ALIASES

    // From gate_data_collapsing.cc
    MZ, MRZ, RZ, ZCX, CNOT, ZCY, ZCZ,
    // From gate_data_noisy.cc
    CORRELATED_ERROR,
    // From gate_data_hada.cc
    H_XZ,
    // From gate_data_period_4.cc
    SQRT_Z, SQRT_Z_DAG,
};


inline uint8_t gate_name_to_hash(const char *v, size_t n) {
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

inline uint8_t gate_hash_to_id(uint8_t hash) noexcept {
    switch (hash) {
        case 1:
            return static_cast<int>(Gates::DEPOLARIZE2);
        case 13:
            return static_cast<int>(Gates::SQRT_YY_DAG);
        case 14:
            return static_cast<int>(Gates::SQRT_ZZ_DAG);
        case 16:
            return static_cast<int>(Gates::SQRT_XX_DAG);
        case 27:
            return static_cast<int>(Gates::DEPOLARIZE1);
        case 29:
            return static_cast<int>(Gates::SHIFT_COORDS);
        case 40:
            return static_cast<int>(Gates::X);
        case 43:
            return static_cast<int>(Gates::Y);
        case 46:
            return static_cast<int>(Gates::Z);
        case 47:
            return static_cast<int>(Gates::E);
        case 48:
            return static_cast<int>(Gates::QUBIT_COORDS);
        case 53:
            return static_cast<int>(Gates::S);
        case 54:
            return static_cast<int>(Gates::R);
        case 55:
            return static_cast<int>(Gates::M);
        case 56:
            return static_cast<int>(Gates::H);
        case 59:
            return static_cast<int>(Gates::I);
        case 64:
            return static_cast<int>(Gates::RY);
        case 65:
            return static_cast<int>(Gates::ELSE_CORRELATED_ERROR);
        case 66:
            return static_cast<int>(Gates::RX);
        case 70:
            return static_cast<int>(Gates::RZ);
        case 73:
            return static_cast<int>(Gates::MR);
        case 81:
            return static_cast<int>(Gates::CY);
        case 83:
            return static_cast<int>(Gates::CX);
        case 87:
            return static_cast<int>(Gates::CZ);
        case 89:
            return static_cast<int>(Gates::MZ);
        case 93:
            return static_cast<int>(Gates::MX);
        case 95:
            return static_cast<int>(Gates::MY);
        case 97:
            return static_cast<int>(Gates::ZCX);
        case 98:
            return static_cast<int>(Gates::YCX);
        case 99:
            return static_cast<int>(Gates::XCX);
        case 103:
            return static_cast<int>(Gates::MRX);
        case 105:
            return static_cast<int>(Gates::YCY);
        case 106:
            return static_cast<int>(Gates::XCY);
        case 108:
            return static_cast<int>(Gates::ZCY);
        case 109:
            return static_cast<int>(Gates::MPP);
        case 110:
            return static_cast<int>(Gates::MRY);
        case 117:
            return static_cast<int>(Gates::MRZ);
        case 119:
            return static_cast<int>(Gates::ZCZ);
        case 120:
            return static_cast<int>(Gates::YCZ);
        case 121:
            return static_cast<int>(Gates::XCZ);
        case 127:
            return static_cast<int>(Gates::SQRT_ZZ);
        case 132:
            return static_cast<int>(Gates::H_YZ);
        case 134:
            return static_cast<int>(Gates::TICK);
        case 136:
            return static_cast<int>(Gates::X_ERROR);
        case 137:
            return static_cast<int>(Gates::PAULI_CHANNEL_1);
        case 139:
            return static_cast<int>(Gates::PAULI_CHANNEL_2);
        case 140:
            return static_cast<int>(Gates::CNOT);
        case 141:
            return static_cast<int>(Gates::SWAP);
        case 146:
            return static_cast<int>(Gates::Z_ERROR);
        case 147:
            return static_cast<int>(Gates::Y_ERROR);
        case 149:
            return static_cast<int>(Gates::SQRT_XX);
        case 154:
            return static_cast<int>(Gates::SQRT_YY);
        case 155:
            return static_cast<int>(Gates::H_XZ);
        case 157:
            return static_cast<int>(Gates::H_XY);
        case 160:
            return static_cast<int>(Gates::C_XYZ);
        case 166:
            return static_cast<int>(Gates::S_DAG);
        case 169:
            return static_cast<int>(Gates::ISWAP);
        case 178:
            return static_cast<int>(Gates::DETECTOR);
        case 179:
            return static_cast<int>(Gates::CORRELATED_ERROR);
        case 182:
            return static_cast<int>(Gates::C_ZYX);
        case 194:
            return static_cast<int>(Gates::SQRT_Z);
        case 202:
            return static_cast<int>(Gates::REPEAT);
        case 205:
            return static_cast<int>(Gates::CXSWAP);
        case 213:
            return static_cast<int>(Gates::SWAPCX);
        case 216:
            return static_cast<int>(Gates::SQRT_X);
        case 219:
            return static_cast<int>(Gates::ISWAP_DAG);
        case 221:
            return static_cast<int>(Gates::SQRT_Y);
        case 236:
            return static_cast<int>(Gates::OBSERVABLE_INCLUDE);
        case 237:
            return static_cast<int>(Gates::SQRT_Y_DAG);
        case 238:
            return static_cast<int>(Gates::SQRT_Z_DAG);
        case 240:
            return static_cast<int>(Gates::SQRT_X_DAG);
        default:
            std::cerr << "Gate hash not mapped to Gate ID\n";
            return 0;
    }
}

inline uint8_t gate_name_to_id(const char *c, size_t n) {
    return gate_hash_to_id(gate_name_to_hash(c, n));
}

inline uint8_t gate_name_to_id(const char *c) {
    return gate_hash_to_id(gate_name_to_hash(c, strlen(c)));
}

enum GateFlags : uint16_t {
    GATE_NO_FLAGS = 0,
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
    void (TableauSimulator::*tableau_simulator_function)(const OperationData &);
    void (FrameSimulator::*frame_simulator_function)(const OperationData &);
    void (ErrorAnalyzer::*reverse_error_analyzer_function)(const OperationData &);
    void (SparseUnsignedRevFrameTracker::*sparse_unsigned_rev_frame_tracker_function)(const OperationData &);
    ExtraGateData (*extra_data_func)(void);
    GateFlags flags;
    uint8_t arg_count;
    uint8_t name_len;
    uint8_t id;
    uint8_t best_candidate_inverse_id;

    Gate();
    Gate(
        const char *name,
        const char *best_inverse_name,
        uint8_t arg_count,
        void (TableauSimulator::*tableau_simulator_function)(const OperationData &),
        void (FrameSimulator::*frame_simulator_function)(const OperationData &),
        void (ErrorAnalyzer::*hit_simulator_function)(const OperationData &),
        void (SparseUnsignedRevFrameTracker::*sparse_unsigned_rev_frame_tracker_function)(const OperationData &),
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

    std::vector<Gate> gates() const;

    inline const Gate &at(const char *text, size_t text_len) const {
        uint8_t h = gate_name_to_id(text, text_len);
        const Gate &gate = items[h];
        if (_case_insensitive_mismatch(text, text_len, gate.name, gate.name_len)) {
            throw std::out_of_range("Gate not found: '" + std::string(text, text_len) + "'");
        }
        // Canonicalize.
        return items[gate.id];
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
        uint8_t h = gate_name_to_id(text.data(), text.size());
        const Gate &gate = items[h];
        return !_case_insensitive_mismatch(text.data(), text.size(), gate.name, gate.name_len);
    }
};

extern const GateDataMap GATE_DATA;

void decompose_mpp_operation(
    const OperationData &target_data,
    size_t num_qubits,
    const std::function<void(
        const OperationData &h_xz, const OperationData &h_yz, const OperationData &cnot, const OperationData &meas)>
        &callback);

}  // namespace stim

#endif
