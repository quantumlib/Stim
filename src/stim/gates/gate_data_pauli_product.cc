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

#include "stim/gates/gates.h"

using namespace stim;

void GateDataMap::add_gate_data_pauli_product(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "MPP",
            .id = GateType::MPP,
            .best_candidate_inverse_id = GateType::MPP,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_PRODUCES_RESULTS | GATE_IS_NOISY | GATE_TARGETS_PAULI_STRING |
                                 GATE_TARGETS_COMBINERS | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "P_Generalized Pauli Product Gates",
            .help = R"MARKDOWN(
Measures general pauli product operators, like X1*Y2*Z3.

Parens Arguments:

    An optional failure probability.
    If no argument is given, all measurements are perfect.
    If one argument is given, it's the chance of reporting measurement results incorrectly.

Targets:

    A series of Pauli products to measure.

    Each Pauli product is a series of Pauli targets (like `X1`, `Y2`, or `Z3`) separated by
    combiners (`*`). Each Pauli term can be inverted (like `!Y2` instead of `Y2`). A negated
    product will record the opposite measurement result.

    Note that, although you can write down instructions that measure anti-Hermitian products,
    like `MPP X1*Z1`, doing this will cause exceptions when you simulate or analyze the
    circuit since measuring an anti-Hermitian operator doesn't have well defined semantics.

    Using overly-complicated Hermitian products, like saying `MPP X1*Y1*Y2*Z2` instead of
    `MPP !Z1*X2`, is technically allowed. But probably not a great idea since tools consuming
    the circuit may have assumed that each qubit would appear at most once in each product.

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
            .unitary_data = {},
            .flow_data =
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
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 1 1 1
H 0 1 3 4
CX 2 0 1 0 4 3
M 0 3
CX 2 0 1 0 4 3
H 0 1 3 4
S 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SPP",
            .id = GateType::SPP,
            .best_candidate_inverse_id = GateType::SPP_DAG,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_TARGETS_PAULI_STRING | GATE_TARGETS_COMBINERS | GATE_IS_UNITARY),
            .category = "P_Generalized Pauli Product Gates",
            .help = R"MARKDOWN(
The generalized S gate. Phases the -1 eigenspace of Pauli product observables by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    A series of Pauli products to phase.

    Each Pauli product is a series of Pauli targets (like `X1`, `Y2`, or `Z3`) separated by
    combiners (`*`). Each Pauli term can be inverted (like `!Y2` instead of `Y2`), to negate
    the product.

    Note that, although you can write down instructions that phase anti-Hermitian products,
    like `SPP X1*Z1`, doing this will cause exceptions when you simulate or analyze the
    circuit since phasing an anti-Hermitian operator doesn't have well defined semantics.

    Using overly-complicated Hermitian products, like saying `SPP X1*Y1*Y2*Z2` instead of
    `SPP !Z1*X2`, is technically allowed. But probably not a great idea since tools consuming
    the circuit may have assumed that each qubit would appear at most once in each product.

Examples:

    # Perform an S gate on qubit 1.
    SPP Z1

    # Perform a SQRT_X gate on qubit 1.
    SPP X1

    # Perform a SQRT_X_DAG gate on qubit 1.
    SPP !X1

    # Perform a SQRT_XX gate between qubit 1 and qubit 2.
    SPP X1*X2

    # Perform a SQRT_YY gate between qubit 1 and 2, and a SQRT_ZZ_DAG between qubit 3 and 4.
    SPP Y1*Y2 !Z1*Z2

    # Phase the -1 eigenspace of -X1*Y2*Z3 by i.
    SPP !X1*Y2*Z3

)MARKDOWN",
            .unitary_data = {},
            .flow_data =
                {
                    // For "SPP X0*Y1*Z2"
                    "X__ -> X__",
                    "Z__ -> -YYZ",
                    "_X_ -> -XZZ",
                    "_Z_ -> XXZ",
                    "__X -> XYY",
                    "__Z -> __Z",
                },
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CX 2 1
CX 1 0
S 1
S 1
H 1
CX 1 0
CX 2 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SPP_DAG",
            .id = GateType::SPP_DAG,
            .best_candidate_inverse_id = GateType::SPP,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_TARGETS_PAULI_STRING | GATE_TARGETS_COMBINERS | GATE_IS_UNITARY),
            .category = "P_Generalized Pauli Product Gates",
            .help = R"MARKDOWN(
The generalized S_DAG gate. Phases the -1 eigenspace of Pauli product observables by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    A series of Pauli products to phase.

    Each Pauli product is a series of Pauli targets (like `X1`, `Y2`, or `Z3`) separated by
    combiners (`*`). Each Pauli term can be inverted (like `!Y2` instead of `Y2`), to negate
    the product.

    Note that, although you can write down instructions that phase anti-Hermitian products,
    like `SPP X1*Z1`, doing this will cause exceptions when you simulate or analyze the
    circuit since phasing an anti-Hermitian operator doesn't have well defined semantics.

    Using overly-complicated Hermitian products, like saying `SPP X1*Y1*Y2*Z2` instead of
    `SPP !Z1*X2`, is technically allowed. But probably not a great idea since tools consuming
    the circuit may have assumed that each qubit would appear at most once in each product.

Examples:

    # Perform an S_DAG gate on qubit 1.
    SPP_DAG Z1

    # Perform a SQRT_X_DAG gate on qubit 1.
    SPP_DAG X1

    # Perform a SQRT_X gate on qubit 1.
    SPP_DAG !X1

    # Perform a SQRT_XX_DAG gate between qubit 1 and qubit 2.
    SPP_DAG X1*X2

    # Perform a SQRT_YY_DAG gate between qubit 1 and 2, and a SQRT_ZZ between qubit 3 and 4.
    SPP_DAG Y1*Y2 !Z1*Z2

    # Phase the -1 eigenspace of -X1*Y2*Z3 by -i.
    SPP_DAG !X1*Y2*Z3

)MARKDOWN",
            .unitary_data = {},
            .flow_data =
                {
                    // For "SPP_DAG X0*Y1*Z2"
                    "X__ -> X__",
                    "Z__ -> YYZ",
                    "_X_ -> XZZ",
                    "_Z_ -> -XXZ",
                    "__X -> -XYY",
                    "__Z -> __Z",
                },
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CX 2 1
CX 1 0
H 1
S 1
S 1
CX 1 0
CX 2 1
)CIRCUIT",
        });
}
