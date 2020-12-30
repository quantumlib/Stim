#include "gtest/gtest.h"
#include "pauli_string.h"

TEST(pauli_string, str) {
    auto p1 = PauliString::from_str("+IXYZ");
    ASSERT_EQ(p1.str(), "+_XYZ");

    auto p2 = PauliString::from_str("X");
    ASSERT_EQ(p2.str(), "+X");

    auto p3 = PauliString::from_str("-XZ");
    ASSERT_EQ(p3.str(), "-XZ");

    auto s1 = PauliString::from_pattern(
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
    auto id = PauliString::from_str("_");
    auto x = PauliString::from_str("X");
    auto y = PauliString::from_str("Y");
    auto z = PauliString::from_str("Z");

    ASSERT_EQ(id.log_i_scalar_byproduct(id), 0);
    ASSERT_EQ(id.log_i_scalar_byproduct(x), 0);
    ASSERT_EQ(id.log_i_scalar_byproduct(y), 0);
    ASSERT_EQ(id.log_i_scalar_byproduct(z), 0);

    ASSERT_EQ(x.log_i_scalar_byproduct(id), 0);
    ASSERT_EQ(x.log_i_scalar_byproduct(x), 0);
    ASSERT_EQ(x.log_i_scalar_byproduct(y), 1);
    ASSERT_EQ(x.log_i_scalar_byproduct(z), 3);

    ASSERT_EQ(y.log_i_scalar_byproduct(id), 0);
    ASSERT_EQ(y.log_i_scalar_byproduct(x), 3);
    ASSERT_EQ(y.log_i_scalar_byproduct(y), 0);
    ASSERT_EQ(y.log_i_scalar_byproduct(z), 1);

    ASSERT_EQ(z.log_i_scalar_byproduct(id), 0);
    ASSERT_EQ(z.log_i_scalar_byproduct(x), 1);
    ASSERT_EQ(z.log_i_scalar_byproduct(y), 3);
    ASSERT_EQ(z.log_i_scalar_byproduct(z), 0);

    ASSERT_EQ(PauliString::from_str("XX").log_i_scalar_byproduct(
            PauliString::from_str("XY")), 1);
    ASSERT_EQ(PauliString::from_str("XX").log_i_scalar_byproduct(
            PauliString::from_str("ZY")), 0);
    ASSERT_EQ(PauliString::from_str("XX").log_i_scalar_byproduct(
            PauliString::from_str("YY")), 2);
    for (size_t n : std::vector<size_t>{1, 499, 4999, 5000}) {
        auto all_x = PauliString::from_pattern(false, n, [](size_t i) { return 'X'; });
        auto all_z = PauliString::from_pattern(false, n, [](size_t i) { return 'Z'; });
        ASSERT_EQ(all_x.log_i_scalar_byproduct(all_z), -(int) n & 3);
    }
}

TEST(pauli_string, equality) {
    ASSERT_TRUE(PauliString::from_str("") == PauliString::from_str(""));
    ASSERT_FALSE(PauliString::from_str("") != PauliString::from_str(""));
    ASSERT_FALSE(PauliString::from_str("") == PauliString::from_str("-"));
    ASSERT_FALSE(PauliString::from_str("X") == PauliString::from_str(""));
    ASSERT_TRUE(PauliString::from_str("XX") == PauliString::from_str("XX"));
    ASSERT_FALSE(PauliString::from_str("XX") == PauliString::from_str("XY"));
    ASSERT_FALSE(PauliString::from_str("XX") == PauliString::from_str("XZ"));
    ASSERT_FALSE(PauliString::from_str("XX") == PauliString::from_str("X_"));

    auto all_x1 = PauliString::from_pattern(false, 1000, [](size_t i) { return 'X'; });
    auto all_x2 = PauliString::from_pattern(false, 1000, [](size_t i) { return 'X'; });
    auto all_z = PauliString::from_pattern(false, 1000, [](size_t i) { return 'Z'; });
    ASSERT_EQ(all_x1, all_x2);
    ASSERT_NE(all_x1, all_z);
}

TEST(pauli_string, multiplication) {
    auto x = PauliString::from_str("X");
    auto y = PauliString::from_str("Y");
    auto z = PauliString::from_str("Z");

    auto lhs = x;
    uint8_t log_i = 0;
    lhs.inplace_right_mul_with_scalar_output(y, &log_i);
    ASSERT_EQ(log_i, 1);
    ASSERT_EQ(lhs, z);

    auto xxi = PauliString::from_str("XXI");
    auto yyy = PauliString::from_str("YYY");
    xxi *= yyy;
    ASSERT_EQ(xxi, PauliString::from_str("-ZZY"));
}

TEST(pauli_string, identity) {
    ASSERT_EQ(PauliString::identity(5).str(), "+_____");
}
