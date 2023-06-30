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

#include "stim/circuit/circuit.h"
#include "stim/circuit/gate_data.h"

using namespace stim;

void GateDataMap::add_gate_data_collapsing(bool &failed) {
    // ===================== Measure Gates. ============================
    add_gate(
        failed,
        Gate{
            "MX",
            GateType::MX,
            GateType::MX,
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                        GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis measurement.
Projects each target qubit into `|+>` or `|->` and reports its value (false=`|+>`, true=`|->`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the X basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the X basis, and append the result into the measurement record.
    MX 5

    # Measure qubit 5 in the X basis, and append the INVERSE of its result into the measurement record.
    MX !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MX(0.01) 5

    # Measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MX 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MX(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"X -> rec[-1]", "X -> +X"},
                    R"CIRCUIT(
H 0
M 0
H 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "MY",
            GateType::MY,
            GateType::MY,
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                        GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis measurement.
Projects each target qubit into `|i>` or `|-i>` and reports its value (false=`|i>`, true=`|-i>`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the Y basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Y basis, and append the result into the measurement record.
    MY 5

    # Measure qubit 5 in the Y basis, and append the INVERSE of its result into the measurement record.
    MY !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MY(0.01) 5

    # Measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MY 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MY(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"Y -> rec[-1]", "Y -> +Y"},
                    R"CIRCUIT(
S 0
S 0
S 0
H 0
M 0
H 0
S 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "M",
            GateType::M,
            GateType::M,
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                        GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis measurement.
Projects each target qubit into `|0>` or `|1>` and reports its value (false=`|0>`, true=`|1>`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the Z basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Z basis, and append the result into the measurement record.
    M 5

    # 'MZ' is the same as 'M'. This also measures qubit 5 in the Z basis.
    MZ 5

    # Measure qubit 5 in the Z basis, and append the INVERSE of its result into the measurement record.
    MZ !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MZ(0.01) 5

    # Measure multiple qubits in the Z basis, putting 3 bits into the measurement record.
    MZ 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MZ(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"Z -> rec[-1]", "Z -> +Z"},
                    R"CIRCUIT(
M 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "MZ", "M");

    // ===================== Measure+Reset Gates. ============================
    add_gate(
        failed,
        Gate{
            "MRX",
            GateType::MRX,
            GateType::MRX,
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                        GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis demolition measurement (optionally noisy).
Projects each target qubit into `|+>` or `|->`, reports its value (false=`|+>`, true=`|->`), then resets to `|+>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the X basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the X basis, reset it to the |+> state, append the measurement result into the measurement record.
    MRX 5

    # Demolition measure qubit 5 in the X basis, but append the INVERSE of its result into the measurement record.
    MRX !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRX(0.01) 5

    # Demolition measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MRX 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRX(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"X -> rec[-1]", "1 -> +X"},
                    R"CIRCUIT(
H 0
M 0
R 0
H 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "MRY",
            GateType::MRY,
            GateType::MRY,
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                        GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis demolition measurement (optionally noisy).
Projects each target qubit into `|i>` or `|-i>`, reports its value (false=`|i>`, true=`|-i>`), then resets to `|i>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the Y basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Y basis, reset it to the |i> state, append the measurement result into the measurement record.
    MRY 5

    # Demolition measure qubit 5 in the Y basis, but append the INVERSE of its result into the measurement record.
    MRY !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRY(0.01) 5

    # Demolition measure multiple qubits in the Y basis, putting 3 bits into the measurement record.
    MRY 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRY(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"Y -> rec[-1]", "1 -> +Y"},
                    R"CIRCUIT(
S 0
S 0
S 0
H 0
M 0
R 0
H 0
S 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "MR",
            GateType::MR,
            GateType::MR,
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                        GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis demolition measurement (optionally noisy).
Projects each target qubit into `|0>` or `|1>`, reports its value (false=`|0>`, true=`|1>`), then resets to `|0>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the Z basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Z basis, reset it to the |0> state, append the measurement result into the measurement record.
    MRZ 5

    # MR is also a Z-basis demolition measurement.
    MR 5

    # Demolition measure qubit 5 in the Z basis, but append the INVERSE of its result into the measurement record.
    MRZ !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRZ(0.01) 5

    # Demolition measure multiple qubits in the Z basis, putting 3 bits into the measurement record.
    MRZ 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRZ(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"Z -> rec[-1]", "1 -> +Z"},
                    R"CIRCUIT(
M 0
R 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "MRZ", "MR");

    // ===================== Reset Gates. ============================
    add_gate(
        failed,
        Gate{
            "RX",
            GateType::RX,
            GateType::MRX,
            0,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis reset.
Forces each target qubit into the `|+>` state by silently measuring it in the X basis and applying a `Z` gate if it ended up in the `|->` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the X basis.

Examples:

    # Reset qubit 5 into the |+> state.
    RX 5

    # Result multiple qubits into the |+> state.
    RX 2 3 5
)MARKDOWN",
                    {},
                    {"1 -> +X"},
                    R"CIRCUIT(
H 0
R 0
H 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "RY",
            GateType::RY,
            GateType::MRY,
            0,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis reset.
Forces each target qubit into the `|i>` state by silently measuring it in the Y basis and applying an `X` gate if it ended up in the `|-i>` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the Y basis.

Examples:

    # Reset qubit 5 into the |i> state.
    RY 5

    # Result multiple qubits into the |i> state.
    RY 2 3 5
)MARKDOWN",
                    {},
                    {"1 -> +Y"},
                    R"CIRCUIT(
S 0
S 0
S 0
H 0
R 0
H 0
S 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "R",
            GateType::R,
            GateType::MR,
            0,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis reset.
Forces each target qubit into the `|0>` state by silently measuring it in the Z basis and applying an `X` gate if it ended up in the `|1>` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the Z basis.

Examples:

    # Reset qubit 5 into the |0> state.
    RZ 5

    # R means the same thing as RZ.
    R 5

    # Reset multiple qubits into the |0> state.
    RZ 2 3 5
)MARKDOWN",
                    {},
                    {"1 -> +Z"},
                    R"CIRCUIT(
R 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "RZ", "R");

    add_gate(
        failed,
        Gate{
            "MPP",
            GateType::MPP,
            GateType::MPP,
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            (GateFlags)(GATE_PRODUCES_RESULTS | GATE_IS_NOISY | GATE_TARGETS_PAULI_STRING | GATE_TARGETS_COMBINERS |
                        GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Measure Pauli products.

Parens Arguments:

    Optional.
    A single float specifying the probability of flipping each reported measurement result.

Targets:

    A series of Pauli products to measure.

    Each Pauli product is a series of Pauli targets (`[XYZ]#`) separated by combiners (`*`).
    Products can be negated by prefixing a Pauli target in the product with an inverter (`!`)

Examples:

    # Measure the two-body +X1*Y2 observable.
    MPP X1*Y2

    # Measure the one-body -Z5 observable.
    MPP !Z5

    # Measure the two-body +X1*Y2 observable and also the three-body -Z3*Z4*Z5 observable.
    MPP X1*Y2 !Z3*Z4*Z5

    # Noisily measure +Z1+Z2 and +X1*X2 (independently flip each reported result 0.1% of the time).
    MPP(0.001) Z1*Z2 X1*X2

)MARKDOWN",
                    {},
                    {
                        "XYZ__ -> rec[-2]",
                        "___XX -> rec[-1]",
                        "X____ -> X____",
                        "_Y___ -> _Y___",
                        "__Z__ -> __Z__",
                        "___X_ -> ___X_",
                        "____X -> ____X",
                        "ZZ___ -> ZZ___",
                        "_XX__ -> _XX__",
                        "___ZZ -> ___ZZ",
                    },
                    R"CIRCUIT(
S 1 1 1
H 0 1 3 4
CX 2 0 1 0 4 3
M 0 3
CX 2 0 1 0 4 3
H 0 1 3 4
S 1
)CIRCUIT",
                };
            },
        });
}
