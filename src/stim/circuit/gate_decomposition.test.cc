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
#include "stim/cmd/command_help.h"

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
        )CIRCUIT")
            .operations[0],
        10,
        append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        MPAD(0.125) 0
        TICK
        MPAD(0.125) 1
        TICK
        MPAD(0.125) 1
        TICK
    )CIRCUIT"));

    ASSERT_THROW(
        { decompose_mpp_operation(Circuit("MPP(0.125) X0*Y0*Z0").operations[0], 10, append_into_circuit); },
        std::invalid_argument);
}

TEST(gate_decomposition, decompose_pair_instruction_into_disjoint_segments) {
    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &segment) {
        out.safe_append(segment);
        out.append_from_text("TICK");
    };
    decompose_pair_instruction_into_disjoint_segments(
        Circuit("MXX(0.125) 0 1 0 2 3 5 4 5 3 4").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        MXX(0.125) 0 1
        TICK

        MXX(0.125) 0 2 3 5
        TICK

        MXX(0.125) 4 5
        TICK

        MXX(0.125) 3 4
        TICK
    )CIRCUIT"));

    out.clear();
    decompose_pair_instruction_into_disjoint_segments(Circuit("MZZ 0 1 1 2").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        MZZ 0 1
        TICK

        MZZ 1 2
        TICK
    )CIRCUIT"));

    out.clear();
    decompose_pair_instruction_into_disjoint_segments(Circuit("MZZ").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_simple) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(Circuit("SPP Z0").operations[0], 10, false, [&](const CircuitInstruction &inst) {
        out.safe_append(inst);
    });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        S 0
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_inverted) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(
        Circuit("SPP !Z0").operations[0], 10, false, [&](const CircuitInstruction &inst) {
            out.safe_append(inst);
        });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        S_DAG 0
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_inverted2) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(Circuit("SPP Z0").operations[0], 10, true, [&](const CircuitInstruction &inst) {
        out.safe_append(inst);
    });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        S_DAG 0
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_inverted3) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(
        Circuit("SPP_DAG Z0").operations[0], 10, false, [&](const CircuitInstruction &inst) {
            out.safe_append(inst);
        });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        S_DAG 0
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_double_inverted) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(Circuit("SPP !Z0").operations[0], 10, true, [&](const CircuitInstruction &inst) {
        out.safe_append(inst);
    });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        S 0
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_triple_inverted) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(
        Circuit("SPP_DAG !Z0").operations[0], 10, true, [&](const CircuitInstruction &inst) {
            out.safe_append(inst);
        });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        S_DAG 0
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_complex) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(
        Circuit("SPP X0*Y1*Z2").operations[0], 10, false, [&](const CircuitInstruction &inst) {
            out.safe_append(inst);
        });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        H 0
        H_YZ 1
        CX 1 0 2 0
        S 0
        CX 1 0 2 0
        H_YZ 1
        H 0
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_multiple) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(
        Circuit("SPP X0 Y0*!Z2").operations[0], 10, false, [&](const CircuitInstruction &inst) {
            out.safe_append(inst);
        });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        H 0
        S 0
        H 0
        H_YZ 0
        CX 2 0
        S_DAG 0
        CX 2 0
        H_YZ 0
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_empty) {
    Circuit out;
    decompose_spp_or_spp_dag_operation(Circuit("SPP").operations[0], 10, false, [&](const CircuitInstruction &inst) {
        out.safe_append(inst);
    });
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_spp_or_spp_dag_operation_bad) {
    ASSERT_THROW(
        {
            decompose_spp_or_spp_dag_operation(
                Circuit("SPP X0*Z0").operations[0], 10, false, [](const CircuitInstruction &inst) {
                });
        },
        std::invalid_argument);

    ASSERT_THROW(
        {
            decompose_spp_or_spp_dag_operation(
                Circuit("MPP X0*Z0").operations[0], 10, false, [](const CircuitInstruction &inst) {
                });
        },
        std::invalid_argument);
}

TEST(gate_decomposition, for_each_disjoint_target_segment_in_instruction_reversed) {
    simd_bits<64> buf(100);

    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &segment) {
        out.safe_append(segment);
        out.append_from_text("TICK");
    };
    for_each_disjoint_target_segment_in_instruction_reversed(
        Circuit("M(0.25) 0 1 2 3 2 5 6 7 1 5 6 6").operations[0], buf, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        M(0.25) 6
        TICK

        M(0.25) 7 1 5 6
        TICK

        M(0.25) 3 2 5 6
        TICK

        M(0.25) 0 1 2
        TICK

    )CIRCUIT"));
}

TEST(gate_decomposition, for_each_combined_targets_group) {
    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &segment) {
        out.safe_append(segment);
        out.append_from_text("TICK");
    };
    for_each_combined_targets_group(Circuit("MPP(0.25) X0 Z1 Y2*Z3 Y4*Z5*Z6 Z8").operations[0], append_into_circuit);
    for_each_combined_targets_group(Circuit("MPP(0.25) X0 Y1 Z2").operations[0], append_into_circuit);
    for_each_combined_targets_group(Circuit("MPP(0.25) X0*Y1 Z2*Z3").operations[0], append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        MPP(0.25) X0
        TICK
        MPP(0.25) Z1
        TICK
        MPP(0.25) Y2*Z3
        TICK
        MPP(0.25) Y4*Z5*Z6
        TICK
        MPP(0.25) Z8
        TICK
        MPP(0.25) X0
        TICK
        MPP(0.25) Y1
        TICK
        MPP(0.25) Z2
        TICK
        MPP(0.25) X0*Y1
        TICK
        MPP(0.25) Z2*Z3
        TICK
    )CIRCUIT"));
}
