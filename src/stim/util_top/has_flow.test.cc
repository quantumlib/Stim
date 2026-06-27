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

#include "stim/util_top/has_flow.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(stabilizer_flow, sample_if_circuit_has_stabilizer_flows, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = sample_if_circuit_has_stabilizer_flows<W>(
        256,
        rng,
        Circuit(R"CIRCUIT(
            R 4
            CX 0 4 1 4 2 4 3 4
            M 4
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("Z___ -> Z____"),
            Flow<W>::from_str("_Z__ -> _Z__"),
            Flow<W>::from_str("__Z_ -> __Z_"),
            Flow<W>::from_str("___Z -> ___Z"),
            Flow<W>::from_str("XX__ -> XX__"),
            Flow<W>::from_str("XXXX -> XXXX"),
            Flow<W>::from_str("XYZ_ -> XYZ_"),
            Flow<W>::from_str("XXX_ -> XXX_"),
            Flow<W>::from_str("ZZZZ -> ____ xor rec[-1]"),
            Flow<W>::from_str("+___Z -> -___Z"),
            Flow<W>::from_str("-___Z -> -___Z"),
            Flow<W>::from_str("-___Z -> +___Z"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, sample_if_circuit_has_stabilizer_flows_signed_checked, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = sample_if_circuit_has_stabilizer_flows<W>(
        256,
        rng,
        Circuit(R"CIRCUIT(
            R 2 3
            X 1 3
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("Z0 -> Z0"),
            Flow<W>::from_str("Z1 -> -Z1"),
            Flow<W>::from_str("1 -> Z2"),
            Flow<W>::from_str("1 -> -Z3"),

            Flow<W>::from_str("Z0 -> -Z0"),
            Flow<W>::from_str("Z1 -> Z1"),
            Flow<W>::from_str("1 -> -Z2"),
            Flow<W>::from_str("1 -> Z3"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 1, 1, 0, 0, 0, 0}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, sample_if_circuit_has_stabilizer_flows_measurements_signed_checked, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = sample_if_circuit_has_stabilizer_flows<W>(
        256,
        rng,
        Circuit(R"CIRCUIT(
            X 1
            M 0 1 2
            X 2
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("Z0 -> Z0"),
            Flow<W>::from_str("Z1 -> -Z1"),
            Flow<W>::from_str("Z2 -> -Z2"),
            Flow<W>::from_str("Z0 -> rec[-3]"),
            Flow<W>::from_str("-Z1 -> rec[-2]"),
            Flow<W>::from_str("Z2 -> rec[-1]"),
            Flow<W>::from_str("1 -> Z0 xor rec[-3]"),
            Flow<W>::from_str("1 -> Z1 xor rec[-2]"),
            Flow<W>::from_str("1 -> -Z2 xor rec[-1]"),

            Flow<W>::from_str("Z0 -> -Z0"),
            Flow<W>::from_str("Z1 -> Z1"),
            Flow<W>::from_str("Z2 -> Z2"),
            Flow<W>::from_str("-Z0 -> rec[-3]"),
            Flow<W>::from_str("Z1 -> rec[-2]"),
            Flow<W>::from_str("-Z2 -> rec[-1]"),
            Flow<W>::from_str("1 -> -Z0 xor rec[-3]"),
            Flow<W>::from_str("1 -> -Z1 xor rec[-2]"),
            Flow<W>::from_str("1 -> Z2 xor rec[-1]"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, sample_if_circuit_has_stabilizer_flows_signed_obs, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = sample_if_circuit_has_stabilizer_flows<W>(
        256,
        rng,
        Circuit(R"CIRCUIT(
            X 1
            M 0 1 2
            X 2
            OBSERVABLE_INCLUDE(0) rec[-3]
            OBSERVABLE_INCLUDE(1) rec[-2]
            OBSERVABLE_INCLUDE(2) rec[-1]
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("Z0 -> Z0"),
            Flow<W>::from_str("Z1 -> -Z1"),
            Flow<W>::from_str("Z2 -> -Z2"),
            Flow<W>::from_str("Z0 -> obs[0]"),
            Flow<W>::from_str("-Z1 -> obs[1]"),
            Flow<W>::from_str("Z2 -> obs[2]"),
            Flow<W>::from_str("1 -> Z0 xor obs[0]"),
            Flow<W>::from_str("1 -> Z1 xor obs[1]"),
            Flow<W>::from_str("1 -> -Z2 xor obs[2]"),

            Flow<W>::from_str("Z0 -> -Z0"),
            Flow<W>::from_str("Z1 -> Z1"),
            Flow<W>::from_str("Z2 -> Z2"),
            Flow<W>::from_str("-Z0 -> obs[0]"),
            Flow<W>::from_str("Z1 -> obs[1]"),
            Flow<W>::from_str("-Z2 -> obs[2]"),
            Flow<W>::from_str("1 -> -Z0 xor obs[0]"),
            Flow<W>::from_str("1 -> -Z1 xor obs[1]"),
            Flow<W>::from_str("1 -> Z2 xor obs[2]"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, sample_if_circuit_has_stabilizer_flows_obs, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = sample_if_circuit_has_stabilizer_flows<W>(
        256,
        rng,
        Circuit(R"CIRCUIT(
            OBSERVABLE_INCLUDE(3) X0
            OBSERVABLE_INCLUDE(2) Y0
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("X0 -> obs[3]"),
            Flow<W>::from_str("Y0 -> obs[2]"),
            Flow<W>::from_str("-X0 -> obs[3]"),
            Flow<W>::from_str("X0 -> obs[2]"),
            Flow<W>::from_str("Y0 -> obs[3]"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 0, 0, 0}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, sample_if_circuit_has_stabilizer_flows_inverted_obs_pauli, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = sample_if_circuit_has_stabilizer_flows<W>(
        256,
        rng,
        Circuit(R"CIRCUIT(
            OBSERVABLE_INCLUDE(3) X0
            OBSERVABLE_INCLUDE(2) !X0
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("X0 -> obs[3]"),
            Flow<W>::from_str("-X0 -> obs[2]"),
            Flow<W>::from_str("-X0 -> obs[3]"),
            Flow<W>::from_str("X0 -> obs[2]"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 0, 0}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, sample_if_circuit_has_stabilizer_flows_inverted_obs_rec, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = sample_if_circuit_has_stabilizer_flows<W>(
        256,
        rng,
        Circuit(R"CIRCUIT(
            M !0
            OBSERVABLE_INCLUDE(3) rec[-1]
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("-Z0 -> obs[3]"),
            Flow<W>::from_str("Z0 -> obs[3]"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 0}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, check_if_circuit_has_unsigned_stabilizer_flows, {
    auto results = check_if_circuit_has_unsigned_stabilizer_flows<W>(
        Circuit(R"CIRCUIT(
            R 4
            CX 0 4 1 4 2 4 3 4
            M 4
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("Z___ -> Z____"),
            Flow<W>::from_str("_Z__ -> _Z__"),
            Flow<W>::from_str("__Z_ -> __Z_"),
            Flow<W>::from_str("___Z -> ___Z"),
            Flow<W>::from_str("XX__ -> XX__"),
            Flow<W>::from_str("XXXX -> XXXX"),
            Flow<W>::from_str("XYZ_ -> XYZ_"),
            Flow<W>::from_str("XXX_ -> XXX_"),
            Flow<W>::from_str("ZZZZ -> ____ xor rec[-1]"),
            Flow<W>::from_str("+___Z -> -___Z"),
            Flow<W>::from_str("-___Z -> -___Z"),
            Flow<W>::from_str("-___Z -> +___Z"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1}));
});

TEST_EACH_WORD_SIZE_W(stabilizer_flow, check_if_circuit_has_unsigned_stabilizer_flows_historical_failure, {
    auto results = check_if_circuit_has_unsigned_stabilizer_flows<W>(
        Circuit(R"CIRCUIT(
            CX 0 1
            S 0
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("X_ -> YX"),
            Flow<W>::from_str("Y_ -> XX"),
            Flow<W>::from_str("X_ -> XX"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 0}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, check_if_circuit_has_unsigned_stabilizer_flows_mzz, {
    auto results = check_if_circuit_has_unsigned_stabilizer_flows<W>(
        Circuit(R"CIRCUIT(
            MZZ 0 1
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("X0*X1 -> Y0*Y1 xor rec[-1]"),
            Flow<W>::from_str("X0*X1 -> Z0*Z1 xor rec[-1]"),
            Flow<W>::from_str("X0*X1 -> X0*X1"),
            Flow<W>::from_str("Z0 -> Z1 xor rec[-1]"),
            Flow<W>::from_str("Z0 -> Z0"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 0, 1, 1, 1}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, has_flow_observable, {
    auto results = check_if_circuit_has_unsigned_stabilizer_flows<W>(
        Circuit(R"CIRCUIT(
            MZZ 0 1
            OBSERVABLE_INCLUDE(2) rec[-1]
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("Z0*Z1 -> obs[2]"),
            Flow<W>::from_str("1 -> Z0*Z1 xor obs[2]"),
            Flow<W>::from_str("X0*X1 -> X0*X1 xor obs[0]"),
            Flow<W>::from_str("X0*X1 -> Y0*Y1 xor obs[2]"),
            Flow<W>::from_str("X0*X1 -> Y0*Y1 xor obs[1]"),
            Flow<W>::from_str("X0*X1 -> Y0*Y1 xor rec[-1]"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 1, 1, 0, 1}));

    results = check_if_circuit_has_unsigned_stabilizer_flows<W>(
        Circuit(R"CIRCUIT(
            OBSERVABLE_INCLUDE(3) X0 Y1 Z2
            OBSERVABLE_INCLUDE(2) Y0
        )CIRCUIT"),
        std::vector<Flow<W>>{
            Flow<W>::from_str("X0*Y1*Z2 -> obs[3]"),
            Flow<W>::from_str("-Y0 -> obs[2]"),
            Flow<W>::from_str("Y0 -> obs[3]"),
            Flow<W>::from_str("1 -> X0*Y1*Z2 xor obs[3]"),
        });
    ASSERT_EQ(results, (std::vector<bool>{1, 1, 0, 1}));
})
