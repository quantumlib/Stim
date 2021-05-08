// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gate_data.h"

#include <complex>

#include "../simulators/error_fuser.h"
#include "../simulators/frame_simulator.h"
#include "../simulators/tableau_simulator.h"

using namespace stim_internal;

static constexpr std::complex<float> i = std::complex<float>(0, 1);
static constexpr std::complex<float> s = 0.7071067811865475244f;

extern const GateDataMap stim_internal::GATE_DATA(
    {
        // Collapsing gates.
        {
            "MX",
            &TableauSimulator::measure_x,
            &FrameSimulator::measure_x,
            &ErrorFuser::MX,
            GATE_PRODUCES_RESULTS,
            {},
            {},
        },
        {
            "MY",
            &TableauSimulator::measure_y,
            &FrameSimulator::measure_y,
            &ErrorFuser::MY,
            GATE_PRODUCES_RESULTS,
            {},
            {},
        },
        {
            "M",
            &TableauSimulator::measure_z,
            &FrameSimulator::measure_z,
            &ErrorFuser::MZ,
            GATE_PRODUCES_RESULTS,
            {},
            {},
        },
        {
            "MRX",
            &TableauSimulator::measure_reset_x,
            &FrameSimulator::measure_reset_x,
            &ErrorFuser::MRX,
            GATE_PRODUCES_RESULTS,
            {},
            {},
        },
        {
            "MRY",
            &TableauSimulator::measure_reset_y,
            &FrameSimulator::measure_reset_y,
            &ErrorFuser::MRY,
            GATE_PRODUCES_RESULTS,
            {},
            {},
        },
        {
            "MR",
            &TableauSimulator::measure_reset_z,
            &FrameSimulator::measure_reset_z,
            &ErrorFuser::MRZ,
            GATE_PRODUCES_RESULTS,
            {},
            {},
        },
        {
            "RX",
            &TableauSimulator::reset_x,
            &FrameSimulator::reset_x,
            &ErrorFuser::RX,
            GATE_NO_FLAGS,
            {},
            {},
        },
        {
            "RY",
            &TableauSimulator::reset_y,
            &FrameSimulator::reset_y,
            &ErrorFuser::RY,
            GATE_NO_FLAGS,
            {},
            {},
        },
        {
            "R",
            &TableauSimulator::reset_z,
            &FrameSimulator::reset_z,
            &ErrorFuser::RZ,
            GATE_NO_FLAGS,
            {},
            {},
        },

        // Pauli gates.
        {
            "I",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_UNITARY,
            {{1, 0}, {0, 1}},
            {"+X", "+Z"},
        },
        {
            "X",
            &TableauSimulator::X,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_UNITARY,
            {{0, 1}, {1, 0}},
            {"+X", "-Z"},
        },
        {
            "Y",
            &TableauSimulator::Y,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_UNITARY,
            {{0, -i}, {i, 0}},
            {"-X", "-Z"},
        },
        {
            "Z",
            &TableauSimulator::Z,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_UNITARY,
            {{1, 0}, {0, -1}},
            {"-X", "+Z"},
        },

        // Axis exchange gates.
        {
            "H_XY",
            &TableauSimulator::H_XY,
            &FrameSimulator::H_XY,
            &ErrorFuser::H_XY,
            GATE_IS_UNITARY,
            {{0, s - i *s}, {s + i * s, 0}},
            {"+Y", "-Z"},
        },
        {
            "H",
            &TableauSimulator::H_XZ,
            &FrameSimulator::H_XZ,
            &ErrorFuser::H_XZ,
            GATE_IS_UNITARY,
            {{s, s}, {s, -s}},
            {"+Z", "+X"},
        },
        {
            "H_YZ",
            &TableauSimulator::H_YZ,
            &FrameSimulator::H_YZ,
            &ErrorFuser::H_YZ,
            GATE_IS_UNITARY,
            {{s, -i *s}, {i * s, -s}},
            {"-X", "+Y"},
        },

        // 90 degree rotation gates.
        {
            "SQRT_X",
            &TableauSimulator::SQRT_X,
            &FrameSimulator::H_YZ,
            &ErrorFuser::H_YZ,
            GATE_IS_UNITARY,
            {{0.5f + 0.5f * i, 0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
            {"+X", "-Y"},
        },
        {
            "SQRT_X_DAG",
            &TableauSimulator::SQRT_X_DAG,
            &FrameSimulator::H_YZ,
            &ErrorFuser::H_YZ,
            GATE_IS_UNITARY,
            {{0.5f - 0.5f * i, 0.5f + 0.5f * i}, {0.5f + 0.5f * i, 0.5f - 0.5f * i}},
            {"+X", "+Y"},
        },
        {
            "SQRT_Y",
            &TableauSimulator::SQRT_Y,
            &FrameSimulator::H_XZ,
            &ErrorFuser::H_XZ,
            GATE_IS_UNITARY,
            {{0.5f + 0.5f * i, -0.5f - 0.5f * i}, {0.5f + 0.5f * i, 0.5f + 0.5f * i}},
            {"-Z", "+X"},
        },
        {
            "SQRT_Y_DAG",
            &TableauSimulator::SQRT_Y_DAG,
            &FrameSimulator::H_XZ,
            &ErrorFuser::H_XZ,
            GATE_IS_UNITARY,
            {{0.5f - 0.5f * i, 0.5f - 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
            {"+Z", "-X"},
        },
        {
            "S",
            &TableauSimulator::SQRT_Z,
            &FrameSimulator::H_XY,
            &ErrorFuser::H_XY,
            GATE_IS_UNITARY,
            {{1, 0}, {0, i}},
            {"+Y", "+Z"},
        },
        {
            "S_DAG",
            &TableauSimulator::SQRT_Z_DAG,
            &FrameSimulator::H_XY,
            &ErrorFuser::H_XY,
            GATE_IS_UNITARY,
            {{1, 0}, {0, -i}},
            {"-Y", "+Z"},
        },

        // Swap gates.
        {
            "SWAP",
            &TableauSimulator::SWAP,
            &FrameSimulator::SWAP,
            &ErrorFuser::SWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
            {"+IX", "+IZ", "+XI", "+ZI"},
        },
        {
            "ISWAP",
            &TableauSimulator::ISWAP,
            &FrameSimulator::ISWAP,
            &ErrorFuser::ISWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            {{1, 0, 0, 0}, {0, 0, i, 0}, {0, i, 0, 0}, {0, 0, 0, 1}},
            {"+ZY", "+IZ", "+YZ", "+ZI"},
        },
        {
            "ISWAP_DAG",
            &TableauSimulator::ISWAP_DAG,
            &FrameSimulator::ISWAP,
            &ErrorFuser::ISWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            {{1, 0, 0, 0}, {0, 0, -i, 0}, {0, -i, 0, 0}, {0, 0, 0, 1}},
            {"-ZY", "+IZ", "-YZ", "+ZI"},
        },

        // Axis interaction gates.
        {
            "XCX",
            &TableauSimulator::XCX,
            &FrameSimulator::XCX,
            &ErrorFuser::XCX,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            {{0.5f, 0.5f, 0.5f, -0.5f},
             {0.5f, 0.5f, -0.5f, 0.5f},
             {0.5f, -0.5f, 0.5f, 0.5f},
             {-0.5f, 0.5f, 0.5f, 0.5f}},
            {"+XI", "+ZX", "+IX", "+XZ"},
        },
        {
            "XCY",
            &TableauSimulator::XCY,
            &FrameSimulator::XCY,
            &ErrorFuser::XCY,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            {{0.5f, 0.5f, -0.5f * i, 0.5f * i},
             {0.5f, 0.5f, 0.5f * i, -0.5f * i},
             {0.5f * i, -0.5f * i, 0.5f, 0.5f},
             {-0.5f * i, 0.5f * i, 0.5f, 0.5f}},
            {"+XI", "+ZY", "+XX", "+XZ"},
        },
        {
            "XCZ",
            &TableauSimulator::XCZ,
            &FrameSimulator::XCZ,
            &ErrorFuser::XCZ,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}},
            {"+XI", "+ZZ", "+XX", "+IZ"},
        },
        {
            "YCX",
            &TableauSimulator::YCX,
            &FrameSimulator::YCX,
            &ErrorFuser::YCX,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            {{0.5f, -i * 0.5f, 0.5f, i * 0.5f},
             {i * 0.5f, 0.5f, -i * 0.5f, 0.5f},
             {0.5f, i * 0.5f, 0.5f, -i * 0.5f},
             {-i * 0.5f, 0.5f, i * 0.5f, 0.5f}},
            {"+XX", "+ZX", "+IX", "+YZ"},
        },
        {
            "YCY",
            &TableauSimulator::YCY,
            &FrameSimulator::YCY,
            &ErrorFuser::YCY,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            {{0.5f, -i * 0.5f, -i * 0.5f, 0.5f},
             {i * 0.5f, 0.5f, -0.5f, -i * 0.5f},
             {i * 0.5f, -0.5f, 0.5f, -i * 0.5f},
             {0.5f, i * 0.5f, i * 0.5f, 0.5f}},
            {"+XY", "+ZY", "+YX", "+YZ"},
        },
        {
            "YCZ",
            &TableauSimulator::YCZ,
            &FrameSimulator::YCZ,
            &ErrorFuser::YCZ,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, -i}, {0, 0, i, 0}},
            {"+XZ", "+ZZ", "+YX", "+IZ"},
        },
        {
            "CX",
            &TableauSimulator::ZCX,
            &FrameSimulator::ZCX,
            &ErrorFuser::ZCX,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}},
            {"+XX", "+ZI", "+IX", "+ZZ"},
        },
        {
            "CY",
            &TableauSimulator::ZCY,
            &FrameSimulator::ZCY,
            &ErrorFuser::ZCY,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            {{1, 0, 0, 0}, {0, 0, 0, -i}, {0, 0, 1, 0}, {0, i, 0, 0}},
            {"+XY", "+ZI", "+ZX", "+ZZ"},
        },
        {
            "CZ",
            &TableauSimulator::ZCZ,
            &FrameSimulator::ZCZ,
            &ErrorFuser::ZCZ,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, -1}},
            {"+XZ", "+ZI", "+ZX", "+IZ"},
        },

        // Noise gates.
        {
            "DEPOLARIZE1",
            &TableauSimulator::DEPOLARIZE1,
            &FrameSimulator::DEPOLARIZE1,
            &ErrorFuser::DEPOLARIZE1,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT),
            {},
            {},
        },
        {
            "DEPOLARIZE2",
            &TableauSimulator::DEPOLARIZE2,
            &FrameSimulator::DEPOLARIZE2,
            &ErrorFuser::DEPOLARIZE2,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT | GATE_TARGETS_PAIRS),
            {},
            {},
        },
        {
            "X_ERROR",
            &TableauSimulator::X_ERROR,
            &FrameSimulator::X_ERROR,
            &ErrorFuser::X_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT),
            {},
            {},
        },
        {
            "Y_ERROR",
            &TableauSimulator::Y_ERROR,
            &FrameSimulator::Y_ERROR,
            &ErrorFuser::Y_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT),
            {},
            {},
        },
        {
            "Z_ERROR",
            &TableauSimulator::Z_ERROR,
            &FrameSimulator::Z_ERROR,
            &ErrorFuser::Z_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT),
            {},
            {},
        },

        // Annotation gates.
        {
            "DETECTOR",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::DETECTOR,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_IS_NOT_FUSABLE),
            {},
            {},
        },
        {
            "OBSERVABLE_INCLUDE",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::OBSERVABLE_INCLUDE,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_TAKES_PARENS_ARGUMENT | GATE_IS_NOT_FUSABLE),
            {},
            {},
        },
        {
            "TICK",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_NOT_FUSABLE,
            {},
            {},
        },
        {
            "REPEAT",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            (GateFlags)(GATE_IS_BLOCK | GATE_IS_NOT_FUSABLE),
            {},
            {},
        },
        {
            "E",
            &TableauSimulator::CORRELATED_ERROR,
            &FrameSimulator::CORRELATED_ERROR,
            &ErrorFuser::CORRELATED_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT | GATE_TARGETS_PAULI_STRING | GATE_IS_NOT_FUSABLE),
            {},
            {},
        },
        {
            "ELSE_CORRELATED_ERROR",
            &TableauSimulator::ELSE_CORRELATED_ERROR,
            &FrameSimulator::ELSE_CORRELATED_ERROR,
            &ErrorFuser::ELSE_CORRELATED_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT | GATE_TARGETS_PAULI_STRING | GATE_IS_NOT_FUSABLE),
            {},
            {},
        },
    },
    {
        {"H_XZ", "H"},
        {"CORRELATED_ERROR", "E"},
        {"SQRT_Z", "S"},
        {"SQRT_Z_DAG", "S_DAG"},
        {"ZCZ", "CZ"},
        {"ZCY", "CY"},
        {"ZCX", "CX"},
        {"CNOT", "CX"},
        {"MZ", "M"},
        {"RZ", "R"},
        {"MRZ", "MR"},
    });

