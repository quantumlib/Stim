#include "stim/util_top/circuit_inverse_unitary.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(conversions, unitary_circuit_inverse) {
    ASSERT_EQ(
        unitary_circuit_inverse(Circuit(R"CIRCUIT(
        H 0
        ISWAP 0 1 1 2 3 2
        S 0 3 4
    )CIRCUIT")),
        Circuit(R"CIRCUIT(
        S_DAG 4 3 0
        ISWAP_DAG 3 2 1 2 0 1
        H 0
    )CIRCUIT"));

    ASSERT_THROW({ unitary_circuit_inverse(Circuit("M 0")); }, std::invalid_argument);
}
