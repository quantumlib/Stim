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

#ifndef GATE_DATA_H
#define GATE_DATA_H

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

#include "../simd/fixed_cap_vector.h"

namespace stim_internal {

struct TableauSimulator;
struct FrameSimulator;
struct OperationData;
struct Tableau;
struct Operation;
struct ErrorFuser;

inline uint8_t gate_name_to_id(const char *v, size_t n) {
    // HACK: A collision is considered to be an error.
    // Just do *anything* that makes all the defined gates have different values.

    constexpr uint8_t c2_factor = 8;
    constexpr uint8_t clast_rotate = 1;

    uint8_t result = 0;
    if (n > 0) {
        uint8_t c_first = v[0] | 0x20;
        uint8_t c_last = v[n - 1] | 0x20;
        c_last = (c_last << clast_rotate) | (c_last >> (8 - clast_rotate));
        result += c_first ^ c_last;
    }
    if (n > 2) {
        result ^= 1;
        char c1 = (char)(v[1] | 0x20);
        char c2 = (char)(v[2] | 0x20);
        result += c1;
        result ^= c1;
        result += c2 * c2_factor;
        result ^= c2;
    }
    if (n > 5) {
        result ^= 5;
        char c3 = (char)(v[3] | 0x20);
        char c5 = (char)(v[5] | 0x20);
        result += c3 * 7;
        result += c5 * 11;
    }
    result &= 0x1F;
    result |= n << 5;
    result ^= n;
    return result;
}

inline uint8_t gate_name_to_id(const char *c) {
    return gate_name_to_id(c, strlen(c));
}

enum GateFlags : uint16_t {
    GATE_NO_FLAGS = 0,
    // Indicates whether unitary and tableau data is available for the gate, so it can be tested more easily.
    GATE_IS_UNITARY = 1 << 0,
    // Determines whether or not the gate is omitted when computing a reference sample.
    GATE_IS_NOISE = 1 << 1,
    // Controls parsing and validation of instructions like X_ERROR(0.01) taking an argument.
    GATE_TAKES_PARENS_ARGUMENT = 1 << 2,
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
    // Controls instructions like CX and SWAP operating on measurement record targets like "rec[-1]".
    GATE_CAN_TARGET_MEASUREMENT_RECORD = 1 << 9,
};

struct ExtraGateData {
    const char *category;
    const char *description;
    FixedCapVector<FixedCapVector<std::complex<float>, 4>, 4> unitary_data;
    FixedCapVector<const char *, 4> tableau_data;
};

struct Gate {
    const char *name;
    uint8_t name_len;
    void (TableauSimulator::*tableau_simulator_function)(const OperationData &);
    void (FrameSimulator::*frame_simulator_function)(const OperationData &);
    void (ErrorFuser::*reverse_error_fuser_function)(const OperationData &);
    GateFlags flags;
    ExtraGateData (*extra_data_func)(void);
    uint8_t id;

    Gate();
    Gate(
        const char *name, void (TableauSimulator::*tableau_simulator_function)(const OperationData &),
        void (FrameSimulator::*frame_simulator_function)(const OperationData &),
        void (ErrorFuser::*hit_simulator_function)(const OperationData &), GateFlags flags,
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
    std::array<Gate, 256> items;
    void add_gate(bool &failed, const Gate &data);
    void add_gate_alias(bool &failed, const char *alt_name, const char *canon_name);

   public:
    GateDataMap();

    std::vector<Gate> gates() const;

    inline const Gate &at(const char *text, size_t text_len) const {
        uint8_t h = gate_name_to_id(text, text_len);
        const Gate &gate = items[h];
        if (_case_insensitive_mismatch(text, text_len, gate.name, gate.name_len)) {
            throw std::out_of_range("Gate not found " + std::string(text, text_len));
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

}

#endif