Tableau Gate::tableau() const {
    const auto &d = tableau_data.data;
    if (tableau_data.size() == 2) {
        return Tableau::gate1(d[0], d[1]);
    }
    if (tableau_data.size() == 4) {
        return Tableau::gate2(d[0], d[1], d[2], d[3]);
    }
    throw std::out_of_range(std::string(name) + " doesn't have 1q or 2q tableau data.");
}

std::vector<std::vector<std::complex<float>>> Gate::unitary() const {
    if (unitary_data.size() != 2 && unitary_data.size() != 4) {
        throw std::out_of_range(std::string(name) + " doesn't have 1q or 2q unitary data.");
    }
    std::vector<std::vector<std::complex<float>>> result;
    for (size_t k = 0; k < unitary_data.size(); k++) {
        const auto &d = unitary_data.data[k];
        result.emplace_back();
        for (size_t j = 0; j < d.size(); j++) {
            result.back().push_back(d.data[j]);
        }
    }
    return result;
}

const Gate &Gate::inverse() const {
    std::string inv_name = name;
    if (!(flags & GATE_IS_UNITARY)) {
        throw std::out_of_range(inv_name + " has no inverse.");
    }

    if (GATE_DATA.has(inv_name + "_DAG")) {
        inv_name += "_DAG";
    } else if (inv_name.size() > 4 && inv_name.substr(inv_name.size() - 4) == "_DAG") {
        inv_name = inv_name.substr(0, inv_name.size() - 4);
    } else {
        // Self inverse.
    }
    return GATE_DATA.at(inv_name);
}

