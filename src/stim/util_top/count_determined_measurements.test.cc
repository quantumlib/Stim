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

#include "stim/util_top/count_determined_measurements.h"

#include "gtest/gtest.h"

#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(count_determined_measurements, single_qubit_measurements_baseline, {
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MX 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MRX 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RZ 0
        MX 0
    )CIRCUIT")),
        0);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RZ 0
        MRX 0
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RY 0
        MY 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RY 0
        MRY 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MY 0
    )CIRCUIT")),
        0);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MRY 0
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RZ 0
        MZ 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RZ 0
        MRZ 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MZ 0
    )CIRCUIT")),
        0);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MRZ 0
    )CIRCUIT")),
        0);
})

TEST_EACH_WORD_SIZE_W(count_determined_measurements, pair_measurements_baseline, {
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0 1
        MXX 0 1
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RY 0 1
        MXX 0 1
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RY 0 1
        MYY 0 1
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0 1
        MYY 0 1
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RZ 0 1
        MZZ 0 1
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RY 0 1
        MZZ 0 1
    )CIRCUIT")),
        0);
})

TEST_EACH_WORD_SIZE_W(count_determined_measurements, mpp_baseline, {
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MPP X0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RY 0
        MPP X0
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RY 0
        MPP Y0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MPP Y0
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RZ 0
        MPP Z0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MPP Z0
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        RY 1
        RZ 2
        MPP X0*Y1*Z2
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        RX 1
        RZ 2
        MPP X0*Y1*Z2
    )CIRCUIT")),
        0);
})

TEST_EACH_WORD_SIZE_W(count_determined_measurements, converge, {
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MX 0 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MY 0 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MZ 0 0
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MRX 0 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MRY 0 0
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0
        MRZ 0 0
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MXX 0 1 0 1
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MYY 0 1 0 1
    )CIRCUIT")),
        1);
    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0 1
        MZZ 0 1 0 1
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MXX 0 1
        MYY 0 1
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MPP X0*X1 Y0*Y1
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MPP X0*X1 X1*X2 !X0*X2
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        REPEAT 3 {
            MPP X0*X1
        }
    )CIRCUIT")),
        2);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MXX 0 1
        MX 0 1
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        MYY 0 1
        MY 0 1
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        count_determined_measurements<W>(Circuit(R"CIRCUIT(
        RX 0 1
        MZZ 0 1
        MZ 0 1
    )CIRCUIT")),
        1);
})

TEST_EACH_WORD_SIZE_W(count_determined_measurements, surface_code, {
    CircuitGenParameters params(7, 5, "rotated_memory_x");
    params.after_clifford_depolarization = 0.01;
    params.before_measure_flip_probability = 0;
    params.after_reset_flip_probability = 0;
    params.before_round_data_depolarization = 0;
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto actual = count_determined_measurements<W>(circuit);
    ASSERT_EQ(actual, circuit.count_detectors() + circuit.count_observables());
})
