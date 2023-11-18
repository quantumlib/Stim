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

void GateDataMap::add_gate_data_swaps(bool &failed) {
    add_gate(
        failed,
        Gate{
            "SWAP",
            GateType::SWAP,
            GateType::SWAP,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            "C_Two Qubit Clifford Gates",
            R"MARKDOWN(
Swaps two qubits.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
            {"+IX", "+IZ", "+XI", "+ZI"},
            R"CIRCUIT(
CNOT 0 1
CNOT 1 0
CNOT 0 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            "ISWAP",
            GateType::ISWAP,
            GateType::ISWAP_DAG,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            "C_Two Qubit Clifford Gates",
            R"MARKDOWN(
Swaps two qubits and phases the -1 eigenspace of the ZZ observable by i.
Equivalent to `SWAP` then `CZ` then `S` on both targets.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            {{1, 0, 0, 0}, {0, 0, i, 0}, {0, i, 0, 0}, {0, 0, 0, 1}},
            {"+ZY", "+IZ", "+YZ", "+ZI"},
            R"CIRCUIT(
H 0
CNOT 0 1
CNOT 1 0
H 1
S 1
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            "ISWAP_DAG",
            GateType::ISWAP_DAG,
            GateType::ISWAP,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            "C_Two Qubit Clifford Gates",
            R"MARKDOWN(
Swaps two qubits and phases the -1 eigenspace of the ZZ observable by -i.
Equivalent to `SWAP` then `CZ` then `S_DAG` on both targets.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            {{1, 0, 0, 0}, {0, 0, -i, 0}, {0, -i, 0, 0}, {0, 0, 0, 1}},
            {"-ZY", "+IZ", "-YZ", "+ZI"},
            R"CIRCUIT(
S 0
S 0
S 0
S 1
S 1
S 1
H 1
CNOT 1 0
CNOT 0 1
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            "CXSWAP",
            GateType::CXSWAP,
            GateType::SWAPCX,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            "C_Two Qubit Clifford Gates",
            R"MARKDOWN(
A combination CX-then-SWAP gate.
This gate is kak-equivalent to the iswap gate, but preserves X/Z noise bias.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, {0, 1, 0, 0}},
            {"+XX", "+IZ", "+XI", "+ZZ"},
            R"CIRCUIT(
CNOT 1 0
CNOT 0 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            "SWAPCX",
            GateType::SWAPCX,
            GateType::CXSWAP,
            0,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            "C_Two Qubit Clifford Gates",
            R"MARKDOWN(
A combination SWAP-then-CX gate.
This gate is kak-equivalent to the iswap gate, but preserves X/Z noise bias.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 1, 0, 0}, {0, 0, 1, 0}},
            {"+IX", "+ZZ", "+XX", "+ZI"},
            R"CIRCUIT(
CNOT 0 1
CNOT 1 0
)CIRCUIT",
        });
}
