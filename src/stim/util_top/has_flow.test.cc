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