Gate::Gate() : name(nullptr) {
}

Gate::Gate(
    const char *name, void (TableauSimulator::*tableau_simulator_function)(const OperationData &),
    void (FrameSimulator::*frame_simulator_function)(const OperationData &),
    void (ErrorFuser::*hit_simulator_function)(const OperationData &), GateFlags flags,
    TruncatedArray<TruncatedArray<std::complex<float>, 4>, 4> unitary_data,
    TruncatedArray<const char *, 4> tableau_data)
    : name(name),
      name_len(strlen(name)),
      tableau_simulator_function(tableau_simulator_function),
      frame_simulator_function(frame_simulator_function),
      reverse_error_fuser_function(hit_simulator_function),
      flags(flags),
      unitary_data(unitary_data),
      tableau_data(tableau_data),
      id(gate_name_to_id(name)) {
}

GateDataMap::GateDataMap(
    std::initializer_list<Gate> gates, std::initializer_list<std::pair<const char *, const char *>> alternate_names) {

    bool collision = false;
    for (const auto &gate : gates) {
        const char *c = gate.name;
        uint8_t h = gate_name_to_id(c);
        Gate &g = items[h];
        if (g.name != nullptr) {
            std::cerr << "GATE COLLISION " << gate.name << " vs " << g.name << "\n";
            collision = true;
        }
        g = gate;
    }
    for (const auto &alt : alternate_names) {
        const auto *alt_name = alt.first;
        const auto *canon_name = alt.second;
        uint8_t h_alt = gate_name_to_id(alt_name);
        Gate &g_alt = items[h_alt];
        if (g_alt.name != nullptr) {
            std::cerr << "GATE COLLISION " << alt_name << " vs " << g_alt.name << "\n";
            collision = true;
        }

        uint8_t h_canon = gate_name_to_id(canon_name);
        Gate &g_canon = items[h_canon];
        assert(g_canon.name != nullptr && g_canon.id == h_canon);
        g_alt.name = alt_name;
        g_alt.name_len = strlen(alt_name);
        g_alt.id = h_canon;
    }
    if (collision) {
        exit(EXIT_FAILURE);
    }
}

std::vector<Gate> GateDataMap::gates() const {
    std::vector<Gate> result;
    for (const auto &item : items) {
        if (item.name != nullptr) {
            result.push_back(item);
        }
    }
    return result;
}
