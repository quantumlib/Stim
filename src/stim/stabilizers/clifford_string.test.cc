#include "stim/stabilizers/clifford_string.h"

#include "gtest/gtest.h"
#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(clifford_string, mul, {
    CliffordString<W> p1 = CliffordString<W>::uninitialized(5);
    FlexPauliString p2 = FlexPauliString::from_text("i__Z");
    ASSERT_EQ(p1 * p2, FlexPauliString::from_text("-XY_"));
}
