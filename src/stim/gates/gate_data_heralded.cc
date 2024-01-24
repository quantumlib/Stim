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

void GateDataMap::add_gate_data_heralded(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "HERALDED_ERASE",
            .id = GateType::HERALDED_ERASE,
            .best_candidate_inverse_id = GateType::HERALDED_ERASE,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES |
                                 GATE_PRODUCES_RESULTS),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
The heralded erasure noise channel.

Whether or not this noise channel fires is recorded into the measurement
record. When it doesn't fire, nothing happens to the target qubit and a
0 is recorded. When it does fire, a 1 is recorded and the target qubit
is erased to the maximally mixed state by applying X_ERROR(0.5) and
Z_ERROR(0.5).

CAUTION: when converting a circuit with this error into a detector
error model, this channel is split into multiple potential effects.
In the context of a DEM, these effects are considered independent.
This is an approximation, because independent effects can be combined.
The effect of this approximation, assuming a detector is declared
on the herald, is that it appears this detector can be cancelled out
by two of the (originally disjoint) heralded effects firing together.
Sampling from the DEM instead of the circuit can thus produce unheralded
errors, even if the circuit noise model only contains heralded errors.
These issues occur with probability p^2, where p is the probability of a
heralded error, since two effects that came from the same heralded error
must occur together to cancel out the herald detector. This also means
a decoder configured using the DEM will think there's a chance of unheralded
errors even if the circuit the DEM came from only uses heralded errors.

Parens Arguments:

    A single float (p) specifying the chance of the noise firing.

Targets:

    Qubits to apply single-qubit depolarizing noise to. Each target
    is operated on independently.

Pauli Mixture:

    1-p: record 0, apply I
    p/4: record 1, apply I
    p/4: record 1, apply X
    p/4: record 1, apply Y
    p/4: record 1, apply Z

Examples:

    # Erase qubit 0 with probability 1%
    HERALDED_ERASE(0.01) 0
    # Declare a flag detector based on the erasure
    DETECTOR rec[-1]

    # Erase qubit 2 with 2% probability
    # Separately, erase qubit 3 with 2% probability
    HERALDED_ERASE(0.02) 2 3

    # Do an XXXX measurement
    MPP X2*X3*X5*X7
    # Apply partially-heralded noise to the two qubits
    HERALDED_ERASE(0.01) 2 3 5 7
    DEPOLARIZE1(0.0001) 2 3 5 7
    # Repeat the XXXX measurement
    MPP X2*X3*X5*X7
    # Declare a detector comparing the two XXXX measurements
    DETECTOR rec[-1] rec[-6]
    # Declare flag detectors based on the erasures
    DETECTOR rec[-2]
    DETECTOR rec[-3]
    DETECTOR rec[-4]
    DETECTOR rec[-5]
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "HERALDED_PAULI_CHANNEL_1",
            .id = GateType::HERALDED_PAULI_CHANNEL_1,
            .best_candidate_inverse_id = GateType::HERALDED_PAULI_CHANNEL_1,
            .arg_count = 4,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES |
                                 GATE_PRODUCES_RESULTS),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
A heralded error channel that applies biased noise.

This error records a bit into the measurement record, indicating whether
or not the herald fired. How likely it is that the herald fires, and the
corresponding chance of each possible error effect (I, X, Y, or Z) are
configured by the parens arguments of the instruction.

CAUTION: when converting a circuit with this error into a detector
error model, this channel is split into multiple potential effects.
In the context of a DEM, these effects are considered independent.
This is an approximation, because independent effects can be combined.
The effect of this approximation, assuming a detector is declared
on the herald, is that it appears this detector can be cancelled out
by two of the (originally disjoint) heralded effects firing together.
Sampling from the DEM instead of the circuit can thus produce unheralded
errors, even if the circuit noise model only contains heralded errors.
These issues occur with probability p^2, where p is the probability of a
heralded error, since two effects that came from the same heralded error
must occur together to cancel out the herald detector. This also means
a decoder configured using the DEM will think there's a chance of unheralded
errors even if the circuit the DEM came from only uses heralded errors.

Parens Arguments:

    This instruction takes four arguments (pi, px, py, pz). The
    arguments are disjoint probabilities, specifying the chances
    of heralding with various effects.

    pi is the chance of heralding with no effect (a false positive).
    px is the chance of heralding with an X error.
    py is the chance of heralding with a Y error.
    pz is the chance of heralding with a Z error.

Targets:

    Qubits to apply heralded biased noise to.

Pauli Mixture:

    1-pi-px-py-pz: record 0, apply I
               pi: record 1, apply I
               px: record 1, apply X
               py: record 1, apply Y
               pz: record 1, apply Z

Examples:

    # With 10% probability perform a phase flip of qubit 0.
    HERALDED_PAULI_CHANNEL_1(0, 0, 0, 0.1) 0
    DETECTOR rec[-1]  # Include the herald in detectors available to the decoder

    # With 20% probability perform a heralded dephasing of qubit 0.
    HERALDED_PAULI_CHANNEL_1(0.1, 0, 0, 0.1) 0
    DETECTOR rec[-1]

    # Subject a Bell Pair to heralded noise.
    MXX 0 1
    MZZ 0 1
    HERALDED_PAULI_CHANNEL_1(0.01, 0.02, 0.03, 0.04) 0 1
    MXX 0 1
    MZZ 0 1
    DETECTOR rec[-1] rec[-5]  # Did ZZ stabilizer change?
    DETECTOR rec[-2] rec[-6]  # Did XX stabilizer change?
    DETECTOR rec[-3]    # Did the herald on qubit 1 fire?
    DETECTOR rec[-4]    # Did the herald on qubit 0 fire?
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });
}
