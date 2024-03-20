#include "stim/util_top/circuit_inverse_qec.h"

#include "gtest/gtest.h"
#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, basic, {
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        H 0
        ISWAP 0 1 1 2 3 2
        S 0 3 4
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
        S_DAG 4 3 0
        ISWAP_DAG 3 2 1 2 0 1
        H 0
    )CIRCUIT"));
})
