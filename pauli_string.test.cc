#include "gtest/gtest.h"
#include "pauli_string.h"

TEST(pauli_string, str) {
    auto p1 = PauliStringVal::from_str("+IXYZ");
    ASSERT_EQ(p1.str(), "+_XYZ");

    auto p2 = PauliStringVal::from_str("X");
    ASSERT_EQ(p2.str(), "+X");

    auto p3 = PauliStringVal::from_str("-XZ");
    ASSERT_EQ(p3.str(), "-XZ");

    auto s1 = PauliStringVal::from_pattern(
            true,
            24*24,
            [](size_t i) { return "_XYZ"[i & 3]; });
    ASSERT_EQ(s1.str(),
              "-"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
              "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ" "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ");
}

TEST(pauli_string, log_i_scalar_byproduct) {
    auto id_val = PauliStringVal::from_str("_");
    auto x_val = PauliStringVal::from_str("X");
    auto y_val = PauliStringVal::from_str("Y");
    auto z_val = PauliStringVal::from_str("Z");
    PauliStringPtr id = id_val;
    PauliStringPtr x = x_val;
    PauliStringPtr y = y_val;
    PauliStringPtr z = z_val;

    auto f = [](PauliStringVal a, PauliStringPtr b){
        return a.ptr().inplace_right_mul_returning_log_i_scalar(b);
    };

    ASSERT_EQ(f(id, id), 0);
    ASSERT_EQ(f(id, x), 0);
    ASSERT_EQ(f(id, y), 0);
    ASSERT_EQ(f(id, z), 0);

    ASSERT_EQ(f(x, id), 0);
    ASSERT_EQ(f(x, x), 0);
    ASSERT_EQ(f(x, y), 1);
    ASSERT_EQ(f(x, z), 3);

    ASSERT_EQ(f(y, id), 0);
    ASSERT_EQ(f(y, x), 3);
    ASSERT_EQ(f(y, y), 0);
    ASSERT_EQ(f(y, z), 1);

    ASSERT_EQ(f(z, id), 0);
    ASSERT_EQ(f(z, x), 1);
    ASSERT_EQ(f(z, y), 3);
    ASSERT_EQ(f(z, z), 0);

    ASSERT_EQ(f(PauliStringVal::from_str("XX"), PauliStringVal::from_str("XY")), 1);
    ASSERT_EQ(f(PauliStringVal::from_str("XX"), PauliStringVal::from_str("ZY")), 0);
    ASSERT_EQ(f(PauliStringVal::from_str("XX"), PauliStringVal::from_str("YY")), 2);
    for (size_t n : std::vector<size_t>{1, 499, 4999, 5000}) {
        auto all_x = PauliStringVal::from_pattern(false, n, [](size_t i) { return 'X'; });
        auto all_z = PauliStringVal::from_pattern(false, n, [](size_t i) { return 'Z'; });
        ASSERT_EQ(f(all_x, all_z), (-(int) n) & 3);
    }
}

TEST(pauli_string, equality) {
    ASSERT_TRUE(PauliStringVal::from_str("") == PauliStringVal::from_str(""));
    ASSERT_FALSE(PauliStringVal::from_str("") != PauliStringVal::from_str(""));
    ASSERT_FALSE(PauliStringVal::from_str("") == PauliStringVal::from_str("-"));
    ASSERT_FALSE(PauliStringVal::from_str("X") == PauliStringVal::from_str(""));
    ASSERT_TRUE(PauliStringVal::from_str("XX") == PauliStringVal::from_str("XX"));
    ASSERT_FALSE(PauliStringVal::from_str("XX") == PauliStringVal::from_str("XY"));
    ASSERT_FALSE(PauliStringVal::from_str("XX") == PauliStringVal::from_str("XZ"));
    ASSERT_FALSE(PauliStringVal::from_str("XX") == PauliStringVal::from_str("X_"));

    auto all_x1 = PauliStringVal::from_pattern(false, 1000, [](size_t i) { return 'X'; });
    auto all_x2 = PauliStringVal::from_pattern(false, 1000, [](size_t i) { return 'X'; });
    auto all_z = PauliStringVal::from_pattern(false, 1000, [](size_t i) { return 'Z'; });
    ASSERT_EQ(all_x1, all_x2);
    ASSERT_NE(all_x1, all_z);
}

TEST(pauli_string, multiplication) {
    auto x = PauliStringVal::from_str("X");
    auto y = PauliStringVal::from_str("Y");
    auto z = PauliStringVal::from_str("Z");

    auto lhs = x;
    uint8_t log_i = lhs.ptr().inplace_right_mul_returning_log_i_scalar(y);
    ASSERT_EQ(log_i, 1);
    ASSERT_EQ(lhs, z);

    auto xxi = PauliStringVal::from_str("XXI");
    auto yyy = PauliStringVal::from_str("YYY");
    xxi.ptr() *= yyy;
    ASSERT_EQ(xxi, PauliStringVal::from_str("-ZZY"));
}

TEST(pauli_string, identity) {
    ASSERT_EQ(PauliStringVal::identity(5).str(), "+_____");
}

TEST(pauli_string, gather) {
    auto p = PauliStringVal::from_str("-____XXXXYYYYZZZZ");
    auto p2_val = PauliStringVal::identity(4);
    PauliStringPtr p2 = p2_val;
    p.ptr().gather_into(p2, {0, 1, 2, 3});
    ASSERT_EQ(p2, PauliStringVal::from_str("+IIII"));
    p.ptr().gather_into(p2, {4, 7, 8, 9});
    ASSERT_EQ(p2, PauliStringVal::from_str("+XXYY"));
}

