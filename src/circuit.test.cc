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

#include "circuit.h"

#include <gtest/gtest.h>

TEST(circuit, operation_from_line) {
    auto f = [](std::string line) {
        return Instruction::from_line(StringView(&line[0], line.size()));
    };
    ASSERT_EQ(f("# not an operation"), Instruction(INSTRUCTION_TYPE_EMPTY, {}));

    auto h0 = GATE_DATA.at("H_XZ").applied(OperationData({0}));
    ASSERT_EQ(f("H 0"), h0);
    ASSERT_EQ(f("H 23"), GATE_DATA.at("H_XZ").applied(OperationData({23})));
    ASSERT_EQ(f("h 0"), h0);
    ASSERT_EQ(f("H 0     "), h0);
    ASSERT_EQ(f("     H 0     "), h0);
    ASSERT_EQ(f("\tH 0\t\t"), h0);
    ASSERT_EQ(f("H 0  # comment"), h0);
    ASSERT_EQ(
        f("DEPOLARIZE1(0.125) 4 5  # comment"),
        GATE_DATA.at("DEPOLARIZE1").applied(OperationData({4, 5}, {false, false}, 0.125)));

    ASSERT_EQ(f("  \t Cnot 5 6  # comment   "), GATE_DATA.at("ZCX").applied({{5, 6}}));

    ASSERT_THROW({ f("H a"); }, std::runtime_error);
    ASSERT_THROW({ f("H(1)"); }, std::runtime_error);
    ASSERT_THROW({ f("X_ERROR 1"); }, std::runtime_error);
    ASSERT_THROW({ f("H 9999999999999999999999999999999999999999999"); }, std::runtime_error);
    ASSERT_THROW({ f("H -1"); }, std::runtime_error);
    ASSERT_THROW({ f("CNOT 0 a"); }, std::runtime_error);
    ASSERT_THROW({ f("CNOT 0 99999999999999999999999999999999"); }, std::runtime_error);
    ASSERT_THROW({ f("CNOT 0 -1"); }, std::runtime_error);
    ASSERT_THROW({ f("CNOT 0@-1 1@-2"); }, std::runtime_error);
}

TEST(circuit, from_text) {
    ASSERT_EQ(Circuit::from_text(""), Circuit({}));
    ASSERT_EQ(Circuit::from_text("# Comment\n\n\n# More"), Circuit({}));
    ASSERT_EQ(Circuit::from_text("H 0"), Circuit({GATE_DATA.at("H_XZ").applied({0})}));
    ASSERT_EQ(Circuit::from_text("H 0 \n H 1"), Circuit({GATE_DATA.at("H_XZ").applied({0, 1})}));
    ASSERT_EQ(Circuit::from_text("H 1"), Circuit({GATE_DATA.at("H_XZ").applied({1})}));
    ASSERT_EQ(
        Circuit::from_text("# EPR\n"
                           "H 0\n"
                           "CNOT 0 1"),
        Circuit({GATE_DATA.at("H_XZ").applied({0}), GATE_DATA.at("ZCX").applied({0, 1})}));
    ASSERT_EQ(
        Circuit::from_text("M 0 !0 1 !1"),
        Circuit({GATE_DATA.at("M").applied(OperationData({0, 0, 1, 1}, {false, true, false, true}, 0))}));
    ASSERT_EQ(
        Circuit::from_text("# Measurement fusion\n"
                           "H 0\n"
                           "M 0\n"
                           "M 1\n"
                           "M 2\n"
                           "SWAP 0 1\n"
                           "M 0\n"
                           "M 10\n"),
        Circuit({
            GATE_DATA.at("H_XZ").applied({0}),
            GATE_DATA.at("M").applied({0, 1, 2}),
            GATE_DATA.at("SWAP").applied({0, 1}),
            GATE_DATA.at("M").applied({0, 10}),
        }));

    ASSERT_EQ(
        Circuit::from_text("X 0\n"
                           "REPEAT 2 {\n"
                           "  Y 1\n"
                           "  Y 2 #####\n"
                           "} #####"),
        Circuit({
            GATE_DATA.at("X").applied({0}),
            GATE_DATA.at("Y").applied({1, 2}),
            GATE_DATA.at("Y").applied({1, 2}),
        }));

    ASSERT_EQ(
        Circuit::from_text("X 0\n"
                           "REPEAT 2 {\n"
                           "  Y 1\n"
                           "  REPEAT 3 {\n"
                           "    Z 1\n"
                           "  }\n"
                           "}"),
        Circuit({
            GATE_DATA.at("X").applied({0}),
            GATE_DATA.at("Y").applied({1}),
            GATE_DATA.at("Z").applied({1}),
            GATE_DATA.at("Z").applied({1}),
            GATE_DATA.at("Z").applied({1}),
            GATE_DATA.at("Y").applied({1}),
            GATE_DATA.at("Z").applied({1}),
            GATE_DATA.at("Z").applied({1}),
            GATE_DATA.at("Z").applied({1}),
        }));
}
