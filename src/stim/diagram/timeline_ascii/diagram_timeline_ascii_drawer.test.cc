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

#include "stim/diagram/timeline_ascii/diagram_timeline_ascii_drawer.h"

#include "gtest/gtest.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_surface_code.h"

using namespace stim;
using namespace stim_draw_internal;

TEST(circuit_diagram_timeline_text, single_qubit_gates) {
    Circuit circuit(R"CIRCUIT(
        I 0
        X 1
        Y 2
        Z 3
        C_XYZ 0
        C_ZYX 1
        H 2
        H_XY 3
        H_XZ 0
        H_YZ 1
        S 2
        SQRT_X 3
        SQRT_X_DAG 0
        SQRT_Y 1
        SQRT_Y_DAG 2
        SQRT_Z 3
        SQRT_Z_DAG 0
        S_DAG 1
        H 2 0 3
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
q0: -I-C_XYZ---H----SQRT_X_DAG-S_DAG-H-

q1: -X-C_ZYX--H_YZ----SQRT_Y---S_DAG---

q2: -Y---H-----S----SQRT_Y_DAG---H-----

q3: -Z-H_XY--SQRT_X-----S------------H-
)DIAGRAM");
}

TEST(circuit_diagram_timeline_text, two_qubits_gates) {
    Circuit circuit(R"CIRCUIT(
        CNOT 0 1
        CX 2 3
        CY 4 5 5 4
        CZ 0 2
        ISWAP 1 3
        ISWAP_DAG 2 4
        SQRT_XX 3 5
        SQRT_XX_DAG 0 5
        SQRT_YY 3 4 4 3
        SQRT_YY_DAG 0 1
        SQRT_ZZ 2 3
        SQRT_ZZ_DAG 4 5
        SWAP 0 1
        XCX 2 3
        XCY 3 4
        XCZ 0 1
        YCX 2 3
        YCY 4 5
        YCZ 0 1
        ZCX 2 3
        ZCY 4 5
        ZCZ 0 5 2 3 1 4
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
q0: -@-@-------------------------SQRT_XX_DAG---------SQRT_YY_DAG----SWAP-------X-Y---@-----
     | |                              |                   |           |        | |   |
q1: -X-|-ISWAP------------------------|--------------SQRT_YY_DAG----SWAP-------@-@---|---@-
       |   |                          |                                              |   |
q2: -@-@---|---ISWAP_DAG--------------|----------------------------SQRT_ZZ---X---Y-@-|-@-|-
     |     |       |                  |                               |      |   | | | | |
q3: -X---ISWAP-----|-----SQRT_XX------|------SQRT_YY---SQRT_YY-----SQRT_ZZ---X-X-X-X-|-@-|-
                   |        |         |         |         |                    |     |   |
q4: -@-Y-------ISWAP_DAG----|---------|------SQRT_YY---SQRT_YY---SQRT_ZZ_DAG---Y-Y-@-|---@-
     | |                    |         |                               |          | | |
q5: -Y-@-----------------SQRT_XX-SQRT_XX_DAG---------------------SQRT_ZZ_DAG-----Y-Y-@-----
)DIAGRAM");
}

TEST(circuit_diagram_timeline_text, noise_gates) {
    Circuit circuit(R"CIRCUIT(
        DEPOLARIZE1(0.125) 0 1
        DEPOLARIZE2(0.125) 0 2 4 5
        X_ERROR(0.125) 0 1 2
        Y_ERROR(0.125) 0 1 4
        Z_ERROR(0.125) 2 3 5
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
q0: -DEPOLARIZE1(0.125)-DEPOLARIZE2(0.125)-X_ERROR(0.125)-Y_ERROR(0.125)-
                                |
q1: -DEPOLARIZE1(0.125)---------|----------X_ERROR(0.125)-Y_ERROR(0.125)-
                                |
q2: --------------------DEPOLARIZE2(0.125)-X_ERROR(0.125)-Z_ERROR(0.125)-

q3: ------------------------------------------------------Z_ERROR(0.125)-

q4: --------------------DEPOLARIZE2(0.125)----------------Y_ERROR(0.125)-
                                |
q5: --------------------DEPOLARIZE2(0.125)----------------Z_ERROR(0.125)-
)DIAGRAM");

    circuit = Circuit(R"CIRCUIT(
        E(0.25) X1 X2
        CORRELATED_ERROR(0.125) X1 Y2 Z3
        ELSE_CORRELATED_ERROR(0.25) X2 Y4 Z3
        ELSE_CORRELATED_ERROR(0.25) X5
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
q0: --------------------------------------------------------------------------------------

q1: -E[X](0.25)-E[X](0.125)---------------------------------------------------------------
         |           |
q2: -E[X](0.25)-E[Y](0.125)-ELSE_CORRELATED_ERROR[X](0.25)--------------------------------
                     |                    |
q3: ------------E[Z](0.125)-ELSE_CORRELATED_ERROR[Z](0.25)--------------------------------
                                          |
q4: ------------------------ELSE_CORRELATED_ERROR[Y](0.25)--------------------------------

q5: -------------------------------------------------------ELSE_CORRELATED_ERROR[X](0.25)-
)DIAGRAM");

    circuit = Circuit(R"CIRCUIT(
        PAULI_CHANNEL_1(0.125,0.25,0.125) 0 1 2 3
        PAULI_CHANNEL_2(0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01) 0 1 2 4
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
q0: -PAULI_CHANNEL_1(0.125,0.25,0.125)-PAULI_CHANNEL_2[0](0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01)-
                                                                                     |
q1: -PAULI_CHANNEL_1(0.125,0.25,0.125)-PAULI_CHANNEL_2[1](0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01)-

q2: -PAULI_CHANNEL_1(0.125,0.25,0.125)-PAULI_CHANNEL_2[0](0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01)-
                                                                                     |
q3: -PAULI_CHANNEL_1(0.125,0.25,0.125)-----------------------------------------------|------------------------------------------------
                                                                                     |
q4: -----------------------------------PAULI_CHANNEL_2[1](0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01)-
)DIAGRAM");
}

