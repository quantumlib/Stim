#include <gtest/gtest.h>
#include "simd_bit_table.h"

TEST(bit_mat, creation) {
    simd_bit_table a(3, 3);
    ASSERT_EQ(a.str(3),
            "...\n"
            "...\n"
            "...");
    a[1][2] = 1;
    ASSERT_EQ(a.str(3),
            "...\n"
            "..1\n"
            "...");

    ASSERT_EQ(simd_bit_table::identity(3).str(3),
            "1..\n"
            ".1.\n"
            "..1");

    ASSERT_EQ(simd_bit_table::identity(2).str(2),
            "1.\n"
            ".1");
}

TEST(bit_mat, equality) {
    simd_bit_table a(3, 3);
    simd_bit_table b(3, 3);
    simd_bit_table c(511, 1000);
    simd_bit_table d(511, 1000);
    ASSERT_EQ(a, b);
    ASSERT_EQ(c, d);
    ASSERT_NE(a, c);
    a[0][1] = 1;
    ASSERT_NE(a, b);
    d[500][900] = 1;
    ASSERT_NE(c, d);
}

TEST(bit_mat, multiplication) {
    simd_bit_table m1(3, 3);
    simd_bit_table m2(3, 3);
    m1[0][2] = true;
    m2[2][1] = true;
    auto m3 = m1.square_mat_mul(m2, 3);
    ASSERT_TRUE(m3[0][1]);
    ASSERT_EQ(m1.str(3),
              "..1\n"
              "...\n"
              "...");
    ASSERT_EQ(m2.str(3),
              "...\n"
              "...\n"
              ".1.");
    ASSERT_EQ(m3.str(3),
              ".1.\n"
              "...\n"
              "...");
}

TEST(bit_mat, xor_row_into) {
    simd_bit_table m(500, 500);
    m[0][10] = true;
    m[0][490] = true;
    m[5][490] = true;
    m[1] ^= m[0];
    m[1] ^= m[5];
    ASSERT_EQ(m[1][10], true);
    ASSERT_EQ(m[1][490], false);
}

TEST(bit_mat, inverse_assuming_lower_triangular) {
    auto m = simd_bit_table::identity(4);
    m[3][1] = true;
    ASSERT_EQ(m.str(4),
            "1...\n"
            ".1..\n"
            "..1.\n"
            ".1.1");
    auto inv = m.inverse_assuming_lower_triangular(4);
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.str(4),
            "1...\n"
            ".1..\n"
            "..1.\n"
            ".1.1");

    m[3][0] = true;
    ASSERT_EQ(m.str(4),
            "1...\n"
            ".1..\n"
            "..1.\n"
            "11.1");
    inv = m.inverse_assuming_lower_triangular(4);
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.str(4),
            "1...\n"
            ".1..\n"
            "..1.\n"
            "11.1");

    m[1][0] = true;
    ASSERT_EQ(m.str(4),
            "1...\n"
            "11..\n"
            "..1.\n"
            "11.1");
    inv = m.inverse_assuming_lower_triangular(4);
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.str(4),
            "1...\n"
            "11..\n"
            "..1.\n"
            ".1.1");
}

TEST(bit_mat, transposed) {
    auto m = simd_bit_table::identity(4);
    m[3][1] = true;
    ASSERT_EQ(m.str(4),
            "1...\n"
            ".1..\n"
            "..1.\n"
            ".1.1");
    auto trans = m;
    trans.do_square_transpose();
    ASSERT_EQ(trans.str(4),
            "1...\n"
            ".1.1\n"
            "..1.\n"
            "...1");
    auto trans2 = trans;
    trans2.do_square_transpose();
    ASSERT_EQ(trans2, m);
}

TEST(bit_mat, from_quadrants) {
    simd_bit_table t(2, 2);
    simd_bit_table z(2, 2);
    t[1][0] = true;
    ASSERT_EQ(simd_bit_table::from_quadrants(2, t, z, z, z).str(4),
            "....\n"
            "1...\n"
            "....\n"
            "....");
    ASSERT_EQ(simd_bit_table::from_quadrants(2, z, t, z, z).str(4),
            "....\n"
            "..1.\n"
            "....\n"
            "....");
    ASSERT_EQ(simd_bit_table::from_quadrants(2, z, z, t, z).str(4),
            "....\n"
            "....\n"
            "....\n"
            "1...");
    ASSERT_EQ(simd_bit_table::from_quadrants(2, z, z, z, t).str(4),
            "....\n"
            "....\n"
            "....\n"
            "..1.");
    ASSERT_EQ(simd_bit_table::from_quadrants(2, t, t, t, t).str(4),
            "....\n"
            "1.1.\n"
            "....\n"
            "1.1.");
}
