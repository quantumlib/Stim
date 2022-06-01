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

#include "stim/mem/simd_bit_table.h"

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(simd_bit_table, creation) {
    simd_bit_table a(3, 3);
    ASSERT_EQ(
        a.str(3),
        "...\n"
        "...\n"
        "...");
    a[1][2] = 1;
    ASSERT_EQ(
        a.str(3),
        "...\n"
        "..1\n"
        "...");

    ASSERT_EQ(
        simd_bit_table::identity(3).str(3),
        "1..\n"
        ".1.\n"
        "..1");

    ASSERT_EQ(
        simd_bit_table::identity(2).str(2),
        "1.\n"
        ".1");

    ASSERT_EQ(
        simd_bit_table::identity(2).str(3, 4),
        "1...\n"
        ".1..\n"
        "....");

    ASSERT_EQ(simd_bit_table::identity(2).str().substr(0, 5), "1....");
    ASSERT_EQ(simd_bit_table(0, 5).str(), "");
    ASSERT_EQ(simd_bit_table(5, 0).str().substr(0, 3), "\n\n\n");

    ASSERT_EQ(
        simd_bit_table::from_text(R"TABLE(
            1.
            0._1
            .1..
        )TABLE")
            .str(5),
        "1....\n"
        "...1.\n"
        ".1...\n"
        ".....\n"
        ".....");

    simd_bit_table t = simd_bit_table::from_text("", 512, 256);
    ASSERT_EQ(t.num_minor_bits_padded(), 256);
    ASSERT_EQ(t.num_major_bits_padded(), 512);
}

TEST(simd_bit_table, equality) {
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

TEST(simd_bit_table, multiplication) {
    simd_bit_table m1(3, 3);
    simd_bit_table m2(3, 3);
    m1[0][2] = true;
    m2[2][1] = true;
    auto m3 = m1.square_mat_mul(m2, 3);
    ASSERT_TRUE(m3[0][1]);
    ASSERT_EQ(
        m1.str(3),
        "..1\n"
        "...\n"
        "...");
    ASSERT_EQ(
        m2.str(3),
        "...\n"
        "...\n"
        ".1.");
    ASSERT_EQ(
        m3.str(3),
        ".1.\n"
        "...\n"
        "...");
}

TEST(simd_bit_table, xor_row_into) {
    simd_bit_table m(500, 500);
    m[0][10] = true;
    m[0][490] = true;
    m[5][490] = true;
    m[1] ^= m[0];
    m[1] ^= m[5];
    ASSERT_EQ(m[1][10], true);
    ASSERT_EQ(m[1][490], false);
}

TEST(simd_bit_table, inverse_assuming_lower_triangular) {
    auto m = simd_bit_table::identity(4);
    m[3][1] = true;
    ASSERT_EQ(
        m.str(4),
        "1...\n"
        ".1..\n"
        "..1.\n"
        ".1.1");
    auto inv = m.inverse_assuming_lower_triangular(4);
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table::identity(4));
    ASSERT_EQ(
        inv.str(4),
        "1...\n"
        ".1..\n"
        "..1.\n"
        ".1.1");

    m[3][0] = true;
    ASSERT_EQ(
        m.str(4),
        "1...\n"
        ".1..\n"
        "..1.\n"
        "11.1");
    inv = m.inverse_assuming_lower_triangular(4);
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table::identity(4));
    ASSERT_EQ(
        inv.str(4),
        "1...\n"
        ".1..\n"
        "..1.\n"
        "11.1");

    m[1][0] = true;
    ASSERT_EQ(
        m.str(4),
        "1...\n"
        "11..\n"
        "..1.\n"
        "11.1");
    inv = m.inverse_assuming_lower_triangular(4);
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table::identity(4));
    ASSERT_EQ(
        inv.str(4),
        "1...\n"
        "11..\n"
        "..1.\n"
        ".1.1");
}

TEST(simd_bit_table, transposed) {
    auto m = simd_bit_table::identity(4);
    m[3][1] = true;
    ASSERT_EQ(
        m.str(4),
        "1...\n"
        ".1..\n"
        "..1.\n"
        ".1.1");
    auto trans = m;
    trans.do_square_transpose();
    ASSERT_EQ(
        trans.str(4),
        "1...\n"
        ".1.1\n"
        "..1.\n"
        "...1");
    auto trans2 = trans;
    trans2.do_square_transpose();
    ASSERT_EQ(trans2, m);
}

TEST(simd_bit_table, random) {
    auto t = simd_bit_table::random(100, 90, SHARED_TEST_RNG());
    ASSERT_NE(t[99], simd_bits(90));
    ASSERT_EQ(t[100], simd_bits(90));
    t = t.transposed();
    ASSERT_NE(t[89], simd_bits(100));
    ASSERT_EQ(t[90], simd_bits(100));
    ASSERT_NE(simd_bit_table::random(10, 10, SHARED_TEST_RNG()), simd_bit_table::random(10, 10, SHARED_TEST_RNG()));
}

TEST(simd_bit_table, slice_maj) {
    auto m = simd_bit_table::random(100, 64, SHARED_TEST_RNG());
    auto s = m.slice_maj(5, 15);
    ASSERT_EQ(s[0], m[5]);
    ASSERT_EQ(s[9], m[14]);
    ASSERT_FALSE(s[10].not_zero());
}

TEST(simd_bit_table, from_quadrants) {
    simd_bit_table t(2, 2);
    simd_bit_table z(2, 2);
    t[1][0] = true;
    ASSERT_EQ(
        simd_bit_table::from_quadrants(2, t, z, z, z).str(4),
        "....\n"
        "1...\n"
        "....\n"
        "....");
    ASSERT_EQ(
        simd_bit_table::from_quadrants(2, z, t, z, z).str(4),
        "....\n"
        "..1.\n"
        "....\n"
        "....");
    ASSERT_EQ(
        simd_bit_table::from_quadrants(2, z, z, t, z).str(4),
        "....\n"
        "....\n"
        "....\n"
        "1...");
    ASSERT_EQ(
        simd_bit_table::from_quadrants(2, z, z, z, t).str(4),
        "....\n"
        "....\n"
        "....\n"
        "..1.");
    ASSERT_EQ(
        simd_bit_table::from_quadrants(2, t, t, t, t).str(4),
        "....\n"
        "1.1.\n"
        "....\n"
        "1.1.");
}

TEST(simd_bit_table, lg) {
    ASSERT_EQ(lg(0), 0);
    ASSERT_EQ(lg(1), 0);
    ASSERT_EQ(lg(2), 1);
    ASSERT_EQ(lg(3), 1);
    ASSERT_EQ(lg(4), 2);
    ASSERT_EQ(lg(5), 2);
    ASSERT_EQ(lg(6), 2);
    ASSERT_EQ(lg(7), 2);
    ASSERT_EQ(lg(8), 3);
    ASSERT_EQ(lg(9), 3);
}
