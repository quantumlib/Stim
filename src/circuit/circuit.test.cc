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

#include "circuit.test.h"

#include <gtest/gtest.h>

OpDat::OpDat(uint32_t target) : targets({target}) {
}

OpDat::OpDat(std::vector<uint32_t> targets) : targets(std::move(targets)) {
}

OpDat OpDat::flipped(size_t target) {
    return OpDat(target | TARGET_INVERTED_BIT);
}

OpDat::operator OperationData() {
    return {0, targets};
}

TEST(circuit, from_text) {
    Circuit expected;
    const auto &f = Circuit::from_text;
    ASSERT_EQ(f("# not an operation"), expected);

    expected.clear();
    expected.append_op("H", {0});
    ASSERT_EQ(f("H 0"), expected);
    ASSERT_EQ(f("h 0"), expected);
    ASSERT_EQ(f("H 0     "), expected);
    ASSERT_EQ(f("     H 0     "), expected);
    ASSERT_EQ(f("\tH 0\t\t"), expected);
    ASSERT_EQ(f("H 0  # comment"), expected);

    expected.clear();
    expected.append_op("H", {23});
    ASSERT_EQ(f("H 23"), expected);

    expected.clear();
    expected.append_op("DEPOLARIZE1", {4, 5}, 0.125);
    ASSERT_EQ(f("DEPOLARIZE1(0.125) 4 5  # comment"), expected);

    expected.clear();
    expected.append_op("ZCX", {5, 6});
    ASSERT_EQ(f("  \t Cnot 5 6  # comment   "), expected);

    ASSERT_THROW({ f("H a"); }, std::out_of_range);
    ASSERT_THROW({ f("H(1)"); }, std::out_of_range);
    ASSERT_THROW({ f("X_ERROR 1"); }, std::out_of_range);
    ASSERT_THROW({ f("H 9999999999999999999999999999999999999999999"); }, std::out_of_range);
    ASSERT_THROW({ f("H -1"); }, std::out_of_range);
    ASSERT_THROW({ f("CNOT 0 a"); }, std::out_of_range);
    ASSERT_THROW({ f("CNOT 0 99999999999999999999999999999999"); }, std::out_of_range);
    ASSERT_THROW({ f("CNOT 0 -1"); }, std::out_of_range);
    ASSERT_THROW({ f("DETECTOR 1 2"); }, std::out_of_range);
    ASSERT_THROW({ f("CX 1 1"); }, std::out_of_range);
    ASSERT_THROW({ f("SWAP 1 1"); }, std::out_of_range);
    ASSERT_THROW({ f("DEPOLARIZE2(1) 1 1"); }, std::out_of_range);
    ASSERT_THROW({ f("DETEstdCTOR rec[-1]"); }, std::out_of_range);
    ASSERT_THROW({ f("DETECTOR rec[0]"); }, std::out_of_range);
    ASSERT_THROW({ f("DETECTOR rec[1]"); }, std::out_of_range);
    ASSERT_THROW({ f("DETECTOR rec[-999999999999]"); }, std::out_of_range);
    ASSERT_THROW({ f("DETECTOR(2) rec[-1]"); }, std::out_of_range);
    ASSERT_THROW({ f("OBSERVABLE_INCLUDE rec[-1]"); }, std::out_of_range);
    ASSERT_THROW({ f("OBSERVABLE_INCLUDE(-1) rec[-1]"); }, std::out_of_range);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) B1"); }, std::out_of_range);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) X 1"); }, std::out_of_range);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) X\n"); }, std::out_of_range);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) 1"); }, std::out_of_range);
    ASSERT_THROW({ f("ELSE_CORRELATED_ERROR(1) 1 2"); }, std::out_of_range);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) 1 2"); }, std::out_of_range);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) A"); }, std::out_of_range);

    ASSERT_THROW({ f("CNOT 0\nCNOT 1"); }, std::out_of_range);

    expected.clear();
    ASSERT_EQ(f(""), expected);
    ASSERT_EQ(f("# Comment\n\n\n# More"), expected);

    expected.clear();
    expected.append_op("H_XZ", {0});
    ASSERT_EQ(f("H 0"), expected);

    expected.clear();
    expected.append_op("H_XZ", {0, 1});
    ASSERT_EQ(f("H 0 \n H 1"), expected);

    expected.clear();
    expected.append_op("H_XZ", {1});
    ASSERT_EQ(f("H 1"), expected);

    expected.clear();
    expected.append_op("H", {0});
    expected.append_op("ZCX", {0, 1});
    ASSERT_EQ(
        f("# EPR\n"
          "H 0\n"
          "CNOT 0 1"),
        expected);

    expected.clear();
    expected.append_op("M", {0, 0 | TARGET_INVERTED_BIT, 1, 1 | TARGET_INVERTED_BIT});
    ASSERT_EQ(f("M 0 !0 1 !1"), expected);

    expected.clear();
    expected.append_op("H", {0});
    expected.append_op("M", {0, 1, 2});
    expected.append_op("SWAP", {0, 1});
    expected.append_op("M", {0, 10});
    ASSERT_EQ(
        f("# Measurement fusion\n"
          "H 0\n"
          "M 0\n"
          "M 1\n"
          "M 2\n"
          "SWAP 0 1\n"
          "M 0\n"
          "M 10\n"),
        expected);

    expected.clear();
    expected.append_op("X", {0});
    expected.append_op("Y", {1, 2});
    expected.fusion_barrier();
    expected.append_op("Y", {1, 2});
    ASSERT_EQ(
        f("X 0\n"
          "REPEAT 2 {\n"
          "  Y 1\n"
          "  Y 2 #####\n"
          "} #####"),
        expected);

    expected.clear();
    expected.append_op("DETECTOR", {5 | TARGET_RECORD_BIT});
    ASSERT_EQ(f("DETECTOR rec[-5]"), expected);
    expected.clear();
    expected.append_op("DETECTOR", {6 | TARGET_RECORD_BIT});
    ASSERT_EQ(f("DETECTOR rec[-6]"), expected);

    expected.clear();
    expected.append_op("M", {0});
    expected.fusion_barrier();
    expected.append_op("M", {1, 2, 3});
    expected.fusion_barrier();
    expected.append_op("M", {1, 2, 3});
    expected.fusion_barrier();
    expected.append_op("M", {1, 2, 3});
    expected.fusion_barrier();
    expected.append_op("M", {1, 2, 3});
    expected.fusion_barrier();
    expected.append_op("M", {1, 2, 3});
    ASSERT_EQ(
        f("M 0\n"
          "REPEAT 5 {\n"
          "  M 1 2\n"
          "  M 3\n"
          "} #####"),
        expected);

    expected.clear();
    expected.append_op(
        "CORRELATED_ERROR",
        {90 | TARGET_PAULI_X_BIT, 91 | TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT, 92 | TARGET_PAULI_Z_BIT,
         93 | TARGET_PAULI_X_BIT},
        0.125);
    ASSERT_EQ(
        f(R"circuit(
            CORRELATED_ERROR(0.125) X90 Y91 Z92 X93
          )circuit"),
        expected);
}

