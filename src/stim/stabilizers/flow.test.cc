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

#include "stim/stabilizers/flow.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_word.test.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(stabilizer_flow, from_str, {
    ASSERT_THROW({ Flow<W>::from_str(""); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X>X"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X-X"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X > X"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X - X"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("->X"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X->"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("rec[0] -> X"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X -> rec[ -1]"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X -> X rec[-1]"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X -> X xor"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X -> rec[-1] xor X"); }, std::invalid_argument);

    ASSERT_EQ(
        Flow<W>::from_str("1 -> 1"),
        (Flow<W>{
            .input = PauliString<W>::from_str(""),
            .output = PauliString<W>::from_str(""),
            .measurements = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("i -> -i"),
        (Flow<W>{
            .input = PauliString<W>::from_str(""),
            .output = PauliString<W>::from_str("-"),
            .measurements = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("iX -> -iY"),
        (Flow<W>{
            .input = PauliString<W>::from_str("X"),
            .output = PauliString<W>::from_str("-Y"),
            .measurements = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("X->-Y"),
        (Flow<W>{
            .input = PauliString<W>::from_str("X"),
            .output = PauliString<W>::from_str("-Y"),
            .measurements = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("X -> -Y"),
        (Flow<W>{
            .input = PauliString<W>::from_str("X"),
            .output = PauliString<W>::from_str("-Y"),
            .measurements = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("-X -> Y"),
        (Flow<W>{
            .input = PauliString<W>::from_str("-X"),
            .output = PauliString<W>::from_str("Y"),
            .measurements = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> -Z_Z"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("-Z_Z"),
            .measurements = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> Z_Y xor rec[-1]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {-1},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> Z_Y xor rec[5]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {5},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> rec[-1]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str(""),
            .measurements = {-1},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> Z_Y xor rec[-1] xor rec[-3]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {-1, -3},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> ZIY xor rec[55] xor rec[-3]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {55, -3},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("X9 -> -Z5*Y3 xor rec[55] xor rec[-3]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("_________X"),
            .output = PauliString<W>::from_str("-___Y_Z"),
            .measurements = {55, -3},
        }));
})

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

TEST_EACH_WORD_SIZE_W(stabilizer_flow, str_and_from_str, {
    auto flow = Flow<W>{
        PauliString<W>::from_str("XY"),
        PauliString<W>::from_str("_Z"),
        {-3},
    };
    auto s_dense = "XY -> _Z xor rec[-3]";
    auto s_sparse = "X0*Y1 -> Z1 xor rec[-3]";
    ASSERT_EQ(flow.str(), s_dense);
    ASSERT_EQ(Flow<W>::from_str(s_sparse), flow);
    ASSERT_EQ(Flow<W>::from_str(s_dense), flow);

    ASSERT_EQ(Flow<W>::from_str("1 -> rec[-1]"), (Flow<W>{PauliString<W>(0), PauliString<W>(0), {-1}}));
    ASSERT_EQ(Flow<W>::from_str("1 -> 1 xor rec[-1]"), (Flow<W>{PauliString<W>(0), PauliString<W>(0), {-1}}));
    ASSERT_EQ(
        Flow<W>::from_str("1 -> Z9 xor rec[55]"), (Flow<W>{PauliString<W>(0), PauliString<W>("_________Z"), {55}}));

    ASSERT_EQ(
        Flow<W>::from_str("-1 -> -X xor rec[-1] xor rec[-3]"),
        (Flow<W>{PauliString<W>::from_str("-"), PauliString<W>::from_str("-X"), {-1, -3}}));
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
