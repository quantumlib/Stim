#include "gtest/gtest.h"
#include "pauli_string.h"

TEST(pauli_string, str) {
    auto p1 = PauliStringStorage::from_str("+IXYZ");
    ASSERT_EQ(p1.ptr().str(), "+_XYZ");

    auto p2 = PauliStringStorage::from_str("X");
    ASSERT_EQ(p2.ptr().str(), "+X");

    auto p3 = PauliStringStorage::from_str("-XZ");
    ASSERT_EQ(p3.ptr().str(), "-XZ");

    auto s1 = PauliStringStorage::from_pattern(
            true,
            24*24,
            [](size_t i) { return "_XYZ"[i & 3]; });
    ASSERT_EQ(s1.ptr().str(),
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
    auto id = PauliStringStorage::from_str("_");
    auto x = PauliStringStorage::from_str("X");
    auto y = PauliStringStorage::from_str("Y");
    auto z = PauliStringStorage::from_str("Z");

    ASSERT_EQ(id.ptr().log_i_scalar_byproduct(id.ptr()), 0);
    ASSERT_EQ(id.ptr().log_i_scalar_byproduct(x.ptr()), 0);
    ASSERT_EQ(id.ptr().log_i_scalar_byproduct(y.ptr()), 0);
    ASSERT_EQ(id.ptr().log_i_scalar_byproduct(z.ptr()), 0);

    ASSERT_EQ(x.ptr().log_i_scalar_byproduct(id.ptr()), 0);
    ASSERT_EQ(x.ptr().log_i_scalar_byproduct(x.ptr()), 0);
    ASSERT_EQ(x.ptr().log_i_scalar_byproduct(y.ptr()), 1);
    ASSERT_EQ(x.ptr().log_i_scalar_byproduct(z.ptr()), 3);

    ASSERT_EQ(y.ptr().log_i_scalar_byproduct(id.ptr()), 0);
    ASSERT_EQ(y.ptr().log_i_scalar_byproduct(x.ptr()), 3);
    ASSERT_EQ(y.ptr().log_i_scalar_byproduct(y.ptr()), 0);
    ASSERT_EQ(y.ptr().log_i_scalar_byproduct(z.ptr()), 1);

    ASSERT_EQ(z.ptr().log_i_scalar_byproduct(id.ptr()), 0);
    ASSERT_EQ(z.ptr().log_i_scalar_byproduct(x.ptr()), 1);
    ASSERT_EQ(z.ptr().log_i_scalar_byproduct(y.ptr()), 3);
    ASSERT_EQ(z.ptr().log_i_scalar_byproduct(z.ptr()), 0);

    ASSERT_EQ(PauliStringStorage::from_str("XX").ptr().log_i_scalar_byproduct(
            PauliStringStorage::from_str("XY").ptr()), 1);
    ASSERT_EQ(PauliStringStorage::from_str("XX").ptr().log_i_scalar_byproduct(
            PauliStringStorage::from_str("ZY").ptr()), 0);
    ASSERT_EQ(PauliStringStorage::from_str("XX").ptr().log_i_scalar_byproduct(
            PauliStringStorage::from_str("YY").ptr()), 2);
    for (size_t n : std::vector<size_t>{1, 499, 4999, 5000}) {
        auto all_x = PauliStringStorage::from_pattern(false, n, [](size_t i) { return 'X'; });
        auto all_z = PauliStringStorage::from_pattern(false, n, [](size_t i) { return 'Z'; });
        ASSERT_EQ(all_x.ptr().log_i_scalar_byproduct(all_z.ptr()), -(int) n & 3);
    }
}

TEST(pauli_string, equality) {
    ASSERT_TRUE(PauliStringStorage::from_str("").ptr() == PauliStringStorage::from_str("").ptr());
    ASSERT_FALSE(PauliStringStorage::from_str("").ptr() != PauliStringStorage::from_str("").ptr());
    ASSERT_FALSE(PauliStringStorage::from_str("").ptr() == PauliStringStorage::from_str("-").ptr());
    ASSERT_FALSE(PauliStringStorage::from_str("X").ptr() == PauliStringStorage::from_str("").ptr());
    ASSERT_TRUE(PauliStringStorage::from_str("XX").ptr() == PauliStringStorage::from_str("XX").ptr());
    ASSERT_FALSE(PauliStringStorage::from_str("XX").ptr() == PauliStringStorage::from_str("XY").ptr());
    ASSERT_FALSE(PauliStringStorage::from_str("XX").ptr() == PauliStringStorage::from_str("XZ").ptr());
    ASSERT_FALSE(PauliStringStorage::from_str("XX").ptr() == PauliStringStorage::from_str("X_").ptr());

    auto all_x1 = PauliStringStorage::from_pattern(false, 1000, [](size_t i) { return 'X'; });
    auto all_x2 = PauliStringStorage::from_pattern(false, 1000, [](size_t i) { return 'X'; });
    auto all_z = PauliStringStorage::from_pattern(false, 1000, [](size_t i) { return 'Z'; });
    ASSERT_EQ(all_x1.ptr(), all_x2.ptr());
    ASSERT_NE(all_x1.ptr(), all_z.ptr());
}