TEST(circuit, append_circuit) {
    Circuit c1;
    c1.append_op("X", {0, 1});
    c1.append_op("M", {0, 1, 2, 4});

    Circuit c2;
    c2.append_op("M", {7});

    Circuit expected;
    expected.append_op("X", {0, 1});
    expected.append_op("M", {0, 1, 2, 4});
    expected.fusion_barrier();
    expected.append_op("M", {7});

    Circuit actual = c1;
    actual += c2;
    ASSERT_EQ(actual, expected);

    actual *= 4;
    for (size_t k = 0; k < 3; k++) {
        expected.append_op("X", {0, 1});
        expected.append_op("M", {0, 1, 2, 4});
        expected.fusion_barrier();
        expected.append_op("M", {7});
    }
    ASSERT_EQ(actual, expected);
    ASSERT_LT(actual.jag_targets.total_allocated(), 7 * 3);
    ASSERT_GT(expected.jag_targets.total_allocated(), 7 * 3);
}

TEST(circuit, append_op_fuse) {
    Circuit expected;
    Circuit actual;
    expected.append_op("H", {1, 2, 3});
    actual.append_op("H", {1}, 0);
    actual.append_op("H", {2, 3}, 0);
    ASSERT_EQ(actual, expected);

    actual.append_op("R", {0}, 0);
    expected.append_op("R", {0});
    ASSERT_EQ(actual, expected);

    actual.append_op("DETECTOR", {0, 0}, 0);
    actual.append_op("DETECTOR", {1, 1}, 0);
    expected.append_op("DETECTOR", {0, 0});
    expected.append_op("DETECTOR", {1, 1});
    ASSERT_EQ(actual, expected);

    actual.append_op("M", {0, 1}, 0);
    actual.append_op("M", {2, 3}, 0);
    expected.append_op("M", {0, 1, 2, 3});
    ASSERT_EQ(actual, expected);

    ASSERT_THROW({ actual.append_op("CNOT", {0}); }, std::out_of_range);
    ASSERT_THROW({ actual.append_op("X", {0}, 0.5); }, std::out_of_range);
}

