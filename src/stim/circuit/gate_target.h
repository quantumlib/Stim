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

#ifndef _STIM_CIRCUIT_GATE_TARGET_H
#define _STIM_CIRCUIT_GATE_TARGET_H

#include <iostream>

#include "stim/gates/gates.h"
#include "stim/mem/span_ref.h"

namespace stim {

constexpr uint32_t TARGET_VALUE_MASK = (uint32_t{1} << 24) - uint32_t{1};
constexpr uint32_t TARGET_INVERTED_BIT = uint32_t{1} << 31;
constexpr uint32_t TARGET_PAULI_X_BIT = uint32_t{1} << 30;
constexpr uint32_t TARGET_PAULI_Z_BIT = uint32_t{1} << 29;
constexpr uint32_t TARGET_RECORD_BIT = uint32_t{1} << 28;
constexpr uint32_t TARGET_COMBINER = uint32_t{1} << 27;
constexpr uint32_t TARGET_SWEEP_BIT = uint32_t{1} << 26;

struct GateTarget {
    uint32_t data;
    int32_t value() const;

    static GateTarget x(uint32_t qubit, bool inverted = false);
    static GateTarget y(uint32_t qubit, bool inverted = false);
    static GateTarget z(uint32_t qubit, bool inverted = false);
    static GateTarget pauli_xz(uint32_t qubit, bool x, bool z, bool inverted = false);
    static GateTarget qubit(uint32_t qubit, bool inverted = false);
    static GateTarget rec(int32_t lookback);
    static GateTarget sweep_bit(uint32_t index);
    static GateTarget combiner();
    static GateTarget from_target_str(std::string_view text);

    GateTarget operator!() const;
    int32_t rec_offset() const;
    bool has_qubit_value() const;
    bool is_combiner() const;
    bool is_x_target() const;
    bool is_y_target() const;
    bool is_z_target() const;
    bool is_inverted_result_target() const;
    bool is_measurement_record_target() const;
    bool is_qubit_target() const;
    bool is_sweep_bit_target() const;
    bool is_classical_bit_target() const;
    bool is_pauli_target() const;
    uint32_t qubit_value() const;
    bool operator==(const GateTarget &other) const;
    bool operator!=(const GateTarget &other) const;
    bool operator<(const GateTarget &other) const;
    std::string str() const;
    std::string repr() const;
    char pauli_type() const;
    std::string target_str() const;

    void write_succinct(std::ostream &out) const;
};

template <typename SOURCE>
uint32_t read_uint24_t(int &c, SOURCE read_char) {
    if (!(c >= '0' && c <= '9')) {
        throw std::invalid_argument("Expected a digit but got '" + std::string(1, c) + "'");
    }
    uint32_t result = 0;
    do {
        result *= 10;
        result += c - '0';
        if (result >= uint32_t{1} << 24) {
            throw std::invalid_argument("Number too large.");
        }
        c = read_char();
    } while (c >= '0' && c <= '9');
    return result;
}

template <typename SOURCE>
inline GateTarget read_raw_qubit_target(int &c, SOURCE read_char) {
    return GateTarget::qubit(read_uint24_t(c, read_char));
}

template <typename SOURCE>
inline GateTarget read_measurement_record_target(int &c, SOURCE read_char) {
    if (c != 'r' || read_char() != 'e' || read_char() != 'c' || read_char() != '[' || read_char() != '-') {
        throw std::invalid_argument("Target started with 'r' but wasn't a record argument like 'rec[-1]'.");
    }
    c = read_char();
    uint32_t lookback = read_uint24_t(c, read_char);
    if (c != ']') {
        throw std::invalid_argument("Target started with 'r' but wasn't a record argument like 'rec[-1]'.");
    }
    c = read_char();
    return GateTarget{lookback | TARGET_RECORD_BIT};
}

template <typename SOURCE>
inline GateTarget read_sweep_bit_target(int &c, SOURCE read_char) {
    if (c != 's' || read_char() != 'w' || read_char() != 'e' || read_char() != 'e' || read_char() != 'p' ||
        read_char() != '[') {
        throw std::invalid_argument("Target started with 's' but wasn't a sweep bit argument like 'sweep[5]'.");
    }
    c = read_char();
    uint32_t lookback = read_uint24_t(c, read_char);
    if (c != ']') {
        throw std::invalid_argument("Target started with 's' but wasn't a sweep bit argument like 'sweep[5]'.");
    }
    c = read_char();
    return GateTarget{lookback | TARGET_SWEEP_BIT};
}

template <typename SOURCE>
inline GateTarget read_single_gate_target(int &c, SOURCE read_char) {
    switch (c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return read_raw_qubit_target(c, read_char);
        case 'r':
            return read_measurement_record_target(c, read_char);
        case '!':
            return read_inverted_target(c, read_char);
        case 'X':
        case 'Y':
        case 'Z':
        case 'x':
        case 'y':
        case 'z':
            return read_pauli_target(c, read_char);
        case '*':
            c = read_char();
            return GateTarget::combiner();
        case 's':
            return read_sweep_bit_target(c, read_char);
        default:
            throw std::invalid_argument("Unrecognized target prefix '" + std::string(1, c) + "'.");
    }
}

template <typename SOURCE>
inline GateTarget read_pauli_target(int &c, SOURCE read_char) {
    uint32_t m = 0;
    if (c == 'x' || c == 'X') {
        m = TARGET_PAULI_X_BIT;
    } else if (c == 'y' || c == 'Y') {
        m = TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT;
    } else if (c == 'z' || c == 'Z') {
        m = TARGET_PAULI_Z_BIT;
    } else {
        assert(false);
    }
    c = read_char();
    if (c == ' ') {
        throw std::invalid_argument(
            "Pauli target '" + std::string(1, c) + "' followed by a space instead of a qubit index.");
    }
    uint32_t q = read_uint24_t(c, read_char);

    return {q | m};
}

template <typename SOURCE>
inline GateTarget read_inverted_target(int &c, SOURCE read_char) {
    assert(c == '!');
    c = read_char();
    GateTarget t;
    if (c == 'X' || c == 'x' || c == 'Y' || c == 'y' || c == 'Z' || c == 'z') {
        t = read_pauli_target(c, read_char);
    } else {
        t = read_raw_qubit_target(c, read_char);
    }
    t.data ^= TARGET_INVERTED_BIT;
    return t;
}

void write_targets(std::ostream &out, SpanRef<const GateTarget> targets);
std::string targets_str(SpanRef<const GateTarget> targets);

std::ostream &operator<<(std::ostream &out, const GateTarget &t);

}  // namespace stim

#endif
