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
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/sparse_rev_frame_tracker.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

static constexpr std::complex<float> i = std::complex<float>(0, 1);

void GateDataMap::add_gate_data_pp(bool &failed) {
    add_gate(
        failed,
        Gate{
            "SQRT_XX",
            Gates::SQRT_XX,
            Gates::SQRT_XX_DAG,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the XX observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{0.5f + 0.5f * i, 0, 0, 0.5f - 0.5f * i},
                     {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                     {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                     {0.5f - 0.5f * i, 0, 0, 0.5f + 0.5f * i}},
                    {"+XI", "-YX", "+IX", "-XY"},
                    R"CIRCUIT(
H 0
CNOT 0 1
H 1
S 0
S 1
H 0
H 1
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "SQRT_XX_DAG",
            Gates::SQRT_XX_DAG,
            Gates::SQRT_XX,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the XX observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0, 0, 0.5f + 0.5f * i},
                     {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                     {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                     {0.5f + 0.5f * i, 0, 0, 0.5f - 0.5f * i}},
                    {"+XI", "+YX", "+IX", "+XY"},
                    R"CIRCUIT(
H 0
CNOT 0 1
H 1
S 0
S 0
S 0
S 1
S 1
S 1
H 0
H 1
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_YY",
            Gates::SQRT_YY,
            Gates::SQRT_YY_DAG,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the YY observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{0.5f + 0.5f * i, 0, 0, -0.5f + 0.5f * i},
                     {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                     {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                     {-0.5f + 0.5f * i, 0, 0, 0.5f + 0.5f * i}},
                    {"-ZY", "+XY", "-YZ", "+YX"},
                    R"CIRCUIT(
S 0
S 0
S 0
S 1
S 1
S 1
H 0
CNOT 0 1
H 1
S 0
S 1
H 0
H 1
S 0
S 1
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "SQRT_YY_DAG",
            Gates::SQRT_YY_DAG,
            Gates::SQRT_YY,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the YY observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0, 0, -0.5f - 0.5f * i},
                     {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                     {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                     {-0.5f - 0.5f * i, 0, 0, 0.5f - 0.5f * i}},
                    {"+ZY", "-XY", "+YZ", "-YX"},
                    R"CIRCUIT(
S 0
S 0
S 0
S 1
H 0
CNOT 0 1
H 1
S 0
S 1
H 0
H 1
S 0
S 1
S 1
S 1
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_ZZ",
            Gates::SQRT_ZZ,
            Gates::SQRT_ZZ_DAG,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the ZZ observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, i, 0, 0}, {0, 0, i, 0}, {0, 0, 0, 1}},
                    {"+YZ", "+ZI", "+ZY", "+IZ"},
                    R"CIRCUIT(
H 1
CNOT 0 1
H 1
S 0
S 1
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "SQRT_ZZ_DAG",
            Gates::SQRT_ZZ_DAG,
            Gates::SQRT_ZZ,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the ZZ observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, -i, 0, 0}, {0, 0, -i, 0}, {0, 0, 0, 1}},
                    {"-YZ", "+ZI", "-ZY", "+IZ"},
                    R"CIRCUIT(
H 1
CNOT 0 1
H 1
S 0
S 0
S 0
S 1
S 1
S 1
)CIRCUIT",
                };
            },
        });
}
