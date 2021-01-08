#include "gtest/gtest.h"
#include "bit_mat.h"

TEST(bit_mat, creation) {
    ASSERT_EQ(BitMat(3).str(),
            "...\n"
            "...\n"
            "...");

    ASSERT_EQ(BitMat::zero(3).str(),
            "...\n"
            "...\n"
            "...");

    ASSERT_EQ(BitMat::identity(3).str(),
            "1..\n"
            ".1.\n"
            "..1");

    ASSERT_EQ(BitMat::identity(2).str(),
            "1.\n"
            ".1");
}

TEST(bit_mat, equality) {
    ASSERT_EQ(BitMat(3), BitMat(3));
    ASSERT_NE(BitMat(3), BitMat(2));
    ASSERT_NE(BitMat(3), BitMat::identity(3));
    ASSERT_EQ(BitMat::identity(3), BitMat::identity(3));
}


TEST(bit_mat, multiplication) {
    BitMat m1(3);
    BitMat m2(3);
    m1.set(0, 2, true);
    m2.set(2, 1, true);
    auto m3 = m1 * m2;
    ASSERT_TRUE(m3.get(0, 1));
    ASSERT_EQ(m1.str(),
              "..1\n"
              "...\n"
              "...");
    ASSERT_EQ(m2.str(),
              "...\n"
              "...\n"
              ".1.");
    ASSERT_EQ(m3.str(),
              ".1.\n"
              "...\n"
              "...");
}

TEST(bit_mat, xor_row_into) {
    BitMat m(500);
    m.set(0, 10, true);
    m.set(0, 490, true);
    m.set(5, 490, true);
    m.xor_row_into(0, 1);
    m.xor_row_into(5, 1);
    ASSERT_EQ(m.get(1, 10), true);
    ASSERT_EQ(m.get(1, 490), false);
}

TEST(bit_mat, inv_lower_triangular) {
    auto m = BitMat::identity(4);
    m.set(3, 1, true);
    ASSERT_EQ(m.str(),
            "1...\n"
            ".1..\n"
            "..1.\n"
            ".1.1");
    auto inv = m.inv_lower_triangular();
    ASSERT_EQ(m * inv, BitMat::identity(4));
    ASSERT_EQ(inv * m, BitMat::identity(4));
    ASSERT_EQ(inv.str(),
            "1...\n"
            ".1..\n"
            "..1.\n"
            ".1.1");

    m.set(3, 0, true);
    ASSERT_EQ(m.str(),
            "1...\n"
            ".1..\n"
            "..1.\n"
            "11.1");
    inv = m.inv_lower_triangular();
    ASSERT_EQ(m * inv, BitMat::identity(4));
    ASSERT_EQ(inv * m, BitMat::identity(4));
    ASSERT_EQ(inv.str(),
            "1...\n"
            ".1..\n"
            "..1.\n"
            "11.1");

    m.set(1, 0, true);
    ASSERT_EQ(m.str(),
            "1...\n"
            "11..\n"
            "..1.\n"
            "11.1");
    inv = m.inv_lower_triangular();
    ASSERT_EQ(m * inv, BitMat::identity(4));
    ASSERT_EQ(inv * m, BitMat::identity(4));
    ASSERT_EQ(inv.str(),
            "1...\n"
            "11..\n"
            "..1.\n"
            ".1.1");
}

TEST(bit_mat, transposed) {
    auto m = BitMat::identity(4);
    m.set(3, 1, true);
    ASSERT_EQ(m.str(),
            "1...\n"
            ".1..\n"
            "..1.\n"
            ".1.1");
    auto trans = m.transposed();
    ASSERT_EQ(trans.str(),
            "1...\n"
            ".1.1\n"
            "..1.\n"
            "...1");
    ASSERT_EQ(trans.transposed(), m);
}

TEST(bit_mat, from_quadrants) {
    BitMat t(2);
    BitMat z(2);
    t.set(1, 0, true);
    ASSERT_EQ(BitMat::from_quadrants(t, z, z, z).str(),
            "....\n"
            "1...\n"
            "....\n"
            "....");
    ASSERT_EQ(BitMat::from_quadrants(z, t, z, z).str(),
            "....\n"
            "..1.\n"
            "....\n"
            "....");
    ASSERT_EQ(BitMat::from_quadrants(z, z, t, z).str(),
            "....\n"
            "....\n"
            "....\n"
            "1...");
    ASSERT_EQ(BitMat::from_quadrants(z, z, z, t).str(),
            "....\n"
            "....\n"
            "....\n"
            "..1.");
    ASSERT_EQ(BitMat::from_quadrants(t, t, t, t).str(),
            "....\n"
            "1.1.\n"
            "....\n"
            "1.1.");
}