TEST(pauli_string, swap_with_overwrite_with) {
    auto a = PauliStringVal::from_pattern(false, 500, [](size_t k){ return "XYZIX"[k % 5]; });
    auto b = PauliStringVal::from_pattern(false, 500, [](size_t k){ return "ZZYIXXY"[k % 7]; });
    auto a2 = a;
    auto b2 = b;

    auto b2_ptr = b2.ptr();
    a2.ptr().swap_with(b2_ptr);
    ASSERT_EQ(a2, b);
    ASSERT_EQ(b2, a);

    a2.ptr().overwrite_with(b2);
    ASSERT_EQ(a2, a);
    ASSERT_EQ(b2, a);
}


TEST(pauli_string, scatter) {
    auto s1 = PauliStringVal::from_str("-_XYZ");
    auto s2 = PauliStringVal::from_str("+XXZZ");
    auto p_val = PauliStringVal::identity(8);
    PauliStringPtr p = p_val;
    s1.ptr().scatter_into(p, {1, 3, 5, 7});
    ASSERT_EQ(p, PauliStringVal::from_str("-___X_Y_Z"));
    s1.ptr().scatter_into(p, {1, 3, 5, 7});
    ASSERT_EQ(p, PauliStringVal::from_str("+___X_Y_Z"));
    s2.ptr().scatter_into(p, {1, 3, 5, 7});
    ASSERT_EQ(p, PauliStringVal::from_str("+_X_X_Z_Z"));
    s2.ptr().scatter_into(p, {4, 5, 6, 7});
    ASSERT_EQ(p, PauliStringVal::from_str("+_X_XXXZZ"));
}

TEST(pauli_string, move_copy_assignment) {
    PauliStringVal x = PauliStringVal::from_str("XYZ");
    ASSERT_EQ(x, PauliStringVal::from_str("XYZ"));

    // Move.
    x = std::move(PauliStringVal::from_str("XXY"));
    ASSERT_EQ(x, PauliStringVal::from_str("XXY"));
    x = std::move(PauliStringVal::from_str("-IIX"));
    ASSERT_EQ(x, PauliStringVal::from_str("-IIX"));

    // Copy.
    auto y = PauliStringVal::from_str("ZZZ");
    x = y;
    ASSERT_EQ(x, PauliStringVal::from_str("ZZZ"));
    y = PauliStringVal::from_str("-ZZZ");
    x = y;
    ASSERT_EQ(x, PauliStringVal::from_str("-ZZZ"));
}

TEST(pauli_string, foreign_memory) {
    size_t bits = 2048;
    auto buffer = aligned_bits256::random(bits);
    bool signs = false;

    auto p1 = PauliStringPtr(500, BitPtr(&signs, 0), &buffer.u64[0], &buffer.u64[16]);
    auto p1b = new PauliStringPtr(500, BitPtr(&signs, 0), &buffer.u64[0], &buffer.u64[16]);
    auto p2 = PauliStringPtr(500, BitPtr(&signs, 1), &buffer.u64[32], &buffer.u64[64]);
    PauliStringVal copy_p1 = p1;
    // p1 aliases p1b.
    ASSERT_EQ(p1, *p1b);
    ASSERT_EQ(p1, copy_p1);
    p1.inplace_right_mul_returning_log_i_scalar(p2);
    ASSERT_EQ(p1, *p1b);
    ASSERT_NE(p1, copy_p1);
    // Deleting p1b shouldn't delete the backing buffer. So p1 survives.
    copy_p1 = p1;
    ASSERT_EQ(p1, copy_p1);
    delete p1b;
    ASSERT_EQ(p1, copy_p1);
}

TEST(pauli_string, commutes) {
    auto f = [](const char *a, const char *b) {
        auto pa = PauliStringVal::from_str(a);
        auto pb = PauliStringVal::from_str(b);
        return pa.ptr().commutes(pb.ptr());
    };
    ASSERT_EQ(f("I", "I"), true);
    ASSERT_EQ(f("I", "X"), true);
    ASSERT_EQ(f("I", "Y"), true);
    ASSERT_EQ(f("I", "Z"), true);
    ASSERT_EQ(f("X", "I"), true);
    ASSERT_EQ(f("X", "X"), true);
    ASSERT_EQ(f("X", "Y"), false);
    ASSERT_EQ(f("X", "Z"), false);
    ASSERT_EQ(f("Y", "I"), true);
    ASSERT_EQ(f("Y", "X"), false);
    ASSERT_EQ(f("Y", "Y"), true);
    ASSERT_EQ(f("Y", "Z"), false);
    ASSERT_EQ(f("Z", "I"), true);
    ASSERT_EQ(f("Z", "X"), false);
    ASSERT_EQ(f("Z", "Y"), false);
    ASSERT_EQ(f("Z", "Z"), true);

    ASSERT_EQ(f("XX", "ZZ"), true);
    ASSERT_EQ(f("-XX", "ZZ"), true);
    ASSERT_EQ(f("XZ", "ZZ"), false);
    ASSERT_EQ(f("-XZ", "ZZ"), false);

    auto qa = PauliStringVal::from_pattern(false, 5000, [](size_t k) { return k == 0 ? 'X' : 'Z'; });
    auto qb = PauliStringVal::from_pattern(false, 5000, [](size_t k) { return 'Z'; });
    ASSERT_EQ(qa.ptr().commutes(qa.ptr()), true);
    ASSERT_EQ(qb.ptr().commutes(qb.ptr()), true);
    ASSERT_EQ(qa.ptr().commutes(qb.ptr()), false);
    ASSERT_EQ(qb.ptr().commutes(qa.ptr()), false);
}
