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

#include "stim/simulators/vector_simulator.h"

#include "gtest/gtest.h"

#include "stim/circuit/gate_data.h"

using namespace stim;

static float complex_distance(std::complex<float> a, std::complex<float> b) {
    auto d = a - b;
    return sqrtf(d.real() * d.real() + d.imag() * d.imag());
}

#define ASSERT_NEAR_C(a, b) ASSERT_LE(complex_distance(a, b), 1e-4)

TEST(vector_sim, qubit_order) {
    VectorSimulator sim(2);
    sim.apply("H_XZ", 0);
    sim.apply("ZCX", 0, 1);
    ASSERT_NEAR_C(sim.state[0], sqrtf(0.5));
    ASSERT_NEAR_C(sim.state[1], 0);
    ASSERT_NEAR_C(sim.state[2], 0);
    ASSERT_NEAR_C(sim.state[3], sqrtf(0.5));
}

TEST(vector_sim, h_squared) {
    VectorSimulator sim(1);
    sim.apply("H_XZ", 0);
    sim.apply("H_XZ", 0);
    ASSERT_NEAR_C(sim.state[0], 1);
    ASSERT_NEAR_C(sim.state[1], 0);
}

TEST(vector_sim, sqrt_x_squared) {
    VectorSimulator sim(1);
    sim.apply("SQRT_X_DAG", 0);
    sim.apply("SQRT_X_DAG", 0);
    ASSERT_NEAR_C(sim.state[0], 0);
    ASSERT_NEAR_C(sim.state[1], 1);
}

TEST(vector_sim, state_channel_duality_cnot) {
    VectorSimulator sim(4);
    sim.apply("H_XZ", 0);
    sim.apply("H_XZ", 1);
    sim.apply("ZCX", 0, 2);
    sim.apply("ZCX", 1, 3);
    sim.apply("ZCX", 2, 3);
    auto u = GATE_DATA.at("ZCX").unitary();
    for (size_t row = 0; row < 4; row++) {
        for (size_t col = 0; col < 4; col++) {
            ASSERT_NEAR_C(sim.state[row * 4 + col], u[row][col] * 0.5f);
        }
    }
}

TEST(vector_sim, state_channel_duality_y) {
    VectorSimulator sim(2);
    sim.apply("H_XZ", 0);
    sim.apply("ZCX", 0, 1);
    sim.apply("Y", 1);
    auto u = GATE_DATA.at("Y").unitary();
    for (size_t row = 0; row < 2; row++) {
        for (size_t col = 0; col < 2; col++) {
            ASSERT_NEAR_C(sim.state[row * 2 + col], u[row][col] * sqrtf(0.5f));
        }
    }
}

TEST(vector_sim, apply_pauli) {
    VectorSimulator sim(2);

    sim.apply(PauliString::from_str("+II").ref(), 0);
    ASSERT_NEAR_C(sim.state[0], 1);
    ASSERT_NEAR_C(sim.state[1], 0);
    ASSERT_NEAR_C(sim.state[2], 0);
    ASSERT_NEAR_C(sim.state[3], 0);

    sim.apply(PauliString::from_str("-II").ref(), 0);
    ASSERT_NEAR_C(sim.state[0], -1);
    ASSERT_NEAR_C(sim.state[1], 0);
    ASSERT_NEAR_C(sim.state[2], 0);
    ASSERT_NEAR_C(sim.state[3], 0);

    sim.apply(PauliString::from_str("+XI").ref(), 0);
    ASSERT_NEAR_C(sim.state[0], 0);
    ASSERT_NEAR_C(sim.state[1], -1);
    ASSERT_NEAR_C(sim.state[2], 0);
    ASSERT_NEAR_C(sim.state[3], 0);

    sim.apply(PauliString::from_str("+IZ").ref(), 0);
    ASSERT_NEAR_C(sim.state[0], 0);
    ASSERT_NEAR_C(sim.state[1], -1);
    ASSERT_NEAR_C(sim.state[2], 0);
    ASSERT_NEAR_C(sim.state[3], 0);

    sim.apply(PauliString::from_str("+ZI").ref(), 0);
    ASSERT_NEAR_C(sim.state[0], 0);
    ASSERT_NEAR_C(sim.state[1], 1);
    ASSERT_NEAR_C(sim.state[2], 0);
    ASSERT_NEAR_C(sim.state[3], 0);

    sim.apply(PauliString::from_str("+IY").ref(), 0);
    ASSERT_NEAR_C(sim.state[0], 0);
    ASSERT_NEAR_C(sim.state[1], 0);
    ASSERT_NEAR_C(sim.state[2], 0);
    ASSERT_NEAR_C(sim.state[3], std::complex<float>(0, 1));

    sim.apply(PauliString::from_str("+XX").ref(), 0);
    ASSERT_NEAR_C(sim.state[0], std::complex<float>(0, 1));
    ASSERT_NEAR_C(sim.state[1], 0);
    ASSERT_NEAR_C(sim.state[2], 0);
    ASSERT_NEAR_C(sim.state[3], 0);

    sim.apply(PauliString::from_str("+X").ref(), 1);
    ASSERT_NEAR_C(sim.state[0], 0);
    ASSERT_NEAR_C(sim.state[1], 0);
    ASSERT_NEAR_C(sim.state[2], std::complex<float>(0, 1));
    ASSERT_NEAR_C(sim.state[3], 0);
}

