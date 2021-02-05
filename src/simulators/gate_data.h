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

#include <cassert>
#include <complex>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct TableauSimulator;
struct FrameSimulator;
struct OperationData;
struct Tableau;
struct Operation;

inline uint8_t gate_name_to_id(const char *v, size_t n) {
    // HACK: A collision is considered to be an error.
    // Just do *anything* that makes all the defined gates have different values.

    uint8_t result = v[0] + (v[n - 1] << 2);
    if (n >= 3) {
        result ^= 1;
        result += v[1];
        result ^= v[1];
        result += v[2] * 5;
        result ^= v[2];
    }
    if (n >= 5) {
        result ^= 5;
        result += v[3] * 7;
        result += v[5] * 11;
    }
    result &= 0x1F;
    result |= n << 5;
    result ^= n;
    return result;
}

inline uint8_t gate_name_to_id(const char *c) {
    return gate_name_to_id(c, strlen(c));
}

template <typename T, size_t max_length>
struct TruncatedArray {
    size_t length;
    T data[max_length];
    TruncatedArray() : length(0), data() {
    }
    TruncatedArray(T item) : length(1), data() {
        data[0] = std::move(item);
    }
    TruncatedArray(std::initializer_list<T> values) : length(values.size()), data() {
        assert(values.size() <= max_length);
        size_t k = 0;
        for (auto &v : values) {
            data[k++] = std::move(v);
        }
    }
};

enum GateFlags : uint8_t {
    GATE_NO_FLAGS = 0,
    GATE_IS_UNITARY = 1 << 0,
    GATE_IS_NOISE = 1 << 1,
    GATE_TARGETS_MEASUREMENT_RECORD = 1 << 2,
    GATE_TAKES_PARENS_ARGUMENT = 1 << 3,
    GATE_PRODUCES_RESULTS = 1 << 4,
    GATE_IS_NOT_FUSABLE = 1 << 5,
};

struct Gate {
    const char *name;
    void (TableauSimulator::*tableau_simulator_function)(const OperationData &);
    void (FrameSimulator::*frame_simulator_function)(const OperationData &);
    GateFlags flags;
    TruncatedArray<TruncatedArray<std::complex<float>, 4>, 4> unitary_data;
    TruncatedArray<const char *, 4> tableau_data;
    uint8_t id;

    Gate();
    Gate(
        const char *name, void (TableauSimulator::*tableau_simulator_function)(const OperationData &),
        void (FrameSimulator::*frame_simulator_function)(const OperationData &), GateFlags flags,
        TruncatedArray<TruncatedArray<std::complex<float>, 4>, 4> unitary_data,
        TruncatedArray<const char *, 4> tableau_data);

    const Gate &inverse() const;
    Tableau tableau() const;
    std::vector<std::vector<std::complex<float>>> unitary() const;
    Operation applied(OperationData data) const;
};

struct GateDataMap {
private:
    Gate items[256];

public:
    GateDataMap(
        std::initializer_list<Gate> gates,
        std::initializer_list<std::pair<const char *, const char *>> alternate_names);

    std::vector<Gate> gates() const;

    inline const Gate &at(const char *text, size_t text_len) const {
        uint8_t h = gate_name_to_id(text);
        const char *bucket_name = items[h].name;
        if (bucket_name == nullptr || strcmp(items[h].name, text) != 0) {
            throw std::out_of_range("Gate not found " + std::string(text));
        }
        // Canonicalize.
        return items[items[h].id];
    }

    inline const Gate &at(const char *text) const {
        return at(text, strlen(text));
    }

    inline const Gate &at(const std::string &text) const {
        return at(text.data(), text.size());
    }

    inline bool has(const std::string &text) const {
        uint8_t h = gate_name_to_id(text.data(), text.size());
        return items[h].name != nullptr && strcmp(items[h].name, text.data()) == 0;
    }
};

extern const GateDataMap GATE_DATA;

#endif