TEST(circuit_diagram_timeline_text, collapsing) {
    Circuit circuit(R"CIRCUIT(
        R 0
        RX 1
        RY 2
        RZ 3
        M(0.001) 0 1
        MR 1 0
        MRX 1 2
        MRY 0 3 1
        MRZ 0
        MX 1
        MY 2
        MZ 3
        MPP X0*Y2 Z3 X1 Z2*Y3
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
q0: -R--M[0](0.001)-MR[3]-MRY[6]-MR[9]---------MPP[X][13]------------
                                                   |
q1: -RX-M[1](0.001)-MR[2]-MRX[4]-MRY[8]-MX[10]-----|------MPP[X][15]-
                                                   |
q2: -RY-------------------MRX[5]--------MY[11]-MPP[Y][13]-MPP[Z][16]-
                                                              |
q3: -R--------------------MRY[7]--------M[12]--MPP[Z][14]-MPP[Y][16]-
)DIAGRAM");
}

TEST(circuit_diagram_timeline_text, repeat) {
    auto circuit = Circuit(R"CIRCUIT(
        H 0 1 2
        REPEAT 5 {
            RX 2
            REPEAT 100 {
                H 0 1 3 3
            }
        }
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
       /REP 5    /REP 100     \ \
q0: -H-|---------|--------H---|-|---
       |         |            | |
q1: -H-|---------|--------H---|-|---
       |         |            | |
q2: -H-|------RX-|------------|-|---
       |         |            | |
q3: ---|---------|--------H-H-|-|---
       \         \            / /
)DIAGRAM");
}

TEST(circuit_diagram_timeline_text, classical_feedback) {
    auto circuit = Circuit(R"CIRCUIT(
        M 0
        CX rec[-1] 1
        YCZ 2 sweep[5]
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
q0: ----M[0]----

q1: ----X^m0----

q2: -Y^sweep[5]-
)DIAGRAM");
}

TEST(circuit_diagram_timeline_text, lattice_surgery_cnot) {
    auto circuit = Circuit(R"CIRCUIT(
        R 2
        MPP X1*X2
        MPP Z0*Z2
        MX 2
        CZ rec[-3] 0
        CX rec[-2] 1
        CZ rec[-1] 0
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
q0: ----------MZZ[1]-Z^m0--Z^m2-
                |
q1: ---MXX[0]---|----X^m1-------
         |      |
q2: -R-MXX[0]-MZZ[1]-MX[2]------
)DIAGRAM");
}

TEST(circuit_diagram_timeline_text, tick) {
    auto circuit = Circuit(R"CIRCUIT(
        H 0 0
        TICK
        H 0 1
        TICK
        H 0
        REPEAT 1 {
            H 0 1
            TICK
            H 0
            S 0
        }
        H 0 0
        SQRT_X 0
        TICK
        H 0 0
    )CIRCUIT");
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
     /-\     /REP 1   /-\ \ /--------\
q0: -H-H-H-H-|------H-H-S-|-H-H-SQRT_X-H-H-
             |            |
q1: -----H---|------H-----|----------------
     \-/     \        \-/ / \--------/
)DIAGRAM");
}

