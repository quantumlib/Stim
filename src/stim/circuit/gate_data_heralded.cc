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

#include "stim/circuit/gate_data.h"

using namespace stim;

void GateDataMap::add_gate_data_heralded(bool &failed) {
    add_gate(
        failed,
        Gate{
            "HERALDED_ERASE",
            GateType::HERALDED_ERASE,
            GateType::HERALDED_ERASE,
            1,
            (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_PRODUCES_RESULTS),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
The heralded erasure noise channel.

Whether or not this noise channel fires is recorded into the measurement
record. When it doesn't fire, nothing happens to the target qubit and a
0 is recorded. When it does fire, a 1 is recorded and the target qubit
is erased to the maximally mixed state by applying X_ERROR(0.5) and
Z_ERROR(0.5).

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
                    {},
                    {},
                    nullptr,
                };
            },
        });
}
