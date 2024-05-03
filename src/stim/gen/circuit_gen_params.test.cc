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

#include "stim/gen/circuit_gen_params.h"

#include "gtest/gtest.h"

#include "stim/simulators/frame_simulator_util.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST(circuit_gen_params, append_begin_round_tick) {
    CircuitGenParameters params(3, 5, "test");
    Circuit circuit;

    circuit.clear();
    params.append_begin_round_tick(circuit, {1, 2, 3});
    ASSERT_EQ(circuit, Circuit("TICK"));

    params.before_round_data_depolarization = 0.125;
    circuit.clear();
    params.append_begin_round_tick(circuit, {1, 2, 3});
    ASSERT_EQ(circuit, Circuit("TICK\nDEPOLARIZE1(0.125) 1 2 3"));
}

TEST(circuit_gen_params, append_unitary_1) {
    CircuitGenParameters params(3, 5, "test");
    Circuit circuit;

    circuit.clear();
    params.append_unitary_1(circuit, "H", {2, 3, 5});
    ASSERT_EQ(circuit, Circuit("H 2 3 5"));

    params.after_clifford_depolarization = 0.125;
    circuit.clear();
    params.append_unitary_1(circuit, "H", {2, 3, 5});
    ASSERT_EQ(circuit, Circuit("H 2 3 5\nDEPOLARIZE1(0.125) 2 3 5"));
}

TEST(circuit_gen_params, append_unitary_2) {
    CircuitGenParameters params(3, 5, "test");
    Circuit circuit;

    circuit.clear();
    params.append_unitary_2(circuit, "CNOT", {2, 3, 5, 7});
    ASSERT_EQ(circuit, Circuit("CX 2 3 5 7"));

    params.after_clifford_depolarization = 0.125;
    circuit.clear();
    params.append_unitary_2(circuit, "CNOT", {2, 3, 5, 7});
    ASSERT_EQ(circuit, Circuit("CX 2 3 5 7\nDEPOLARIZE2(0.125) 2 3 5 7"));
}

TEST(circuit_gen_params, append_reset) {
    CircuitGenParameters params(3, 5, "test");
    Circuit circuit;

    circuit.clear();
    params.append_reset(circuit, {2, 3, 5});
    params.append_reset(circuit, {2, 3, 5}, 'Z');
    ASSERT_EQ(circuit, Circuit("R 2 3 5\nR 2 3 5"));
    params.append_reset(circuit, {1}, 'X');
    params.append_reset(circuit, {4}, 'Y');
    ASSERT_EQ(circuit, Circuit("R 2 3 5\nR 2 3 5\nRX 1\nRY 4"));

    params.after_reset_flip_probability = 0.125;
    circuit.clear();
    params.append_reset(circuit, {2, 3, 5});
    ASSERT_EQ(circuit, Circuit("R 2 3 5\nX_ERROR(0.125) 2 3 5"));
    params.append_reset(circuit, {1}, 'X');
    params.append_reset(circuit, {4}, 'Y');
    ASSERT_EQ(circuit, Circuit("R 2 3 5\nX_ERROR(0.125) 2 3 5\nRX 1\nZ_ERROR(0.125) 1\nRY 4\nX_ERROR(0.125) 4"));
}

TEST(circuit_gen_params, append_measure) {
    CircuitGenParameters params(3, 5, "test");
    Circuit circuit;

    circuit.clear();
    params.append_measure(circuit, {2, 3, 5});
    params.append_measure(circuit, {2, 3, 5}, 'Z');
    ASSERT_EQ(circuit, Circuit("M 2 3 5\nM 2 3 5"));
    params.append_measure(circuit, {1}, 'X');
    params.append_measure(circuit, {4}, 'Y');
    ASSERT_EQ(circuit, Circuit("M 2 3 5\nM 2 3 5\nMX 1\nMY 4"));

    params.before_measure_flip_probability = 0.125;
    circuit.clear();
    params.append_measure(circuit, {2, 3, 5});
    ASSERT_EQ(circuit, Circuit("X_ERROR(0.125) 2 3 5\nM 2 3 5"));
    params.append_measure(circuit, {1}, 'X');
    params.append_measure(circuit, {4}, 'Y');
    ASSERT_EQ(circuit, Circuit("X_ERROR(0.125) 2 3 5\nM 2 3 5\nZ_ERROR(0.125) 1\nMX 1\nX_ERROR(0.125) 4\nMY 4"));
}

TEST(circuit_gen_params, append_measure_reset) {
    CircuitGenParameters params(3, 5, "test");
    Circuit circuit;

    circuit.clear();
    params.append_measure_reset(circuit, {2, 3, 5});
    params.append_measure_reset(circuit, {2, 3, 5}, 'Z');
    ASSERT_EQ(circuit, Circuit("MR 2 3 5\nMR 2 3 5"));
    params.append_measure_reset(circuit, {1}, 'X');
    params.append_measure_reset(circuit, {4}, 'Y');
    ASSERT_EQ(circuit, Circuit("MR 2 3 5\nMR 2 3 5\nMRX 1\nMRY 4"));

    params.before_measure_flip_probability = 0.125;
    params.after_reset_flip_probability = 0.25;
    circuit.clear();
    params.append_measure_reset(circuit, {2, 3, 5});
    params.append_measure_reset(circuit, {1}, 'X');
    params.append_measure_reset(circuit, {4}, 'Y');
    ASSERT_EQ(circuit, Circuit(R"CIRCUIT(
        X_ERROR(0.125) 2 3 5
        MR 2 3 5
        X_ERROR(0.25) 2 3 5
        Z_ERROR(0.125) 1
        MRX 1
        Z_ERROR(0.25) 1
        X_ERROR(0.125) 4
        MRY 4
        X_ERROR(0.25) 4
    )CIRCUIT"));
}
