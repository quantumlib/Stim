#include "gtest/gtest.h"
#include "../src/bit_ptr.h"

TEST(bit_ptr, basic) {
    uint64_t word = 0;
    BitPtr b(&word, 5);
    ASSERT_EQ(b.get(), false);
    word = 1;
    ASSERT_EQ(b.get(), false);
    word = 32;
    ASSERT_EQ(b.get(), true);
    word = UINT64_MAX;
    b.set(false);
    ASSERT_EQ(word, UINT64_MAX - 32);
    word = 0;
    b.toggle();
    ASSERT_EQ(word, 32);
    b.toggle();
    ASSERT_EQ(word, 0);
    b.toggle();
    ASSERT_EQ(word, 32);
    b.toggle_if(false);
    ASSERT_EQ(word, 32);
    b.toggle_if(true);
    ASSERT_EQ(word, 0);
}
