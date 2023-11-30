#include "stim/stabilizers/flex_pauli_string.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(flex_pauli_string, mul) {
    FlexPauliString p1 = FlexPauliString::from_text("iXYZ");
    FlexPauliString p2 = FlexPauliString::from_text("i__Z");
    ASSERT_EQ(p1 * p2, FlexPauliString::from_text("-XY_"));
}