TEST(vector_sim, approximate_equals) {
    VectorSimulator s1(2);
    VectorSimulator s2(2);
    ASSERT_TRUE(s1.approximate_equals(s2));
    ASSERT_TRUE(s1.approximate_equals(s2, false));
    ASSERT_TRUE(s1.approximate_equals(s2, true));
    s1.state[0] *= -1;
    ASSERT_FALSE(s1.approximate_equals(s2));
    ASSERT_FALSE(s1.approximate_equals(s2, false));
    ASSERT_TRUE(s1.approximate_equals(s2, true));
    s1.state[0] *= std::complex<float>(0, 1);
    ASSERT_FALSE(s1.approximate_equals(s2));
    ASSERT_FALSE(s2.approximate_equals(s1));
    ASSERT_FALSE(s1.approximate_equals(s2, false));
    ASSERT_TRUE(s1.approximate_equals(s2, true));
    ASSERT_FALSE(s2.approximate_equals(s1, false));
    ASSERT_TRUE(s2.approximate_equals(s1, true));
    s1.state[0] = 0;
    s1.state[1] = 1;
    ASSERT_FALSE(s1.approximate_equals(s2));
    ASSERT_FALSE(s1.approximate_equals(s2, false));
    ASSERT_FALSE(s1.approximate_equals(s2, true));
    s2.state[0] = 0;
    s2.state[1] = 1;
    ASSERT_TRUE(s1.approximate_equals(s2));
    s1.state[0] = sqrtf(0.5);
    s1.state[1] = sqrtf(0.5);
    s2.state[0] = sqrtf(0.5);
    s2.state[1] = sqrtf(0.5);
    ASSERT_TRUE(s1.approximate_equals(s2));
    s1.state[0] *= -1;
    ASSERT_FALSE(s1.approximate_equals(s2));
}

TEST(vector_sim, project_empty) {
    VectorSimulator sim(0);
    sim.project(PauliString(0));
    VectorSimulator ref(0);
    ref.state = {1};
    ASSERT_TRUE(sim.approximate_equals(ref, true));
}

TEST(vector_sim, project) {
    VectorSimulator sim(2);
    VectorSimulator ref(2);

    sim.state = {0.5, 0.5, 0.5, 0.5};
    ASSERT_NEAR_C(sim.project(PauliString::from_str("ZI")), 0.5);
    ref.state = {sqrtf(0.5), 0, sqrtf(0.5), 0};
    ASSERT_TRUE(sim.approximate_equals(ref));
    ASSERT_NEAR_C(sim.project(PauliString::from_str("ZI")), 1);
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.state = {0.5, 0.5, 0.5, 0.5};
    sim.project(PauliString::from_str("-ZI"));
    ref.state = {0, sqrtf(0.5), 0, sqrtf(0.5)};
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.state = {0.5, 0.5, 0.5, 0.5};
    sim.project(PauliString::from_str("IZ"));
    ref.state = {sqrtf(0.5), sqrtf(0.5), 0, 0};
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.state = {0.5, 0.5, 0.5, 0.5};
    sim.project(PauliString::from_str("-IZ"));
    ref.state = {0, 0, sqrtf(0.5), sqrtf(0.5)};
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.state = {0.5, 0.5, 0.5, 0.5};
    sim.project(PauliString::from_str("ZZ"));
    ref.state = {sqrtf(0.5), 0, 0, sqrtf(0.5)};
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.state = {0.5, 0.5, 0.5, 0.5};
    sim.project(PauliString::from_str("-ZZ"));
    ref.state = {0, sqrtf(0.5), sqrtf(0.5), 0};
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.project(PauliString::from_str("ZI"));
    sim.state = {1, 0, 0, 0};
    sim.project(PauliString::from_str("ZZ"));
    ref.state = {1, 0, 0, 0};
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.project(PauliString::from_str("XX"));
    ref.state = {sqrtf(0.5f), 0, 0, sqrtf(0.5f)};
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.project(PauliString::from_str("-YZ"));
    ref.state = {0.5, {0, -0.5}, {0, -0.5}, 0.5};
    ASSERT_TRUE(sim.approximate_equals(ref));

    sim.project(PauliString::from_str("-ZI"));
    ref.state = {0, {0, -sqrtf(0.5)}, 0, sqrtf(0.5)};
    ASSERT_TRUE(sim.approximate_equals(ref));
}

