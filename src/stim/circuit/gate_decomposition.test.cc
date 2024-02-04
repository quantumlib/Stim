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
    auto append_into_circuit = [&](const CircuitInstruction &inst) {
        out.safe_append(inst);
        out.append_from_text("TICK");
    };
    decompose_mpp_operation(
        Circuit("MPP(0.125) X0*X1*X2 Z3*Z4*Z5 X2*Y4 Z3 Z3 Z4*Z5").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        H 0 1 2
        TICK
        CX 1 0 2 0 4 3 5 3
        TICK
        M(0.125) 0 3
        TICK
        CX 1 0 2 0 4 3 5 3
        TICK
        H 0 1 2
        TICK

        H 2
        TICK
        H_YZ 4
        TICK
        CX 4 2
        TICK
        M(0.125) 2 3
        TICK
        CX 4 2
        TICK
        H_YZ 4
        TICK
        H 2
        TICK

        CX 5 4
        TICK
        M(0.125) 3 4
        TICK
        CX 5 4
        TICK
    )CIRCUIT"));

    out.clear();
    decompose_mpp_operation(Circuit("MPP X0*Z1*Y2 X3*X4 Y0*Y1*Y2*Y3*Y4").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        H 0 3 4
        TICK
        H_YZ 2
        TICK
        CX 1 0 2 0 4 3
        TICK
        M 0 3
        TICK
        CX 1 0 2 0 4 3
        TICK
        H_YZ 2
        TICK
        H 0 3 4
        TICK

        H_YZ 0 1 2 3 4
        TICK
        CX 1 0 2 0 3 0 4 0
        TICK
        M 0
        TICK
        CX 1 0 2 0 3 0 4 0
        TICK
        H_YZ 0 1 2 3 4
        TICK
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_mpp_to_mpad) {
    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &inst) {
        out.safe_append(inst);
        out.append_from_text("TICK");
    };
    decompose_mpp_operation(
        Circuit(R"CIRCUIT(
            MPP(0.125) X0*X0 X0*!X0 X0*Y0*Z0*X1*Y1*Z1
        )CIRCUIT").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        MPAD(0.125) 0
        TICK
        MPAD(0.125) 1
        TICK
        MPAD(0.125) 1
        TICK
    )CIRCUIT"));

    ASSERT_THROW({
            decompose_mpp_operation(
        Circuit("MPP(0.125) X0*Y0*Z0").operations[0], 10, append_into_circuit);
    }, std::invalid_argument);
}

//TEST(gate_decomposition, decompose_mpp_rec_targets) {
//    Circuit out;
//    auto append_into_circuit = [&](const CircuitInstruction &inst) {
//        out.safe_append(inst);
//        out.append_from_text("TICK");
//    };
//    decompose_mpp_operation(
//        Circuit(R"CIRCUIT(
//            MPP(0.125) rec[-1]*X0 rec[-2] X0*!X0*rec[-5]
//        )CIRCUIT").operations[0], 10, append_into_circuit);
//    ASSERT_EQ(out, Circuit(R"CIRCUIT(
//        MPAD(0.125) 0
//        TICK
//        MPAD(0.125) 1
//        TICK
//        MPAD(0.125) 1
//        TICK
//    )CIRCUIT"));
//
//    ASSERT_THROW({
//            decompose_mpp_operation(
//        Circuit("MPP(0.125) X0*Y0*Z0").operations[0], 10, append_into_circuit);
//    }, std::invalid_argument);
//}

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

TEST(gate_decomposition, decompose_cpp_operation_with_reverse_independence_swap) {
    Circuit out;
    decompose_cpp_operation_with_reverse_independence(
        Circuit("CPP X0*X1 Z0*Z1").operations[0],
        10,
        [&](const CircuitInstruction &inst) { out.safe_append(inst); });
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
