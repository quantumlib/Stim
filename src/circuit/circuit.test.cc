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
    return OpDat(target | MEASURE_FLIPPED_MASK);
}

OpDat::operator OperationData() const {
    return {0, {&targets, 0, targets.size()}};
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
    ASSERT_THROW({ f("CNOT 0@-1 1@-2"); }, std::out_of_range);
    ASSERT_THROW({ f("DETECTOR 1 2"); }, std::out_of_range);
    ASSERT_THROW({ f("DETEstdCTOR 1@0"); }, std::out_of_range);
    ASSERT_THROW({ f("DETECTOR -1@0"); }, std::out_of_range);
    ASSERT_THROW({ f("DETECTOR(2) -1@0"); }, std::out_of_range);
    ASSERT_THROW({ f("OBSERVABLE_INCLUDE 1@-2"); }, std::out_of_range);

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
    expected.append_op("M", {0, 0 | MEASURE_FLIPPED_MASK, 1, 1 | MEASURE_FLIPPED_MASK});
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
    expected.append_op("Y", {1, 2});
    ASSERT_EQ(
        f("X 0\n"
          "REPEAT 2 {\n"
          "  Y 1\n"
          "  Y 2 #####\n"
          "} #####"),
        expected);

    expected.clear();
    expected.append_op("DETECTOR", {5, 0});
    ASSERT_EQ(f("DETECTOR 5@-1"), expected);

    expected.clear();
    expected.append_op("M", {0});
    expected.append_op("M", {1, 2, 3});
    expected.append_op("M", {1, 2, 3});
    expected.append_op("M", {1, 2, 3});
    expected.append_op("M", {1, 2, 3});
    expected.append_op("M", {1, 2, 3});
    ASSERT_EQ(
        f("M 0\n"
          "REPEAT 5 {\n"
          "  M 1 2\n"
          "  M 3\n"
          "} #####"),
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
    expected.append_op("M", {7});

    Circuit actual = c1;
    actual += c2;
    ASSERT_EQ(actual, expected);

    actual *= 4;
    for (size_t k = 0; k < 3; k++) {
        expected.append_op("X", {0, 1});
        expected.append_op("M", {0, 1, 2, 4});
        expected.append_op("M", {7});
    }
    ASSERT_EQ(actual, expected);
    ASSERT_EQ(actual.jagged_data.size(), 7);
    ASSERT_EQ(expected.jagged_data.size(), 7 * 4);
}
