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

void GateDataMap::add_gate_data_period_4(bool &failed) {
    add_gate(
        failed,
        Gate{
            "SQRT_X",
            GateType::SQRT_X,
            GateType::SQRT_X_DAG,
            0,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principal square root of X gate.
Phases the amplitude of |-> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0.5f + 0.5f * i, 0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
                    {"+X", "-Y"},
                    R"CIRCUIT(
H 0
S 0
H 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_X_DAG",
            GateType::SQRT_X_DAG,
            GateType::SQRT_X,
            0,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Adjoint of the principal square root of X gate.
Phases the amplitude of |-> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0.5f + 0.5f * i}, {0.5f + 0.5f * i, 0.5f - 0.5f * i}},
                    {"+X", "+Y"},
                    R"CIRCUIT(
S 0
H 0
S 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_Y",
            GateType::SQRT_Y,
            GateType::SQRT_Y_DAG,
            0,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principal square root of Y gate.
Phases the amplitude of |-i> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0.5f + 0.5f * i, -0.5f - 0.5f * i}, {0.5f + 0.5f * i, 0.5f + 0.5f * i}},
                    {"-Z", "+X"},
                    R"CIRCUIT(
S 0
S 0
H 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_Y_DAG",
            GateType::SQRT_Y_DAG,
            GateType::SQRT_Y,
            0,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Adjoint of the principal square root of Y gate.
Phases the amplitude of |-i> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0.5f - 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
                    {"+Z", "-X"},
                    R"CIRCUIT(
H 0
S 0
S 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "S",
            GateType::S,
            GateType::S_DAG,
            0,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principal square root of Z gate.
Phases the amplitude of |1> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{1, 0}, {0, i}},
                    {"+Y", "+Z"},
                    R"CIRCUIT(
S 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "SQRT_Z", "S");

    add_gate(
        failed,
        Gate{
            "S_DAG",
            GateType::S_DAG,
            GateType::S,
            0,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Adjoint of the principal square root of Z gate.
Phases the amplitude of |1> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{1, 0}, {0, -i}},
                    {"-Y", "+Z"},
                    R"CIRCUIT(
S 0
S 0
S 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "SQRT_Z_DAG", "S_DAG");
}