TEST(circuit_diagram_timeline_text, surface_code) {
    CircuitGenParameters params(10, 3, "unrotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    ASSERT_EQ("\n" + DiagramTimelineAsciiDrawer::from_circuit(circuit).str(), R"DIAGRAM(
      /-----------------\     /-------\ /-------\            /REP 9       /-------\ /-------\            \
 q0: -QUBIT_COORDS(0,0)-R-----------------@-------X----------|------------------------@-------X----------|-M[120]-
                                          |       |          |                        |       |          |
 q1: -QUBIT_COORDS(1,0)-R-H-@-@-----------|-------@-H-MR[0]--|--------H-@-@-----------|-------@-H-MR[12]-|--------
                            | |           |                  |          | |           |                  |
 q2: -QUBIT_COORDS(2,0)-R---X-|-----------|-@-----X----------|----------X-|-----------|-@-----X----------|-M[121]-
                              |           | |     |          |            |           | |     |          |
 q3: -QUBIT_COORDS(3,0)-R-H-@-|-@---------|-|-----@-H-MR[1]--|--------H-@-|-@---------|-|-----@-H-MR[13]-|--------
                            | | |         | |                |          | | |         | |                |
 q4: -QUBIT_COORDS(4,0)-R---X-|-|---------|-|-@--------------|----------X-|-|---------|-|-@--------------|-M[122]-
                              | |         | | |              |            | |         | | |              |
 q5: -QUBIT_COORDS(0,1)-R---X-|-|-X-------X-|-|-------MR[2]--|----------X-|-|-X-------X-|-|-------MR[14]-|--------
                            | | | |         | |              |          | | | |         | |              |
 q6: -QUBIT_COORDS(1,1)-R---@-X-|-|-----X---|-|---@----------|----------@-X-|-|-----X---|-|---@----------|-M[123]-
                                | |     |   | |   |          |              | |     |   | |   |          |
 q7: -QUBIT_COORDS(2,1)-R---X---|-|-X---|---X-|---X---MR[3]--|----------X---|-|-X---|---X-|---X---MR[15]-|--------
                            |   | | |   |     |              |          |   | | |   |     |              |
 q8: -QUBIT_COORDS(3,1)-R---@---X-|-|---|-X---|---@----------|----------@---X-|-|---|-X---|---@----------|-M[124]-
                                  | |   | |   |   |          |                | |   | |   |   |          |
 q9: -QUBIT_COORDS(4,1)-R---------|-|-X-|-|---X---X---MR[4]--|----------------|-|-X-|-|---X---X---MR[16]-|--------
                                  | | | | |                  |                | | | | |                  |
q10: -QUBIT_COORDS(0,2)-R---------@-|-|-|-|-@-----X----------|----------------@-|-|-|-|-@-----X----------|-M[125]-
                                    | | | | |     |          |                  | | | | |     |          |
q11: -QUBIT_COORDS(1,2)-R-H-@-@-----|-|-@-|-|-----@-H-MR[5]--|--------H-@-@-----|-|-@-|-|-----@-H-MR[17]-|--------
                            | |     | |   | |                |          | |     | |   | |                |
q12: -QUBIT_COORDS(2,2)-R---X-|-----@-|---|-|-@---X----------|----------X-|-----@-|---|-|-@---X----------|-M[126]-
                              |       |   | | |   |          |            |       |   | | |   |          |
q13: -QUBIT_COORDS(3,2)-R-H-@-|-@-----|---@-|-|---@-H-MR[6]--|--------H-@-|-@-----|---@-|-|---@-H-MR[18]-|--------
                            | | |     |     | |              |          | | |     |     | |              |
q14: -QUBIT_COORDS(4,2)-R---X-|-|-----@-----|-|-@------------|----------X-|-|-----@-----|-|-@------------|-M[127]-
                              | |           | | |            |            | |           | | |            |
q15: -QUBIT_COORDS(0,3)-R---X-|-|-X---------X-|-|-----MR[7]--|----------X-|-|-X---------X-|-|-----MR[19]-|--------
                            | | | |           | |            |          | | | |           | |            |
q16: -QUBIT_COORDS(1,3)-R---@-X-|-|-----X-----|-|-@----------|----------@-X-|-|-----X-----|-|-@----------|-M[128]-
                                | |     |     | | |          |              | |     |     | | |          |
q17: -QUBIT_COORDS(2,3)-R---X---|-|-X---|-----X-|-X---MR[8]--|----------X---|-|-X---|-----X-|-X---MR[20]-|--------
                            |   | | |   |       |            |          |   | | |   |       |            |
q18: -QUBIT_COORDS(3,3)-R---@---X-|-|---|-X-----|-@----------|----------@---X-|-|---|-X-----|-@----------|-M[129]-
                                  | |   | |     | |          |                | |   | |     | |          |
q19: -QUBIT_COORDS(4,3)-R---------|-|-X-|-|-----X-X---MR[9]--|----------------|-|-X-|-|-----X-X---MR[21]-|--------
                                  | | | | |                  |                | | | | |                  |
q20: -QUBIT_COORDS(0,4)-R---------@-|-|-|-|-------X----------|----------------@-|-|-|-|-------X----------|-M[130]-
                                    | | | |       |          |                  | | | |       |          |
q21: -QUBIT_COORDS(1,4)-R-H-@-------|-|-@-|-------@-H-MR[10]-|--------H-@-------|-|-@-|-------@-H-MR[22]-|--------
                            |       | |   |                  |          |       | |   |                  |
q22: -QUBIT_COORDS(2,4)-R---X-------@-|---|-------X----------|----------X-------@-|---|-------X----------|-M[131]-
                                      |   |       |          |                    |   |       |          |
q23: -QUBIT_COORDS(3,4)-R-H-@---------|---@-------@-H-MR[11]-|--------H-@---------|---@-------@-H-MR[23]-|--------
                            |         |                      |          |         |                      |
q24: -QUBIT_COORDS(4,4)-R---X---------@----------------------|----------X---------@----------------------|-M[132]-
      \-----------------/     \-------/ \-------/            \            \-------/ \-------/            /
)DIAGRAM");
}
