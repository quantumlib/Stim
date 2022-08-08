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

#include "stim/stabilizers/pauli_string.h"

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(pauli_string, str) {
    auto p1 = PauliString::from_str("+IXYZ");
    ASSERT_EQ(p1.str(), "+_XYZ");

    auto p2 = PauliString::from_str("X");
    ASSERT_EQ(p2.str(), "+X");

    auto p3 = PauliString::from_str("-XZ");
    ASSERT_EQ(p3.str(), "-XZ");

    auto s1 = PauliString::from_func(true, 24 * 24, [](size_t i) {
        return "_XYZ"[i & 3];
    });
    ASSERT_EQ(
        s1.str(),
        "-"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ"
        "_XYZ_XYZ_XYZ_XYZ_XYZ_XYZ");
}

TEST(pauli_string, log_i_scalar_byproduct) {
    auto id = PauliString::from_str("_");
    auto x = PauliString::from_str("X");
    auto y = PauliString::from_str("Y");
    auto z = PauliString::from_str("Z");

    auto f = [](PauliString a, PauliString b) {
        // Note: copying is intentational. Do not change args to references.
        return a.ref().inplace_right_mul_returning_log_i_scalar(b);
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

    ASSERT_EQ(f(PauliString::from_str("XX"), PauliString::from_str("XY")), 1);
    ASSERT_EQ(f(PauliString::from_str("XX"), PauliString::from_str("ZY")), 0);
    ASSERT_EQ(f(PauliString::from_str("XX"), PauliString::from_str("YY")), 2);
    for (size_t n : std::vector<size_t>{1, 499, 4999, 5000}) {
        auto all_x = PauliString::from_func(false, n, [](size_t i) {
            return 'X';
        });
        auto all_z = PauliString::from_func(false, n, [](size_t i) {
            return 'Z';
        });
        ASSERT_EQ(f(all_x, all_z), (-(int)n) & 3);
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

    auto all_x1 = PauliString::from_func(false, 1000, [](size_t i) {
        return 'X';
    });
    auto all_x2 = PauliString::from_func(false, 1000, [](size_t i) {
        return 'X';
    });
    auto all_z = PauliString::from_func(false, 1000, [](size_t i) {
        return 'Z';
    });
    ASSERT_EQ(all_x1, all_x2);
    ASSERT_NE(all_x1, all_z);
}

TEST(pauli_string, multiplication) {
    auto x = PauliString::from_str("X");
    auto y = PauliString::from_str("Y");
    auto z = PauliString::from_str("Z");

    auto lhs = x;
    uint8_t log_i = lhs.ref().inplace_right_mul_returning_log_i_scalar(y);
    ASSERT_EQ(log_i, 1);
    ASSERT_EQ(lhs, z);

    auto xxi = PauliString::from_str("XXI");
    auto yyy = PauliString::from_str("YYY");
    xxi.ref() *= yyy;
    ASSERT_EQ(xxi, PauliString::from_str("-ZZY"));
}

TEST(pauli_string, identity) {
    ASSERT_EQ(PauliString(5).str(), "+_____");
}

TEST(pauli_string, gather) {
    auto p = PauliString::from_str("-____XXXXYYYYZZZZ");
    auto p2 = PauliString(4);
    p.ref().gather_into(p2, {0, 1, 2, 3});
    ASSERT_EQ(p2, PauliString::from_str("+IIII"));
    p.ref().gather_into(p2, {4, 7, 8, 9});
    ASSERT_EQ(p2, PauliString::from_str("+XXYY"));
}

TEST(pauli_string, swap_with_overwrite_with) {
    auto a = PauliString::from_func(false, 500, [](size_t k) {
        return "XYZIX"[k % 5];
    });
    auto b = PauliString::from_func(false, 500, [](size_t k) {
        return "ZZYIXXY"[k % 7];
    });
    auto a2 = a;
    auto b2 = b;

    a2.ref().swap_with(b2);
    ASSERT_EQ(a2, b);
    ASSERT_EQ(b2, a);

    a2.ref() = b2;
    ASSERT_EQ(a2, a);
    ASSERT_EQ(b2, a);
}

TEST(pauli_string, scatter) {
    auto s1 = PauliString::from_str("-_XYZ");
    auto s2 = PauliString::from_str("+XXZZ");
    auto p = PauliString(8);
    s1.ref().scatter_into(p, {1, 3, 5, 7});
    ASSERT_EQ(p, PauliString::from_str("-___X_Y_Z"));
    s1.ref().scatter_into(p, {1, 3, 5, 7});
    ASSERT_EQ(p, PauliString::from_str("+___X_Y_Z"));
    s2.ref().scatter_into(p, {1, 3, 5, 7});
    ASSERT_EQ(p, PauliString::from_str("+_X_X_Z_Z"));
    s2.ref().scatter_into(p, {4, 5, 6, 7});
    ASSERT_EQ(p, PauliString::from_str("+_X_XXXZZ"));
}

TEST(pauli_string, move_copy_assignment) {
    PauliString x = PauliString::from_str("XYZ");
    ASSERT_EQ(x, PauliString::from_str("XYZ"));

    // Move.
    PauliString z = PauliString::from_str("XXY");
    x = std::move(z);
    ASSERT_EQ(x, PauliString::from_str("XXY"));
    z = PauliString::from_str("-IIX");
    x = std::move(z);
    ASSERT_EQ(x, PauliString::from_str("-IIX"));

    // Copy.
    auto y = PauliString::from_str("ZZZ");
    x = y;
    ASSERT_EQ(x, PauliString::from_str("ZZZ"));
    y = PauliString::from_str("-ZZZ");
    x = y;
    ASSERT_EQ(x, PauliString::from_str("-ZZZ"));
}

TEST(pauli_string, foreign_memory) {
    size_t bits = 2048;
    auto buffer = simd_bits<MAX_BITWORD_WIDTH>::random(bits, SHARED_TEST_RNG());
    bool signs = false;

    auto p1 = PauliStringRef(500, bit_ref(&signs, 0), buffer.word_range_ref(0, 2), buffer.word_range_ref(4, 2));
    auto p1b = new PauliStringRef(500, bit_ref(&signs, 0), buffer.word_range_ref(0, 2), buffer.word_range_ref(4, 2));
    auto p2 = PauliStringRef(500, bit_ref(&signs, 1), buffer.word_range_ref(2, 2), buffer.word_range_ref(6, 2));
    PauliString copy_p1 = p1;
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
        auto pa = PauliString::from_str(a);
        auto pb = PauliString::from_str(b);
        return pa.ref().commutes(pb);
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

    auto qa = PauliString::from_func(false, 5000, [](size_t k) {
        return k == 0 ? 'X' : 'Z';
    });
    auto qb = PauliString::from_func(false, 5000, [](size_t k) {
        return 'Z';
    });
    ASSERT_EQ(qa.ref().commutes(qa), true);
    ASSERT_EQ(qb.ref().commutes(qb), true);
    ASSERT_EQ(qa.ref().commutes(qb), false);
    ASSERT_EQ(qb.ref().commutes(qa), false);

    // Differing sizes.
    ASSERT_EQ(qa.ref().commutes(PauliString(0)), true);
    ASSERT_EQ(PauliString(0).ref().commutes(qa), true);
}

TEST(PauliStringPtr, sparse_str) {
    ASSERT_EQ(PauliString::from_str("IIIII").ref().sparse_str(), "+I");
    ASSERT_EQ(PauliString::from_str("-IIIII").ref().sparse_str(), "-I");
    ASSERT_EQ(PauliString::from_str("IIIXI").ref().sparse_str(), "+X3");
    ASSERT_EQ(PauliString::from_str("IYIXZ").ref().sparse_str(), "+Y1*X3*Z4");
    ASSERT_EQ(PauliString::from_str("-IYIXZ").ref().sparse_str(), "-Y1*X3*Z4");
    auto x501 = PauliString::from_func(
                    false,
                    1000,
                    [](size_t k) {
                        return "IX"[k == 501];
                    })
                    .ref()
                    .sparse_str();
    ASSERT_EQ(x501, "+X501");
}

TEST(PauliString, ensure_num_qubits) {
    auto p = PauliString::from_str("IXYZ_I");
    p.ensure_num_qubits(1);
    ASSERT_EQ(p, PauliString::from_str("IXYZ_I"));
    p.ensure_num_qubits(6);
    ASSERT_EQ(p, PauliString::from_str("IXYZ_I"));
    p.ensure_num_qubits(7);
    ASSERT_EQ(p, PauliString::from_str("IXYZ_I_"));
    p.ensure_num_qubits(1000);
    PauliString p2(1000);
    p2.xs[1] = true;
    p2.xs[2] = true;
    p2.zs[2] = true;
    p2.zs[3] = true;
    ASSERT_EQ(p, p2);
}

TEST(PauliString, pauli_xz_to_xyz) {
    ASSERT_EQ(pauli_xz_to_xyz(false, false), 0);
    ASSERT_EQ(pauli_xz_to_xyz(true, false), 1);
    ASSERT_EQ(pauli_xz_to_xyz(true, true), 2);
    ASSERT_EQ(pauli_xz_to_xyz(false, true), 3);
}

TEST(PauliString, pauli_xyz_to_xz) {
    uint8_t x = 1;
    uint8_t z = 2;
    ASSERT_EQ(pauli_xyz_to_xz(0), 0);
    ASSERT_EQ(pauli_xyz_to_xz(1), x);
    ASSERT_EQ(pauli_xyz_to_xz(2), x + z);
    ASSERT_EQ(pauli_xyz_to_xz(3), z);
}

TEST(PauliString, py_get_item) {
    auto p = PauliString::from_str("-XYZ_XYZ_XX");
    ASSERT_EQ(p.py_get_item(0), 1);
    ASSERT_EQ(p.py_get_item(1), 2);
    ASSERT_EQ(p.py_get_item(2), 3);
    ASSERT_EQ(p.py_get_item(3), 0);
    ASSERT_EQ(p.py_get_item(8), 1);
    ASSERT_EQ(p.py_get_item(9), 1);

    ASSERT_EQ(p.py_get_item(-1), 1);
    ASSERT_EQ(p.py_get_item(-2), 1);
    ASSERT_EQ(p.py_get_item(-3), 0);
    ASSERT_EQ(p.py_get_item(-4), 3);
    ASSERT_EQ(p.py_get_item(-9), 2);
    ASSERT_EQ(p.py_get_item(-10), 1);

    ASSERT_ANY_THROW({ p.py_get_item(10); });
    ASSERT_ANY_THROW({ p.py_get_item(-11); });
}

TEST(PauliString, py_get_slice) {
    auto p = PauliString::from_str("-XYZ_XYZ_YX");
    ASSERT_EQ(p.py_get_slice(0, 2, 0), PauliString::from_str(""));
    ASSERT_EQ(p.py_get_slice(0, 2, 1), PauliString::from_str("X"));
    ASSERT_EQ(p.py_get_slice(0, 2, 2), PauliString::from_str("XZ"));
    ASSERT_EQ(p.py_get_slice(1, 2, 2), PauliString::from_str("Y_"));
    ASSERT_EQ(p.py_get_slice(5, 1, 4), PauliString::from_str("YZ_Y"));
    ASSERT_EQ(p.py_get_slice(5, -1, 4), PauliString::from_str("YX_Z"));
}
