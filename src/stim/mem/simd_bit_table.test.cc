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

#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(simd_bit_table, creation, {
    simd_bit_table<W> a(3, 3);
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
        simd_bit_table<W>::identity(3).str(3),
        "1..\n"
        ".1.\n"
        "..1");

    ASSERT_EQ(
        simd_bit_table<W>::identity(2).str(2),
        "1.\n"
        ".1");

    ASSERT_EQ(
        simd_bit_table<W>::identity(2).str(3, 4),
        "1...\n"
        ".1..\n"
        "....");

    ASSERT_EQ(simd_bit_table<W>::identity(2).str().substr(0, 5), "1....");
    ASSERT_EQ(simd_bit_table<W>(0, 5).str(), "");
    ASSERT_EQ(simd_bit_table<W>(5, 0).str().substr(0, 3), "\n\n\n");

    ASSERT_EQ(
        simd_bit_table<W>::from_text(R"TABLE(
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

    simd_bit_table<W> t = simd_bit_table<W>::from_text("", 512, 256);
    ASSERT_EQ(t.num_minor_bits_padded(), 256);
    ASSERT_EQ(t.num_major_bits_padded(), 512);
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, equality, {
    simd_bit_table<W> a(3, 3);
    simd_bit_table<W> b(3, 3);
    simd_bit_table<W> c(511, 1000);
    simd_bit_table<W> d(511, 1000);
    ASSERT_EQ(a, b);
    ASSERT_EQ(c, d);
    ASSERT_NE(a, c);
    a[0][1] = 1;
    ASSERT_NE(a, b);
    d[500][900] = 1;
    ASSERT_NE(c, d);
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, multiplication, {
    simd_bit_table<W> m1(3, 3);
    simd_bit_table<W> m2(3, 3);
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
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, xor_row_into, {
    simd_bit_table<W> m(500, 500);
    m[0][10] = true;
    m[0][490] = true;
    m[5][490] = true;
    m[1] ^= m[0];
    m[1] ^= m[5];
    ASSERT_EQ(m[1][10], true);
    ASSERT_EQ(m[1][490], false);
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, inverse_assuming_lower_triangular, {
    auto m = simd_bit_table<W>::identity(4);
    m[3][1] = true;
    ASSERT_EQ(
        m.str(4),
        "1...\n"
        ".1..\n"
        "..1.\n"
        ".1.1");
    auto inv = m.inverse_assuming_lower_triangular(4);
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table<W>::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table<W>::identity(4));
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
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table<W>::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table<W>::identity(4));
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
    ASSERT_EQ(m.square_mat_mul(inv, 4), simd_bit_table<W>::identity(4));
    ASSERT_EQ(inv.square_mat_mul(m, 4), simd_bit_table<W>::identity(4));
    ASSERT_EQ(
        inv.str(4),
        "1...\n"
        "11..\n"
        "..1.\n"
        ".1.1");
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, transposed, {
    auto m = simd_bit_table<W>::identity(4);
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
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, random, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = simd_bit_table<W>::random(100, 90, rng);
    ASSERT_NE(t[99], simd_bits<W>(90));
    ASSERT_EQ(t[100], simd_bits<W>(90));
    t = t.transposed();
    ASSERT_NE(t[89], simd_bits<W>(100));
    ASSERT_EQ(t[90], simd_bits<W>(100));
    ASSERT_NE(simd_bit_table<W>::random(10, 10, rng), simd_bit_table<W>::random(10, 10, rng));
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, slice_maj, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto m = simd_bit_table<W>::random(100, 64, rng);
    auto s = m.slice_maj(5, 15);
    ASSERT_EQ(s[0], m[5]);
    ASSERT_EQ(s[9], m[14]);
    ASSERT_FALSE(s[10].not_zero());
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, from_concat_major, {
    auto a = simd_bit_table<W>::from_text(R"TABLE(
        000111
        101011
        111111
        000000
    )TABLE");
    auto b = simd_bit_table<W>::from_text(R"TABLE(
        100000
        000001
        000100
    )TABLE");
    ASSERT_EQ(a.concat_major(b, 4, 3), simd_bit_table<W>::from_text(R"TABLE(
            000111
            101011
            111111
            000000
            100000
            000001
            000100
        )TABLE"));
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, from_quadrants, {
    simd_bit_table<W> t(2, 2);
    simd_bit_table<W> z(2, 2);
    t[1][0] = true;
    ASSERT_EQ(
        simd_bit_table<W>::from_quadrants(2, t, z, z, z).str(4),
        "....\n"
        "1...\n"
        "....\n"
        "....");
    ASSERT_EQ(
        simd_bit_table<W>::from_quadrants(2, z, t, z, z).str(4),
        "....\n"
        "..1.\n"
        "....\n"
        "....");
    ASSERT_EQ(
        simd_bit_table<W>::from_quadrants(2, z, z, t, z).str(4),
        "....\n"
        "....\n"
        "....\n"
        "1...");
    ASSERT_EQ(
        simd_bit_table<W>::from_quadrants(2, z, z, z, t).str(4),
        "....\n"
        "....\n"
        "....\n"
        "..1.");
    ASSERT_EQ(
        simd_bit_table<W>::from_quadrants(2, t, t, t, t).str(4),
        "....\n"
        "1.1.\n"
        "....\n"
        "1.1.");
})

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

TEST_EACH_WORD_SIZE_W(simd_bit_table, destructive_resize, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bit_table<W> table = simd_bit_table<W>::random(5, 7, rng);
    const uint8_t *prev_pointer = table.data.u8;
    table.destructive_resize(5, 7);
    ASSERT_EQ(table.data.u8, prev_pointer);
    table.destructive_resize(1025, 7);
    ASSERT_NE(table.data.u8, prev_pointer);
    ASSERT_GE(table.num_major_bits_padded(), 1025);
    ASSERT_GE(table.num_minor_bits_padded(), 7);
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, read_across_majors_at_minor_index, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bit_table<W> table = simd_bit_table<W>::random(5, 7, rng);
    simd_bits<W> slice = table.read_across_majors_at_minor_index(2, 5, 1);
    ASSERT_GE(slice.num_bits_padded(), 4);
    ASSERT_EQ(slice[0], table[2][1]);
    ASSERT_EQ(slice[1], table[3][1]);
    ASSERT_EQ(slice[2], table[4][1]);
    ASSERT_EQ(slice[3], false);
})

template <size_t W>
bool is_table_overlap_identical(const simd_bit_table<W> &a, const simd_bit_table<W> &b) {
    size_t w_min = std::min(a.num_simd_words_minor, b.num_simd_words_minor);
    size_t n_maj = std::min(a.num_major_bits_padded(), b.num_major_bits_padded());
    for (size_t k_maj = 0; k_maj < n_maj; k_maj++) {
        if (a[k_maj].word_range_ref(0, w_min) != b[k_maj].word_range_ref(0, w_min)) {
            return false;
        }
    }
    return true;
}

template <size_t W>
bool is_table_zero_outside(const simd_bit_table<W> &a, size_t num_major_bits, size_t num_minor_bits) {
    size_t num_major_words = min_bits_to_num_simd_words<W>(num_major_bits);
    size_t num_minor_words = min_bits_to_num_simd_words<W>(num_minor_bits);
    if (a.num_simd_words_minor > num_minor_words) {
        for (size_t k = 0; k < a.num_simd_words_major; k++) {
            if (a[k].word_range_ref(num_minor_words, a.num_simd_words_minor - num_minor_words).not_zero()) {
                return false;
            }
        }
    }
    for (size_t k = a.num_simd_words_major; k < num_major_words; k++) {
        if (a[k].not_zero()) {
            return false;
        }
    }
    return true;
}

TEST_EACH_WORD_SIZE_W(simd_bit_table, copy_into_different_size_table, {
    auto rng = INDEPENDENT_TEST_RNG();

    auto check_size = [&](size_t w1, size_t h1, size_t w2, size_t h2) {
        simd_bit_table<W> src = simd_bit_table<W>::random(w1, h1, rng);
        simd_bit_table<W> dst = simd_bit_table<W>::random(w1, h1, rng);
        src.copy_into_different_size_table(dst);
        return is_table_overlap_identical(src, dst);
    };

    EXPECT_TRUE(check_size(0, 0, 0, 0));

    EXPECT_TRUE(check_size(64, 0, 0, 0));
    EXPECT_TRUE(check_size(0, 64, 0, 0));
    EXPECT_TRUE(check_size(0, 0, 64, 0));
    EXPECT_TRUE(check_size(0, 0, 0, 64));

    EXPECT_TRUE(check_size(64, 64, 64, 64));
    EXPECT_TRUE(check_size(512, 64, 64, 64));
    EXPECT_TRUE(check_size(64, 512, 64, 64));
    EXPECT_TRUE(check_size(64, 64, 512, 64));
    EXPECT_TRUE(check_size(64, 64, 64, 512));

    EXPECT_TRUE(check_size(512, 512, 64, 64));
    EXPECT_TRUE(check_size(512, 64, 512, 64));
    EXPECT_TRUE(check_size(512, 64, 64, 512));
    EXPECT_TRUE(check_size(64, 512, 512, 64));
    EXPECT_TRUE(check_size(64, 512, 64, 512));
    EXPECT_TRUE(check_size(64, 64, 512, 512));
})

TEST_EACH_WORD_SIZE_W(simd_bit_table, resize, {
    auto rng = INDEPENDENT_TEST_RNG();

    auto check_size = [&](size_t w1, size_t h1, size_t w2, size_t h2) {
        simd_bit_table<W> src = simd_bit_table<W>::random(w1, h1, rng);
        simd_bit_table<W> dst = src;
        dst.resize(w2, h2);
        return is_table_overlap_identical(src, dst) && is_table_zero_outside(dst, std::min(w1, w2), std::min(h1, h2));
    };

    EXPECT_TRUE(check_size(0, 0, 0, 0));

    EXPECT_TRUE(check_size(64, 0, 0, 0));
    EXPECT_TRUE(check_size(0, 64, 0, 0));
    EXPECT_TRUE(check_size(0, 0, 64, 0));
    EXPECT_TRUE(check_size(0, 0, 0, 64));

    EXPECT_TRUE(check_size(64, 64, 64, 64));
    EXPECT_TRUE(check_size(512, 64, 64, 64));
    EXPECT_TRUE(check_size(64, 512, 64, 64));
    EXPECT_TRUE(check_size(64, 64, 512, 64));
    EXPECT_TRUE(check_size(64, 64, 64, 512));

    EXPECT_TRUE(check_size(512, 512, 64, 64));
    EXPECT_TRUE(check_size(512, 64, 512, 64));
    EXPECT_TRUE(check_size(512, 64, 64, 512));
    EXPECT_TRUE(check_size(64, 512, 512, 64));
    EXPECT_TRUE(check_size(64, 512, 64, 512));
    EXPECT_TRUE(check_size(64, 64, 512, 512));
})
