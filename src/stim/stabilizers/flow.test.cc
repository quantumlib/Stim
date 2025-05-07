#include "stim/stabilizers/flow.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(stabilizer_flow, from_str_dedupe, {
    EXPECT_EQ(
        Flow<W>::from_str(
            "X -> Y xor rec[-1] xor rec[-1] xor rec[-1] xor rec[-2] xor rec[-2] xor rec[-3] xor obs[1] xor obs[1] xor "
            "obs[3] xor obs[3] xor obs[3]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("X"),
            .output = PauliString<W>::from_str("Y"),
            .measurements = {-3, -1},
            .observables = {3},
        }));
})

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
    ASSERT_THROW({ Flow<W>::from_str("X -> obs[-1]"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X -> obs[A]"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X -> obs[]"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X -> obs[ 5]"); }, std::invalid_argument);
    ASSERT_THROW({ Flow<W>::from_str("X -> rec[]"); }, std::invalid_argument);

    ASSERT_EQ(
        Flow<W>::from_str("1 -> 1"),
        (Flow<W>{
            .input = PauliString<W>::from_str(""),
            .output = PauliString<W>::from_str(""),
            .measurements = {},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("1 -> -rec[0]"),
        (Flow<W>{
            .input = PauliString<W>::from_str(""),
            .output = PauliString<W>::from_str("-"),
            .measurements = {0},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("i -> -i"),
        (Flow<W>{
            .input = PauliString<W>::from_str(""),
            .output = PauliString<W>::from_str("-"),
            .measurements = {},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("iX -> -iY"),
        (Flow<W>{
            .input = PauliString<W>::from_str("X"),
            .output = PauliString<W>::from_str("-Y"),
            .measurements = {},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("X->-Y"),
        (Flow<W>{
            .input = PauliString<W>::from_str("X"),
            .output = PauliString<W>::from_str("-Y"),
            .measurements = {},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("X -> -Y"),
        (Flow<W>{
            .input = PauliString<W>::from_str("X"),
            .output = PauliString<W>::from_str("-Y"),
            .measurements = {},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("-X -> Y"),
        (Flow<W>{
            .input = PauliString<W>::from_str("-X"),
            .output = PauliString<W>::from_str("Y"),
            .measurements = {},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> -Z_Z"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("-Z_Z"),
            .measurements = {},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> Z_Y xor rec[-1]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {-1},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> Z_Y xor rec[5]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {5},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> rec[-1]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str(""),
            .measurements = {-1},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> Z_Y xor rec[-1] xor rec[-3]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {-3, -1},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> ZIY xor rec[55] xor rec[-3]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {-3, 55},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("XYZ -> ZIY xor rec[-3] xor rec[55]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("XYZ"),
            .output = PauliString<W>::from_str("Z_Y"),
            .measurements = {-3, 55},
            .observables = {},
        }));
    ASSERT_EQ(
        Flow<W>::from_str("X9 -> -Z5*Y3 xor rec[55] xor rec[-3]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("_________X"),
            .output = PauliString<W>::from_str("-___Y_Z"),
            .measurements = {-3, 55},
            .observables = {},
        }));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, from_str_obs, {
    EXPECT_EQ(
        Flow<W>::from_str("X9 -> obs[5]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("_________X"),
            .output = PauliString<W>::from_str(""),
            .measurements = {},
            .observables = {5},
        }));
    EXPECT_EQ(
        Flow<W>::from_str("X9 -> X xor obs[5] xor obs[3] xor rec[-1]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("_________X"),
            .output = PauliString<W>::from_str("X"),
            .measurements = {-1},
            .observables = {3, 5},
        }));
    EXPECT_EQ(
        Flow<W>::from_str("X9 -> X xor obs[5] xor rec[-1] xor obs[3]"),
        (Flow<W>{
            .input = PauliString<W>::from_str("_________X"),
            .output = PauliString<W>::from_str("X"),
            .measurements = {-1},
            .observables = {3, 5},
        }));
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
        (Flow<W>{PauliString<W>::from_str("-"), PauliString<W>::from_str("-X"), {-3, -1}}));
})

TEST_EACH_WORD_SIZE_W(stabilizer_flow, ordering, {
    ASSERT_FALSE(Flow<W>::from_str("1 -> 1") < Flow<W>::from_str("1 -> 1"));
    ASSERT_FALSE(Flow<W>::from_str("X -> 1") < Flow<W>::from_str("1 -> 1"));
    ASSERT_FALSE(Flow<W>::from_str("1 -> X") < Flow<W>::from_str("1 -> 1"));
    ASSERT_FALSE(Flow<W>::from_str("1 -> rec[-1]") < Flow<W>::from_str("1 -> 1"));
    ASSERT_TRUE(Flow<W>::from_str("1 -> 1") < Flow<W>::from_str("X -> 1"));
    ASSERT_TRUE(Flow<W>::from_str("1 -> 1") < Flow<W>::from_str("1 -> X"));
    ASSERT_TRUE(Flow<W>::from_str("1 -> 1") < Flow<W>::from_str("1 -> rec[-1]"));
})
