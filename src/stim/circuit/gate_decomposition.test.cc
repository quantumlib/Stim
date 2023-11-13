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

#include "stim/circuit/gate_decomposition.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"

using namespace stim;

TEST(gate_decomposition, decompose_mpp_operation) {
    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &h_xz,
                                   const CircuitInstruction &h_yz,
                                   const CircuitInstruction &cnot,
                                   const CircuitInstruction &meas) {
        out.safe_append(h_xz);
        out.safe_append(h_yz);
        out.safe_append(cnot);
        out.safe_append(meas);
        out.safe_append(cnot);
        out.safe_append(h_yz);
        out.safe_append(h_xz);
        out.append_from_text("TICK");
    };
    decompose_mpp_operation(
        Circuit("MPP(0.125) X0*X1*X2 Z3*Z4*Z5 X2*Y4 Z3 Z3 Z4*Z5").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        H 0 1 2
        H_YZ
        CX 1 0 2 0 4 3 5 3
        M(0.125) 0 3
        CX 1 0 2 0 4 3 5 3
        H_YZ
        H 0 1 2
        TICK

        H 2
        H_YZ 4
        CX 4 2
        M(0.125) 2 3
        CX 4 2
        H_YZ 4
        H 2
        TICK

        H
        H_YZ
        CX 5 4
        M(0.125) 3 4
        CX 5 4
        H_YZ
        H
        TICK
    )CIRCUIT"));

    out.clear();
    decompose_mpp_operation(Circuit("MPP X0*Z1*Y2 X3*X4 Y0*Y1*Y2*Y3*Y4").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        H 0 3 4
        H_YZ 2
        CX 1 0 2 0 4 3
        M 0 3
        CX 1 0 2 0 4 3
        H_YZ 2
        H 0 3 4
        TICK

        H
        H_YZ 0 1 2 3 4
        CX 1 0 2 0 3 0 4 0
        M 0
        CX 1 0 2 0 3 0 4 0
        H_YZ 0 1 2 3 4
        H
        TICK
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_pair_instruction_into_segments_with_single_use_controls) {
    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &segment) {
        std::vector<GateTarget> evens;
        for (size_t k = 0; k < segment.targets.size(); k += 2) {
            evens.push_back(segment.targets[k]);
        }
        out.safe_append(CircuitInstruction{GateType::CX, {}, segment.targets});
        out.safe_append(CircuitInstruction{GateType::MX, segment.args, evens});
        out.safe_append(CircuitInstruction{GateType::CX, {}, segment.targets});
        out.append_from_text("TICK");
    };
    decompose_pair_instruction_into_segments_with_single_use_controls(
        Circuit("MXX(0.125) 0 1 0 2 3 5 4 5 3 4").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        CX 0 1
        MX(0.125) 0
        CX 0 1
        TICK

        CX 0 2 3 5 4 5
        MX(0.125) 0 3 4
        CX 0 2 3 5 4 5
        TICK

        CX 3 4
        MX(0.125) 3
        CX 3 4
        TICK
    )CIRCUIT"));
}