TEST(circuit, str) {
    Circuit c;
    c.append_op("tick", {});
    c.append_op("CNOT", {2, 3});
    c.append_op("CNOT", {5 | TARGET_RECORD_BIT, 3});
    c.append_op("M", {1, 3, 2});
    c.append_op("DETECTOR", {7 | TARGET_RECORD_BIT});
    c.append_op("OBSERVABLE_INCLUDE", {11 | TARGET_RECORD_BIT, 1 | TARGET_RECORD_BIT}, 17);
    c.append_op("X_ERROR", {19}, 0.5);
    c.append_op(
        "CORRELATED_ERROR",
        {
            23 | TARGET_PAULI_X_BIT,
            27 | TARGET_PAULI_Z_BIT,
            29 | TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT,
        },
        0.25);
    ASSERT_EQ(c.str(), R"circuit(# Circuit [num_qubits=30, num_measurements=3]
TICK
CX 2 3 rec[-5] 3
M 1 3 2
DETECTOR rec[-7]
OBSERVABLE_INCLUDE(17) rec[-11] rec[-1]
X_ERROR(0.5) 19
E(0.25) X23 Z27 Y29)circuit");
}

TEST(circuit, append_op_validation) {
    Circuit c;
    ASSERT_THROW({ c.append_op("CNOT", {0}); }, std::out_of_range);
    c.append_op("CNOT", {0, 1});

    ASSERT_THROW({ c.append_op("X", {0 | TARGET_PAULI_X_BIT}); }, std::out_of_range);
    ASSERT_THROW({ c.append_op("X", {0 | TARGET_PAULI_Z_BIT}); }, std::out_of_range);
    ASSERT_THROW({ c.append_op("X", {0 | TARGET_INVERTED_BIT}); }, std::out_of_range);
    ASSERT_THROW({ c.append_op("X", {0}, 0.5); }, std::out_of_range);

    ASSERT_THROW({ c.append_op("M", {0 | TARGET_PAULI_X_BIT}); }, std::out_of_range);
    ASSERT_THROW({ c.append_op("M", {0 | TARGET_PAULI_Z_BIT}); }, std::out_of_range);
    c.append_op("M", {0 | TARGET_INVERTED_BIT});
    ASSERT_THROW({ c.append_op("M", {0}, 0.5); }, std::out_of_range);

    c.append_op("CORRELATED_ERROR", {0 | TARGET_PAULI_X_BIT});
    c.append_op("CORRELATED_ERROR", {0 | TARGET_PAULI_Z_BIT});
    ASSERT_THROW(
        { c.append_op("CORRELATED_ERROR", {0 | TARGET_PAULI_X_BIT | TARGET_INVERTED_BIT}); }, std::out_of_range);
    c.append_op("X_ERROR", {0}, 0.5);

    ASSERT_THROW({ c.append_op("CNOT", {0, 0}); }, std::out_of_range);
}

TEST(circuit, classical_controls) {
    ASSERT_THROW(
        {
            Circuit::from_text(R"circuit(
            M 0
            XCX rec[-1] 1
         )circuit");
        },
        std::out_of_range);

    Circuit expected;
    expected.append_op("CX", {0, 1, 1 | TARGET_RECORD_BIT, 1});
    expected.append_op("CY", {2 | TARGET_RECORD_BIT, 1});
    expected.append_op("CZ", {4 | TARGET_RECORD_BIT, 1});
    ASSERT_EQ(
        Circuit::from_text(R"circuit(# Circuit [num_qubits=5, num_measurements=0]
ZCX 0 1
ZCX rec[-1] 1
ZCY rec[-2] 1
ZCZ rec[-4] 1)circuit"),
        expected);
}
