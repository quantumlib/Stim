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

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(pauli_string, str, {
    auto p1 = PauliString<W>::from_str("+IXYZ");
    ASSERT_EQ(p1.str(), "+_XYZ");

    auto p2 = PauliString<W>::from_str("X");
    ASSERT_EQ(p2.str(), "+X");

    auto p3 = PauliString<W>::from_str("-XZ");
    ASSERT_EQ(p3.str(), "-XZ");

    auto s1 = PauliString<W>::from_func(true, 24 * 24, [](size_t i) {
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
})

TEST_EACH_WORD_SIZE_W(pauli_string, log_i_scalar_byproduct, {
    auto id = PauliString<W>::from_str("_");
    auto x = PauliString<W>::from_str("X");
    auto y = PauliString<W>::from_str("Y");
    auto z = PauliString<W>::from_str("Z");

    auto f = [](PauliString<W> a, PauliString<W> b) {
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

    ASSERT_EQ(f(PauliString<W>::from_str("XX"), PauliString<W>::from_str("XY")), 1);
    ASSERT_EQ(f(PauliString<W>::from_str("XX"), PauliString<W>::from_str("ZY")), 0);
    ASSERT_EQ(f(PauliString<W>::from_str("XX"), PauliString<W>::from_str("YY")), 2);
    for (size_t n : std::vector<size_t>{1, 499, 4999, 5000}) {
        auto all_x = PauliString<W>::from_func(false, n, [](size_t i) {
            return 'X';
        });
        auto all_z = PauliString<W>::from_func(false, n, [](size_t i) {
            return 'Z';
        });
        ASSERT_EQ(f(all_x, all_z), (-(int)n) & 3);
    }
})

TEST_EACH_WORD_SIZE_W(pauli_string, equality, {
    ASSERT_TRUE(PauliString<W>::from_str("") == PauliString<W>::from_str(""));
    ASSERT_FALSE(PauliString<W>::from_str("") != PauliString<W>::from_str(""));
    ASSERT_FALSE(PauliString<W>::from_str("") == PauliString<W>::from_str("-"));
    ASSERT_FALSE(PauliString<W>::from_str("X") == PauliString<W>::from_str(""));
    ASSERT_TRUE(PauliString<W>::from_str("XX") == PauliString<W>::from_str("XX"));
    ASSERT_FALSE(PauliString<W>::from_str("XX") == PauliString<W>::from_str("XY"));
    ASSERT_FALSE(PauliString<W>::from_str("XX") == PauliString<W>::from_str("XZ"));
    ASSERT_FALSE(PauliString<W>::from_str("XX") == PauliString<W>::from_str("X_"));

    auto all_x1 = PauliString<W>::from_func(false, 1000, [](size_t i) {
        return 'X';
    });
    auto all_x2 = PauliString<W>::from_func(false, 1000, [](size_t i) {
        return 'X';
    });
    auto all_z = PauliString<W>::from_func(false, 1000, [](size_t i) {
        return 'Z';
    });
    ASSERT_EQ(all_x1, all_x2);
    ASSERT_NE(all_x1, all_z);
})

TEST_EACH_WORD_SIZE_W(pauli_string, multiplication, {
    auto x = PauliString<W>::from_str("X");
    auto y = PauliString<W>::from_str("Y");
    auto z = PauliString<W>::from_str("Z");

    auto lhs = x;
    uint8_t log_i = lhs.ref().inplace_right_mul_returning_log_i_scalar(y);
    ASSERT_EQ(log_i, 1);
    ASSERT_EQ(lhs, z);

    auto xxi = PauliString<W>::from_str("XXI");
    auto yyy = PauliString<W>::from_str("YYY");
    xxi.ref() *= yyy;
    ASSERT_EQ(xxi, PauliString<W>::from_str("-ZZY"));
})

TEST_EACH_WORD_SIZE_W(pauli_string, identity, { ASSERT_EQ(PauliString<W>(5).str(), "+_____"); })

TEST_EACH_WORD_SIZE_W(pauli_string, gather, {
    auto p = PauliString<W>::from_str("-____XXXXYYYYZZZZ");
    auto p2 = PauliString<W>(4);
    p.ref().gather_into(p2, std::vector<size_t>{0, 1, 2, 3});
    ASSERT_EQ(p2, PauliString<W>::from_str("+IIII"));
    p.ref().gather_into(p2, std::vector<size_t>{4, 7, 8, 9});
    ASSERT_EQ(p2, PauliString<W>::from_str("+XXYY"));
})

TEST_EACH_WORD_SIZE_W(pauli_string, swap_with_overwrite_with, {
    auto a = PauliString<W>::from_func(false, 500, [](size_t k) {
        return "XYZIX"[k % 5];
    });
    auto b = PauliString<W>::from_func(false, 500, [](size_t k) {
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
})

TEST_EACH_WORD_SIZE_W(pauli_string, scatter, {
    auto s1 = PauliString<W>::from_str("-_XYZ");
    auto s2 = PauliString<W>::from_str("+XXZZ");
    auto p = PauliString<W>(8);
    s1.ref().scatter_into(p, std::vector<size_t>{1, 3, 5, 7});
    ASSERT_EQ(p, PauliString<W>::from_str("-___X_Y_Z"));
    s1.ref().scatter_into(p, std::vector<size_t>{1, 3, 5, 7});
    ASSERT_EQ(p, PauliString<W>::from_str("+___X_Y_Z"));
    s2.ref().scatter_into(p, std::vector<size_t>{1, 3, 5, 7});
    ASSERT_EQ(p, PauliString<W>::from_str("+_X_X_Z_Z"));
    s2.ref().scatter_into(p, std::vector<size_t>{4, 5, 6, 7});
    ASSERT_EQ(p, PauliString<W>::from_str("+_X_XXXZZ"));
})

TEST_EACH_WORD_SIZE_W(pauli_string, move_copy_assignment, {
    PauliString<W> x = PauliString<W>::from_str("XYZ");
    ASSERT_EQ(x, PauliString<W>::from_str("XYZ"));

    // Move.
    PauliString<W> z = PauliString<W>::from_str("XXY");
    x = std::move(z);
    ASSERT_EQ(x, PauliString<W>::from_str("XXY"));
    z = PauliString<W>::from_str("-IIX");
    x = std::move(z);
    ASSERT_EQ(x, PauliString<W>::from_str("-IIX"));

    // Copy.
    auto y = PauliString<W>::from_str("ZZZ");
    x = y;
    ASSERT_EQ(x, PauliString<W>::from_str("ZZZ"));
    y = PauliString<W>::from_str("-ZZZ");
    x = y;
    ASSERT_EQ(x, PauliString<W>::from_str("-ZZZ"));
})

TEST_EACH_WORD_SIZE_W(pauli_string, foreign_memory, {
    auto rng = INDEPENDENT_TEST_RNG();
    size_t bits = 2048;
    auto buffer = simd_bits<W>::random(bits, rng);
    bool signs = false;
    size_t num_qubits = W * 2 - 12;

    auto p1 =
        PauliStringRef<W>(num_qubits, bit_ref(&signs, 0), buffer.word_range_ref(0, 2), buffer.word_range_ref(4, 2));
    auto p1b =
        new PauliStringRef<W>(num_qubits, bit_ref(&signs, 0), buffer.word_range_ref(0, 2), buffer.word_range_ref(4, 2));
    auto p2 =
        PauliStringRef<W>(num_qubits, bit_ref(&signs, 1), buffer.word_range_ref(2, 2), buffer.word_range_ref(6, 2));
    PauliString<W> copy_p1 = p1;
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
})

TEST_EACH_WORD_SIZE_W(pauli_string, commutes, {
    auto f = [](const char *a, const char *b) {
        auto pa = PauliString<W>::from_str(a);
        auto pb = PauliString<W>::from_str(b);
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

    auto qa = PauliString<W>::from_func(false, 5000, [](size_t k) {
        return k == 0 ? 'X' : 'Z';
    });
    auto qb = PauliString<W>::from_func(false, 5000, [](size_t k) {
        return 'Z';
    });
    ASSERT_EQ(qa.ref().commutes(qa), true);
    ASSERT_EQ(qb.ref().commutes(qb), true);
    ASSERT_EQ(qa.ref().commutes(qb), false);
    ASSERT_EQ(qb.ref().commutes(qa), false);

    // Differing sizes.
    ASSERT_EQ(qa.ref().commutes(PauliString<W>(0)), true);
    ASSERT_EQ(PauliString<W>(0).ref().commutes(qa), true);
})

TEST_EACH_WORD_SIZE_W(pauli_string, sparse_str, {
    ASSERT_EQ(PauliString<W>::from_str("IIIII").ref().sparse_str(), "+I");
    ASSERT_EQ(PauliString<W>::from_str("-IIIII").ref().sparse_str(), "-I");
    ASSERT_EQ(PauliString<W>::from_str("IIIXI").ref().sparse_str(), "+X3");
    ASSERT_EQ(PauliString<W>::from_str("IYIXZ").ref().sparse_str(), "+Y1*X3*Z4");
    ASSERT_EQ(PauliString<W>::from_str("-IYIXZ").ref().sparse_str(), "-Y1*X3*Z4");
    auto x501 = PauliString<W>::from_func(
                    false,
                    1000,
                    [](size_t k) {
                        return "IX"[k == 501];
                    })
                    .ref()
                    .sparse_str();
    ASSERT_EQ(x501, "+X501");
})

TEST_EACH_WORD_SIZE_W(pauli_string, ensure_num_qubits, {
    auto p = PauliString<W>::from_str("IXYZ_I");
    p.ensure_num_qubits(1, 1.0);
    ASSERT_EQ(p, PauliString<W>::from_str("IXYZ_I"));
    p.ensure_num_qubits(6, 1.0);
    ASSERT_EQ(p, PauliString<W>::from_str("IXYZ_I"));
    p.ensure_num_qubits(7, 1.0);
    ASSERT_EQ(p, PauliString<W>::from_str("IXYZ_I_"));
    p.ensure_num_qubits(1000, 1.0);
    PauliString<W> p2(1000);
    p2.xs[1] = true;
    p2.xs[2] = true;
    p2.zs[2] = true;
    p2.zs[3] = true;
    ASSERT_EQ(p, p2);
})

TEST_EACH_WORD_SIZE_W(pauli_string, ensure_num_qubits_padded, {
    auto p = PauliString<W>::from_str("IXYZ_I");
    auto p2 = p;
    p.ensure_num_qubits(1121, 10.0);
    p2.ensure_num_qubits(1121, 1.0);
    ASSERT_GT(p.xs.num_simd_words, p2.xs.num_simd_words);
    ASSERT_EQ(p, p2);
})

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

TEST_EACH_WORD_SIZE_W(pauli_string, py_get_item, {
    auto p = PauliString<W>::from_str("-XYZ_XYZ_XX");
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
})

TEST_EACH_WORD_SIZE_W(pauli_string, py_get_slice, {
    auto p = PauliString<W>::from_str("-XYZ_XYZ_YX");
    ASSERT_EQ(p.py_get_slice(0, 2, 0), PauliString<W>::from_str(""));
    ASSERT_EQ(p.py_get_slice(0, 2, 1), PauliString<W>::from_str("X"));
    ASSERT_EQ(p.py_get_slice(0, 2, 2), PauliString<W>::from_str("XZ"));
    ASSERT_EQ(p.py_get_slice(1, 2, 2), PauliString<W>::from_str("Y_"));
    ASSERT_EQ(p.py_get_slice(5, 1, 4), PauliString<W>::from_str("YZ_Y"));
    ASSERT_EQ(p.py_get_slice(5, -1, 4), PauliString<W>::from_str("YX_Z"));
})

TEST_EACH_WORD_SIZE_W(pauli_string, after_circuit, {
    auto actual = PauliString<W>::from_str("+_XYZ").ref().after(Circuit(R"CIRCUIT(
        H 1
        CNOT 1 2
        S 2
    )CIRCUIT"));
    ASSERT_EQ(actual, PauliString<W>::from_str("-__XZ"));

    actual = PauliString<W>::from_str("+X___").ref().after(Circuit(R"CIRCUIT(
        CX 0 1 1 2 2 3
    )CIRCUIT"));
    ASSERT_EQ(actual, PauliString<W>::from_str("+XXXX"));

    actual = PauliString<W>::from_str("+X___").ref().after(Circuit(R"CIRCUIT(
        REPEAT 6 {
            CX 0 1 1 2 2 3
        }
    )CIRCUIT"));
    ASSERT_EQ(actual, PauliString<W>::from_str("+X_X_"));

    ASSERT_THROW(
        {
            PauliString<W>::from_str("+X___").ref().after(Circuit(R"CIRCUIT(
            M(0.1) 0
        )CIRCUIT"));
        },
        std::invalid_argument);

    ASSERT_THROW(
        {
            PauliString<W>::from_str("+X").ref().after(Circuit(R"CIRCUIT(
                Z_ERROR(0.1) 0
            )CIRCUIT"));
        },
        std::invalid_argument);

    ASSERT_THROW(
        {
            PauliString<W>::from_str("+X").ref().after(Circuit(R"CIRCUIT(
            H 9
        )CIRCUIT"));
        },
        std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, before_after_circuit_ignores_annotations, {
    auto c = Circuit(R"CIRCUIT(
        QUBIT_COORDS(2, 3) 5
        REPEAT 5 {
            DETECTOR rec[-1]
        }
        H 1
        TICK
        OBSERVABLE_INCLUDE(0) rec[-1]
        CNOT 1 2
        S 2
        SHIFT_COORDS(1, 2, 3)
        TICK
    )CIRCUIT");
    auto before = PauliString<W>::from_str("+_XYZ");
    auto after = PauliString<W>::from_str("-__XZ");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);
})

TEST_EACH_WORD_SIZE_W(pauli_string, before_after_circuit_understands_avoiding_resets, {
    auto c = Circuit(R"CIRCUIT(
        R 0
        MR 1
        RX 2
        MRX 3
        RY 4
        MRY 5
        H 6
    )CIRCUIT");
    auto before = PauliString<W>::from_str("+______X");
    auto after = PauliString<W>::from_str("+______Z");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    ASSERT_THROW({ PauliString<W>::from_str("+Z______").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+X______").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+_Z_____").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+__Z____").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+___Z___").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+____Z__").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+_____Z_").ref().after(c); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, before_after_circuit_understands_commutation_with_m, {
    auto c = Circuit(R"CIRCUIT(
        M 0
        H 1
    )CIRCUIT");
    auto before = PauliString<W>::from_str("+_X");
    auto after = PauliString<W>::from_str("+_Z");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = after = PauliString<W>::from_str("+Z_");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    ASSERT_THROW({ PauliString<W>::from_str("+X_").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+Y_").ref().after(c); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, before_after_circuit_understands_commutation_with_mx, {
    auto c = Circuit(R"CIRCUIT(
        MX 0
        H 1
    )CIRCUIT");
    auto before = PauliString<W>::from_str("+_X");
    auto after = PauliString<W>::from_str("+_Z");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = after = PauliString<W>::from_str("+X_");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    ASSERT_THROW({ PauliString<W>::from_str("+Z_").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+Y_").ref().after(c); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, before_after_circuit_understands_commutation_with_my, {
    auto c = Circuit(R"CIRCUIT(
        MY 0
        H 1
    )CIRCUIT");
    auto before = PauliString<W>::from_str("+_X");
    auto after = PauliString<W>::from_str("+_Z");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = after = PauliString<W>::from_str("+Y_");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    ASSERT_THROW({ PauliString<W>::from_str("+X_").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+Z_").ref().after(c); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, before_after_circuit_understands_commutation_with_mpp, {
    auto c = Circuit(R"CIRCUIT(
        MPP X2*Y3*Z4 X5*X6
        H 1
    )CIRCUIT");
    auto before = PauliString<W>::from_str("+_X_____");
    auto after = PauliString<W>::from_str("+_Z_____");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = PauliString<W>::from_str("+_XXYZXX");
    after = PauliString<W>::from_str("+_ZXYZXX");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = PauliString<W>::from_str("+_XX____");
    after = PauliString<W>::from_str("+_ZX____");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = after = PauliString<W>::from_str("+__ZX___");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = after = PauliString<W>::from_str("+__ZZ_ZZ");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = after = PauliString<W>::from_str("+___XYZZ");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = after = PauliString<W>::from_str("+__XXYZZ");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    before = after = PauliString<W>::from_str("+__X____");
    ASSERT_EQ(before.ref().after(c), after);
    ASSERT_EQ(after.ref().before(c), before);

    ASSERT_THROW({ PauliString<W>::from_str("+__XXYZX").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+_____ZX").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+__Z____").ref().after(c); }, std::invalid_argument);
    ASSERT_THROW({ PauliString<W>::from_str("+__XXYXY").ref().after(c); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, before_circuit, {
    auto actual = PauliString<W>::from_str("-__XZ").ref().before(Circuit(R"CIRCUIT(
        H 1
        CNOT 1 2
        S 2
    )CIRCUIT"));
    ASSERT_EQ(actual, PauliString<W>::from_str("+_XYZ"));

    actual = PauliString<W>::from_str("+XXXX").ref().before(Circuit(R"CIRCUIT(
        CX 0 1 1 2 2 3
    )CIRCUIT"));
    ASSERT_EQ(actual, PauliString<W>::from_str("+X___"));

    actual = PauliString<W>::from_str("+X_X_").ref().after(Circuit(R"CIRCUIT(
        REPEAT 6 {
            CX 0 1 1 2 2 3
        }
    )CIRCUIT"));
    ASSERT_EQ(actual, PauliString<W>::from_str("+X___"));

    ASSERT_THROW(
        {
            PauliString<W>::from_str("+X___").ref().before(Circuit(R"CIRCUIT(
            M(0.1) 0
        )CIRCUIT"));
        },
        std::invalid_argument);

    ASSERT_THROW(
        {
            PauliString<W>::from_str("+X").ref().before(Circuit(R"CIRCUIT(
            H 9
        )CIRCUIT"));
        },
        std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, after_tableau, {
    auto actual =
        PauliString<W>::from_str("+XZ_").ref().after(GATE_DATA.at("CX").tableau<W>(), std::vector<size_t>{0, 1, 1, 2});
    ASSERT_EQ(actual, PauliString<W>::from_str("-YYX"));

    ASSERT_THROW(
        {
            PauliString<W>::from_str("+XZ_").ref().after(GATE_DATA.at("CX").tableau<W>(), std::vector<size_t>{0, 1, 1});
        },
        std::invalid_argument);

    ASSERT_THROW(
        { PauliString<W>::from_str("+XZ_").ref().after(GATE_DATA.at("CX").tableau<W>(), std::vector<size_t>{0, 5}); },
        std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, before_tableau, {
    auto actual =
        PauliString<W>::from_str("-YYX").ref().before(GATE_DATA.at("CX").tableau<W>(), std::vector<size_t>{0, 1, 1, 2});
    ASSERT_EQ(actual, PauliString<W>::from_str("+XZ_"));

    ASSERT_THROW(
        {
            PauliString<W>::from_str("+XZ_").ref().before(
                GATE_DATA.at("CX").tableau<W>(), std::vector<size_t>{0, 1, 1});
        },
        std::invalid_argument);

    ASSERT_THROW(
        { PauliString<W>::from_str("+XZ_").ref().before(GATE_DATA.at("CX").tableau<W>(), std::vector<size_t>{0, 5}); },
        std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(pauli_string, left_mul_pauli, {
    PauliString<W> p(0);
    bool imag = false;

    p.left_mul_pauli(GateTarget::x(5), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("_____X"));

    p.left_mul_pauli(GateTarget::x(5), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("______"));

    p.left_mul_pauli(GateTarget::x(5), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("_____X"));

    p.left_mul_pauli(GateTarget::z(5), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("_____Y"));

    p.left_mul_pauli(GateTarget::z(5), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("_____X"));

    p.left_mul_pauli(GateTarget::y(5), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-_____Z"));

    p.left_mul_pauli(GateTarget::y(15), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-_____Z_________Y"));

    p.left_mul_pauli(GateTarget::y(15, true), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("_____Z__________"));
})

TEST_EACH_WORD_SIZE_W(pauli_string, left_mul_pauli_mul_table, {
    PauliString<W> p(1);
    bool imag = false;

    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::x(0), &imag);
    p.left_mul_pauli(GateTarget::y(0), &imag);
    p.left_mul_pauli(GateTarget::z(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-I"));

    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::z(0), &imag);
    p.left_mul_pauli(GateTarget::y(0), &imag);
    p.left_mul_pauli(GateTarget::x(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("I"));

    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::x(0), &imag);
    p.left_mul_pauli(GateTarget::x(0), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("I"));
    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::x(0), &imag);
    p.left_mul_pauli(GateTarget::y(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-Z"));
    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::x(0), &imag);
    p.left_mul_pauli(GateTarget::z(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("Y"));

    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::y(0), &imag);
    p.left_mul_pauli(GateTarget::x(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("Z"));
    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::y(0), &imag);
    p.left_mul_pauli(GateTarget::y(0), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("I"));
    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::y(0), &imag);
    p.left_mul_pauli(GateTarget::z(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-X"));

    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::z(0), &imag);
    p.left_mul_pauli(GateTarget::x(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-Y"));
    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::z(0), &imag);
    p.left_mul_pauli(GateTarget::y(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("X"));
    p = PauliString<W>(1);
    imag = false;
    p.left_mul_pauli(GateTarget::z(0), &imag);
    p.left_mul_pauli(GateTarget::z(0), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("I"));
})

TEST_EACH_WORD_SIZE_W(pauli_string, right_mul_pauli_mul_table, {
    PauliString<W> p(1);
    bool imag = false;

    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::x(0), &imag);
    p.right_mul_pauli(GateTarget::y(0), &imag);
    p.right_mul_pauli(GateTarget::z(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("+I"));

    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::z(0), &imag);
    p.right_mul_pauli(GateTarget::y(0), &imag);
    p.right_mul_pauli(GateTarget::x(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-I"));

    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::x(0), &imag);
    p.right_mul_pauli(GateTarget::x(0), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("I"));
    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::x(0), &imag);
    p.right_mul_pauli(GateTarget::y(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("Z"));
    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::x(0), &imag);
    p.right_mul_pauli(GateTarget::z(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-Y"));

    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::y(0), &imag);
    p.right_mul_pauli(GateTarget::x(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-Z"));
    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::y(0), &imag);
    p.right_mul_pauli(GateTarget::y(0), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("I"));
    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::y(0), &imag);
    p.right_mul_pauli(GateTarget::z(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("X"));

    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::z(0), &imag);
    p.right_mul_pauli(GateTarget::x(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("Y"));
    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::z(0), &imag);
    p.right_mul_pauli(GateTarget::y(0), &imag);
    ASSERT_EQ(imag, 1);
    ASSERT_EQ(p, PauliString<W>("-X"));
    p = PauliString<W>(1);
    imag = false;
    p.right_mul_pauli(GateTarget::z(0), &imag);
    p.right_mul_pauli(GateTarget::z(0), &imag);
    ASSERT_EQ(imag, 0);
    ASSERT_EQ(p, PauliString<W>("I"));
})