TEST(VectorSim, from_stabilizers) {
    VectorSimulator ref(2);
    auto sim = VectorSimulator::from_stabilizers({PauliString::from_str("ZI"), PauliString::from_str("IZ")});
    ref.state = {1, 0, 0, 0};
    ASSERT_TRUE(sim.approximate_equals(ref, true));

    sim = VectorSimulator::from_stabilizers({PauliString::from_str("-YX"), PauliString::from_str("ZZ")});
    ref.state = {sqrtf(0.5), 0, 0, {0, -sqrtf(0.5)}};
    ASSERT_TRUE(sim.approximate_equals(ref, true));

    sim = VectorSimulator::from_stabilizers({PauliString::from_str("ZI"), PauliString::from_str("ZZ")});
    ref.state = {1, 0, 0, 0};
    ASSERT_TRUE(sim.approximate_equals(ref, true));

    sim = VectorSimulator::from_stabilizers({PauliString::from_str("ZI"), PauliString::from_str("-ZZ")});
    ref.state = {0, 0, 1, 0};
    ASSERT_TRUE(sim.approximate_equals(ref, true));

    sim = VectorSimulator::from_stabilizers({PauliString::from_str("ZI"), PauliString::from_str("IX")});
    ref.state = {sqrtf(0.5), 0, sqrtf(0.5), 0};
    ASSERT_TRUE(sim.approximate_equals(ref, true));

    sim = VectorSimulator::from_stabilizers({PauliString::from_str("ZZ"), PauliString::from_str("XX")});
    ref.state = {sqrtf(0.5), 0, 0, sqrtf(0.5)};
    ASSERT_TRUE(sim.approximate_equals(ref, true));

    sim = VectorSimulator::from_stabilizers(
        {PauliString::from_str("XXX"), PauliString::from_str("ZZI"), PauliString::from_str("IZZ")});
    ref.state = {sqrtf(0.5), 0, 0, 0, 0, 0, 0, sqrtf(0.5)};
    ASSERT_TRUE(sim.approximate_equals(ref, true));

    sim = VectorSimulator::from_stabilizers(
        {PauliString::from_str("YYY"), PauliString::from_str("ZZI"), PauliString::from_str("IZZ")});
    ref.state = {sqrtf(0.5), 0, 0, 0, 0, 0, 0, {0, -sqrtf(0.5)}};
    ASSERT_TRUE(sim.approximate_equals(ref, true));

    sim = VectorSimulator::from_stabilizers(
        {PauliString::from_str("-YYY"), PauliString::from_str("-ZZI"), PauliString::from_str("IZZ")});
    ref.state = {0, sqrtf(0.5), 0, 0, 0, 0, {0, -sqrtf(0.5)}, 0};
    ASSERT_TRUE(sim.approximate_equals(ref, true));
}

TEST(VectorSim, smooth_stabilizer_state) {
    VectorSimulator sim(0);
    sim.state = {{1, 0}, {1, 1}};
    ASSERT_THROW({ sim.smooth_stabilizer_state(1); }, std::invalid_argument);

    sim.state = {{0.25, 0.25}, {-0.25, 0.25}};
    ASSERT_THROW({ sim.smooth_stabilizer_state(1); }, std::invalid_argument);

    sim.state = {{0.25, 0.25}, {-0.25, 0.25}};
    sim.smooth_stabilizer_state({-0.25, -0.25});
    ASSERT_EQ(sim.state, (std::vector<std::complex<float>>{{-1, 0}, {0, -1}}));
}

TEST(VectorSim, do_unitary_circuit) {
    VectorSimulator sim(3);
    sim.do_unitary_circuit(Circuit(R"CIRCUIT(
        H 0 1
        S 0
    )CIRCUIT"));
    sim.smooth_stabilizer_state(0.5);
    ASSERT_EQ(sim.state, (std::vector<std::complex<float>>({1, {0, 1}, 1, {0, 1}, 0, 0, 0, 0})));

    sim.do_unitary_circuit(Circuit(R"CIRCUIT(
        CNOT 0 2 2 0 0 2
    )CIRCUIT"));
    ASSERT_EQ(sim.state, (std::vector<std::complex<float>>({1, 0, 1, 0, {0, 1}, 0, {0, 1}, 0})));

    ASSERT_THROW({ sim.do_unitary_circuit(Circuit("H 3")); }, std::invalid_argument);
    ASSERT_THROW({ sim.do_unitary_circuit(Circuit("CX rec[-1] 0")); }, std::invalid_argument);
    ASSERT_THROW({ sim.do_unitary_circuit(Circuit("X_ERROR(0.1) 0")); }, std::invalid_argument);
}
