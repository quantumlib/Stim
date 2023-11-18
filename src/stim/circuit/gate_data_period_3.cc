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

#include <complex>

#include "stim/circuit/gate_data.h"

using namespace stim;

static constexpr std::complex<float> i = std::complex<float>(0, 1);

void GateDataMap::add_gate_data_period_3(bool &failed) {
    add_gate(
        failed,
        Gate{
            "C_XYZ",
            GateType::C_XYZ,
            GateType::C_ZYX,
            0,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            "B_Single Qubit Clifford Gates",
            R"MARKDOWN(
Right handed period 3 axis cycling gate, sending X -> Y -> Z -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            {{0.5f - i * 0.5f, -0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
            {"Y", "X"},
            R"CIRCUIT(
S 0
S 0
S 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            "C_ZYX",
            GateType::C_ZYX,
            GateType::C_XYZ,
            0,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            "B_Single Qubit Clifford Gates",
            R"MARKDOWN(
Left handed period 3 axis cycling gate, sending Z -> Y -> X -> Z.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            {{0.5f + i * 0.5f, 0.5f + 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
            {"Z", "Y"},
            R"CIRCUIT(
H 0
S 0
)CIRCUIT",
        });
}
