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

#include "stim/stabilizers/tableau.h"

#include <random>

#include "gtest/gtest.h"

#include "stim/gates/gates.h"
#include "stim/mem/simd_word.test.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/tableau_transposed_raii.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

static float complex_distance(std::complex<float> a, std::complex<float> b) {
    auto d = a - b;
    return sqrtf(d.real() * d.real() + d.imag() * d.imag());
}

TEST_EACH_WORD_SIZE_W(tableau, identity, {
    auto t = Tableau<W>::identity(4);
    ASSERT_EQ(
        t.str(),
        "+-xz-xz-xz-xz-\n"
        "| ++ ++ ++ ++\n"
        "| XZ __ __ __\n"
        "| __ XZ __ __\n"
        "| __ __ XZ __\n"
        "| __ __ __ XZ");
})

TEST_EACH_WORD_SIZE_W(tableau, gate1, {
    auto gate1 = Tableau<W>::gate1("+X", "+Z");
    ASSERT_EQ(gate1.xs[0].str(), "+X");
    ASSERT_EQ(gate1.eval_y_obs(0).str(), "+Y");
    ASSERT_EQ(gate1.zs[0].str(), "+Z");
})

template <size_t W>
bool tableau_agrees_with_unitary(
    const Tableau<W> &tableau, const std::vector<std::vector<std::complex<float>>> &unitary) {
    auto n = tableau.num_qubits;
    assert(unitary.size() == 1ULL << n);

    std::vector<PauliString<W>> basis;
    for (size_t x = 0; x < 2; x++) {
        for (size_t k = 0; k < n; k++) {
            basis.emplace_back(n);
            if (x) {
                basis.back().xs[k] ^= 1;
            } else {
                basis.back().zs[k] ^= 1;
            }
        }
    }

    for (const auto &input_side_obs : basis) {
        VectorSimulator sim(n * 2);
        // Create EPR pairs to test all possible inputs via state channel duality.
        for (size_t q = 0; q < n; q++) {
            sim.apply(GateType::H, q);
            sim.apply(GateType::CX, q, q + n);
        }
        // Apply input-side observable.
        sim.apply<W>(input_side_obs, n);
        // Apply operation's unitary.
        std::vector<size_t> qs;
        for (size_t q = 0; q < n; q++) {
            qs.push_back(q + n);
        }
        sim.apply(unitary, {qs});
        // Apply output-side observable, which should cancel input-side.
        sim.apply<W>(tableau(input_side_obs), n);

        // Verify that the state encodes the unitary matrix, with the
        // input-side and output-side observables having perfectly cancelled out.
        auto scale = powf(0.5f, 0.5f * n);
        for (size_t row = 0; row < 1u << n; row++) {
            for (size_t col = 0; col < 1u << n; col++) {
                auto a = sim.state[(row << n) | col];
                auto b = unitary[row][col] * scale;
                if (complex_distance(a, b) > 1e-4) {
                    return false;
                }
            }
        }
    }

    return true;
}

TEST_EACH_WORD_SIZE_W(tableau, big_not_seeing_double, {
    Tableau<W> t(500);
    auto s = t.xs[0].str();
    size_t n = 0;
    for (size_t k = 1; k < s.size(); k++) {
        if (s[k] != '_') {
            n += 1;
        }
    }
    ASSERT_EQ(n, 1) << s;
})

TEST_EACH_WORD_SIZE_W(tableau, str, {
    ASSERT_EQ(
        Tableau<W>::gate1("+X", "-Z").str(),
        "+-xz-\n"
        "| +-\n"
        "| XZ");
    ASSERT_EQ(
        GATE_DATA.at("X").tableau<W>().str(),
        "+-xz-\n"
        "| +-\n"
        "| XZ");
    ASSERT_EQ(
        GATE_DATA.at("SQRT_Z").tableau<W>().str(),
        "+-xz-\n"
        "| ++\n"
        "| YZ");
    ASSERT_EQ(
        GATE_DATA.at("SQRT_Z_DAG").tableau<W>().str(),
        "+-xz-\n"
        "| -+\n"
        "| YZ");
    ASSERT_EQ(
        GATE_DATA.at("H_XZ").tableau<W>().str(),
        "+-xz-\n"
        "| ++\n"
        "| ZX");
    ASSERT_EQ(
        GATE_DATA.at("ZCX").tableau<W>().str(),
        "+-xz-xz-\n"
        "| ++ ++\n"
        "| XZ _Z\n"
        "| X_ XZ");

    Tableau<W> t(4);
    t.prepend_H_XZ(0);
    t.prepend_H_XZ(1);
    t.prepend_SQRT_Z(1);
    t.prepend_ZCX(0, 2);
    t.prepend_ZCX(1, 3);
    t.prepend_ZCX(0, 1);
    t.prepend_ZCX(1, 0);
    ASSERT_EQ(
        t.inverse().str(),
        "+-xz-xz-xz-xz-\n"
        "| ++ +- ++ ++\n"
        "| Z_ ZY _Z _Z\n"
        "| ZX _X _Z __\n"
        "| _X __ XZ __\n"
        "| __ _X __ XZ");
    ASSERT_EQ(
        t.inverse(true).str(),
        "+-xz-xz-xz-xz-\n"
        "| ++ ++ ++ ++\n"
        "| Z_ ZY _Z _Z\n"
        "| ZX _X _Z __\n"
        "| _X __ XZ __\n"
        "| __ _X __ XZ");
    ASSERT_EQ(
        t.str(),
        "+-xz-xz-xz-xz-\n"
        "| -+ ++ ++ ++\n"
        "| Z_ ZX _X __\n"
        "| YX _X __ _X\n"
        "| X_ X_ XZ __\n"
        "| X_ __ __ XZ");
})

TEST_EACH_WORD_SIZE_W(tableau, gate_tableau_data_vs_unitary_data, {
    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            EXPECT_TRUE(tableau_agrees_with_unitary<W>(gate.tableau<W>(), gate.unitary())) << gate.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(tableau, inverse_data, {
    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            auto &inv_gate = gate.inverse();
            auto tab = gate.tableau<W>();
            auto inv_tab = inv_gate.tableau<W>();
            ASSERT_EQ(tab.then(inv_tab), Tableau<W>::identity(tab.num_qubits)) << gate.name << " -> " << inv_gate.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(tableau, eval, {
    const auto &cnot = GATE_DATA.at("ZCX").tableau<W>();
    ASSERT_EQ(cnot(PauliString<W>::from_str("-XX")), PauliString<W>::from_str("-XI"));
    ASSERT_EQ(cnot(PauliString<W>::from_str("+XX")), PauliString<W>::from_str("+XI"));
    ASSERT_EQ(cnot(PauliString<W>::from_str("+ZZ")), PauliString<W>::from_str("+IZ"));
    ASSERT_EQ(cnot(PauliString<W>::from_str("+IY")), PauliString<W>::from_str("+ZY"));
    ASSERT_EQ(cnot(PauliString<W>::from_str("+YI")), PauliString<W>::from_str("+YX"));
    ASSERT_EQ(cnot(PauliString<W>::from_str("+YY")), PauliString<W>::from_str("-XZ"));

    const auto &x2 = GATE_DATA.at("SQRT_X").tableau<W>();
    ASSERT_EQ(x2(PauliString<W>::from_str("+X")), PauliString<W>::from_str("+X"));
    ASSERT_EQ(x2(PauliString<W>::from_str("+Y")), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(x2(PauliString<W>::from_str("+Z")), PauliString<W>::from_str("-Y"));

    const auto &s = GATE_DATA.at("SQRT_Z").tableau<W>();
    ASSERT_EQ(s(PauliString<W>::from_str("+X")), PauliString<W>::from_str("+Y"));
    ASSERT_EQ(s(PauliString<W>::from_str("+Y")), PauliString<W>::from_str("-X"));
    ASSERT_EQ(s(PauliString<W>::from_str("+Z")), PauliString<W>::from_str("+Z"));
})

TEST_EACH_WORD_SIZE_W(tableau, apply_within, {
    const auto &cnot = GATE_DATA.at("ZCX").tableau<W>();

    auto p1 = PauliString<W>::from_str("-XX");
    PauliStringRef<W> p1_ptr(p1);
    cnot.apply_within(p1_ptr, std::vector<size_t>{0, 1});
    ASSERT_EQ(p1, PauliString<W>::from_str("-XI"));

    auto p2 = PauliString<W>::from_str("+XX");
    PauliStringRef<W> p2_ptr(p2);
    cnot.apply_within(p2_ptr, std::vector<size_t>{0, 1});
    ASSERT_EQ(p2, PauliString<W>::from_str("+XI"));
})

TEST_EACH_WORD_SIZE_W(tableau, equality, {
    ASSERT_TRUE(GATE_DATA.at("SQRT_Z").tableau<W>() == GATE_DATA.at("SQRT_Z").tableau<W>());
    ASSERT_FALSE(GATE_DATA.at("SQRT_Z").tableau<W>() != GATE_DATA.at("SQRT_Z").tableau<W>());
    ASSERT_FALSE(GATE_DATA.at("SQRT_Z").tableau<W>() == GATE_DATA.at("ZCX").tableau<W>());
    ASSERT_TRUE(GATE_DATA.at("SQRT_Z").tableau<W>() != GATE_DATA.at("ZCX").tableau<W>());
    ASSERT_FALSE(GATE_DATA.at("SQRT_Z").tableau<W>() == GATE_DATA.at("SQRT_X").tableau<W>());
    ASSERT_TRUE(GATE_DATA.at("SQRT_Z").tableau<W>() != GATE_DATA.at("SQRT_X").tableau<W>());
    ASSERT_EQ(Tableau<W>(1), GATE_DATA.at("I").tableau<W>());
    ASSERT_NE(Tableau<W>(1), GATE_DATA.at("X").tableau<W>());
})

TEST_EACH_WORD_SIZE_W(tableau, inplace_scatter_append, {
    auto t1 = Tableau<W>::identity(1);
    t1.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau<W>(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("SQRT_Z").tableau<W>());
    t1.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau<W>(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("Z").tableau<W>());
    t1.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau<W>(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("SQRT_Z_DAG").tableau<W>());

    // Test swap decomposition into exp(i pi/2 (XX + YY + ZZ)).
    auto t2 = Tableau<W>::identity(2);
    // ZZ^0.5
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau<W>(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau<W>(), {1});
    t2.inplace_scatter_append(GATE_DATA.at("ZCZ").tableau<W>(), {0, 1});
    // YY^0.5
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_Y").tableau<W>(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_Y").tableau<W>(), {1});
    t2.inplace_scatter_append(GATE_DATA.at("H_YZ").tableau<W>(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("ZCY").tableau<W>(), {0, 1});
    t2.inplace_scatter_append(GATE_DATA.at("H_YZ").tableau<W>(), {0});
    // XX^0.5
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau<W>(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau<W>(), {1});
    t2.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau<W>(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("ZCX").tableau<W>(), {0, 1});
    t2.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau<W>(), {0});
    ASSERT_EQ(t2, GATE_DATA.at("SWAP").tableau<W>());

    // Test order dependence.
    auto t3 = Tableau<W>::identity(2);
    t3.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau<W>(), {0});
    t3.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau<W>(), {1});
    t3.inplace_scatter_append(GATE_DATA.at("ZCX").tableau<W>(), {0, 1});
    ASSERT_EQ(t3(PauliString<W>::from_str("XI")), PauliString<W>::from_str("ZI"));
    ASSERT_EQ(t3(PauliString<W>::from_str("ZI")), PauliString<W>::from_str("XX"));
    ASSERT_EQ(t3(PauliString<W>::from_str("IX")), PauliString<W>::from_str("IX"));
    ASSERT_EQ(t3(PauliString<W>::from_str("IZ")), PauliString<W>::from_str("-ZY"));
})

TEST_EACH_WORD_SIZE_W(tableau, inplace_scatter_prepend, {
    auto t1 = Tableau<W>::identity(1);
    t1.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau<W>(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("SQRT_Z").tableau<W>());
    t1.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau<W>(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("Z").tableau<W>());
    t1.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau<W>(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("SQRT_Z_DAG").tableau<W>());

    // Test swap decomposition into exp(i pi/2 (XX + YY + ZZ)).
    auto t2 = Tableau<W>::identity(2);
    // ZZ^0.5
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau<W>(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau<W>(), {1});
    t2.inplace_scatter_prepend(GATE_DATA.at("ZCZ").tableau<W>(), {0, 1});
    // YY^0.5
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_Y").tableau<W>(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_Y").tableau<W>(), {1});
    t2.inplace_scatter_prepend(GATE_DATA.at("H_YZ").tableau<W>(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("ZCY").tableau<W>(), {0, 1});
    t2.inplace_scatter_prepend(GATE_DATA.at("H_YZ").tableau<W>(), {0});
    // XX^0.5
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_X").tableau<W>(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_X").tableau<W>(), {1});
    t2.inplace_scatter_prepend(GATE_DATA.at("H_XZ").tableau<W>(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau<W>(), {0, 1});
    t2.inplace_scatter_prepend(GATE_DATA.at("H_XZ").tableau<W>(), {0});
    ASSERT_EQ(t2, GATE_DATA.at("SWAP").tableau<W>());

    // Test order dependence.
    auto t3 = Tableau<W>::identity(2);
    t3.inplace_scatter_prepend(GATE_DATA.at("H_XZ").tableau<W>(), {0});
    t3.inplace_scatter_prepend(GATE_DATA.at("SQRT_X").tableau<W>(), {1});
    t3.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau<W>(), {0, 1});
    ASSERT_EQ(t3(PauliString<W>::from_str("XI")), PauliString<W>::from_str("ZX"));
    ASSERT_EQ(t3(PauliString<W>::from_str("ZI")), PauliString<W>::from_str("XI"));
    ASSERT_EQ(t3(PauliString<W>::from_str("IX")), PauliString<W>::from_str("IX"));
    ASSERT_EQ(t3(PauliString<W>::from_str("IZ")), PauliString<W>::from_str("-XY"));
})

TEST_EACH_WORD_SIZE_W(tableau, eval_y, {
    ASSERT_EQ(GATE_DATA.at("H_XZ").tableau<W>().zs[0], PauliString<W>::from_str("+X"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Z").tableau<W>().zs[0], PauliString<W>::from_str("+Z"));
    ASSERT_EQ(GATE_DATA.at("H_YZ").tableau<W>().zs[0], PauliString<W>::from_str("+Y"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Y").tableau<W>().zs[0], PauliString<W>::from_str("X"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Y_DAG").tableau<W>().zs[0], PauliString<W>::from_str("-X"));
    ASSERT_EQ(GATE_DATA.at("ZCX").tableau<W>().zs[1], PauliString<W>::from_str("ZZ"));

    ASSERT_EQ(GATE_DATA.at("H_XZ").tableau<W>().eval_y_obs(0), PauliString<W>::from_str("-Y"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Z").tableau<W>().eval_y_obs(0), PauliString<W>::from_str("-X"));
    ASSERT_EQ(GATE_DATA.at("H_YZ").tableau<W>().eval_y_obs(0), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Y").tableau<W>().eval_y_obs(0), PauliString<W>::from_str("+Y"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Y_DAG").tableau<W>().eval_y_obs(0), PauliString<W>::from_str("+Y"));
    ASSERT_EQ(GATE_DATA.at("SQRT_X").tableau<W>().eval_y_obs(0), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(GATE_DATA.at("SQRT_X_DAG").tableau<W>().eval_y_obs(0), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(GATE_DATA.at("ZCX").tableau<W>().eval_y_obs(1), PauliString<W>::from_str("ZY"));
})

template <size_t W>
bool are_tableau_mutations_equivalent(
    size_t n,
    const std::function<void(Tableau<W> &t, const std::vector<size_t> &)> &mutation1,
    const std::function<void(Tableau<W> &t, const std::vector<size_t> &)> &mutation2) {
    auto rng = INDEPENDENT_TEST_RNG();
    auto test_tableau_dual = Tableau<W>::identity(2 * n);
    std::vector<size_t> targets1;
    std::vector<size_t> targets2;
    std::vector<size_t> targets3;
    for (size_t k = 0; k < n; k++) {
        test_tableau_dual.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau<W>(), {k});
        test_tableau_dual.inplace_scatter_append(GATE_DATA.at("ZCX").tableau<W>(), {k, k + n});
        targets1.push_back(k);
        targets2.push_back(k + n);
        targets3.push_back(k + (k % 2 == 0 ? 0 : n));
    }

    std::vector<Tableau<W>> tableaus{
        test_tableau_dual,
        Tableau<W>::random(n + 10, rng),
        Tableau<W>::random(n + 30, rng),
    };
    std::vector<std::vector<size_t>> cases{targets1, targets2, targets3};
    for (const auto &t : tableaus) {
        for (const auto &targets : cases) {
            auto t1 = t;
            auto t2 = t;
            mutation1(t1, targets);
            mutation2(t2, targets);
            if (t1 != t2) {
                return false;
            }
        }
    }
    return true;
}

template <size_t W>
bool are_tableau_prepends_equivalent(std::string_view name, const std::function<void(Tableau<W> &t, size_t)> &func) {
    return are_tableau_mutations_equivalent<W>(
        1,
        [&](Tableau<W> &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at(name).tableau<W>(), targets);
        },
        [&](Tableau<W> &t, const std::vector<size_t> &targets) {
            func(t, targets[0]);
        });
}

template <size_t W>
bool are_tableau_prepends_equivalent(
    std::string_view name, const std::function<void(Tableau<W> &t, size_t, size_t)> &func) {
    return are_tableau_mutations_equivalent<W>(
        2,
        [&](Tableau<W> &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at(name).tableau<W>(), targets);
        },
        [&](Tableau<W> &t, const std::vector<size_t> &targets) {
            func(t, targets[0], targets[1]);
        });
}

TEST_EACH_WORD_SIZE_W(tableau, check_invariants, {
    ASSERT_TRUE(Tableau<W>::gate1("X", "Z").satisfies_invariants());
    ASSERT_TRUE(Tableau<W>::gate2("XI", "ZI", "IX", "IZ").satisfies_invariants());
    ASSERT_FALSE(Tableau<W>::gate1("X", "X").satisfies_invariants());
    ASSERT_FALSE(Tableau<W>::gate2("XI", "ZI", "XI", "ZI").satisfies_invariants());
    ASSERT_FALSE(Tableau<W>::gate2("XI", "II", "IX", "IZ").satisfies_invariants());
})

TEST_EACH_WORD_SIZE_W(tableau, is_conjugation_by_pauli, {
    Tableau<W> tableau(8);
    ASSERT_TRUE(tableau.is_pauli_product());
    tableau.xs.signs[0] = true;
    tableau.xs.signs[3] = true;
    ASSERT_TRUE(tableau.is_pauli_product());
    tableau.xs.zt[0][2] = true;
    tableau.zs.zt[0][2] = true;
    ASSERT_FALSE(tableau.is_pauli_product());
})

TEST_EACH_WORD_SIZE_W(tableau, to_pauli_string, {
    Tableau<W> tableau(8);
    tableau.xs.signs[3] = true;
    auto pauli_string_z = tableau.to_pauli_string();
    ASSERT_EQ(pauli_string_z.str(), "+___Z____");
    tableau.zs.signs[3] = true;
    auto pauli_string_y = tableau.to_pauli_string();
    ASSERT_EQ(pauli_string_y.str(), "+___Y____");
    tableau.xs.signs[3] = false;
    tableau.xs.signs[5] = true;
    auto pauli_string_xz = tableau.to_pauli_string();
    ASSERT_EQ(pauli_string_xz.str(), "+___X_Z__");
    tableau.xs.zt[0][1] = true;
    ASSERT_THROW(tableau.to_pauli_string(), std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(tableau, from_pauli_string, {
    auto pauli_string_empty = PauliString<W>::from_str("");
    auto tableau_empty = Tableau<W>::from_pauli_string(pauli_string_empty);
    ASSERT_EQ(tableau_empty.to_pauli_string(), pauli_string_empty);
    auto pauli_string = PauliString<W>::from_str("+_XZX__YZZX");
    auto tableau = Tableau<W>::from_pauli_string(pauli_string);
    ASSERT_EQ(tableau.to_pauli_string(), pauli_string);
})

TEST_EACH_WORD_SIZE_W(tableau, random, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau<W>::random(1, rng);
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau<W>::random(2, rng);
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau<W>::random(3, rng);
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau<W>::random(30, rng);
        ASSERT_TRUE(t.satisfies_invariants());
    }
})

TEST_EACH_WORD_SIZE_W(tableau, specialized_operation, {
    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            1,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("X").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("X").tableau<W>(), targets);
            }));
    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            1,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau<W>(), targets);
            }));
    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            2,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau<W>(), targets);
            }));
    EXPECT_FALSE(
        are_tableau_mutations_equivalent<W>(
            1,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("H_XZ").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("SQRT_Y").tableau<W>(), targets);
            }));
    EXPECT_FALSE(
        are_tableau_mutations_equivalent<W>(
            2,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_prepend(GATE_DATA.at("ZCZ").tableau<W>(), targets);
            }));

    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("H_XZ", &Tableau<W>::prepend_H_XZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("H_YZ", &Tableau<W>::prepend_H_YZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("H_XY", &Tableau<W>::prepend_H_XY));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("X", &Tableau<W>::prepend_X));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("Y", &Tableau<W>::prepend_Y));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("Z", &Tableau<W>::prepend_Z));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("C_XYZ", &Tableau<W>::prepend_C_XYZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("C_ZYX", &Tableau<W>::prepend_C_ZYX));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_X", &Tableau<W>::prepend_SQRT_X));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_Y", &Tableau<W>::prepend_SQRT_Y));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_Z", &Tableau<W>::prepend_SQRT_Z));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_X_DAG", &Tableau<W>::prepend_SQRT_X_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_Y_DAG", &Tableau<W>::prepend_SQRT_Y_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_Z_DAG", &Tableau<W>::prepend_SQRT_Z_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SWAP", &Tableau<W>::prepend_SWAP));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("ZCX", &Tableau<W>::prepend_ZCX));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("ZCY", &Tableau<W>::prepend_ZCY));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("ZCZ", &Tableau<W>::prepend_ZCZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("ISWAP", &Tableau<W>::prepend_ISWAP));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("ISWAP_DAG", &Tableau<W>::prepend_ISWAP_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("XCX", &Tableau<W>::prepend_XCX));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("XCY", &Tableau<W>::prepend_XCY));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("XCZ", &Tableau<W>::prepend_XCZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("YCX", &Tableau<W>::prepend_YCX));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("YCY", &Tableau<W>::prepend_YCY));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("YCZ", &Tableau<W>::prepend_YCZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_XX", &Tableau<W>::prepend_SQRT_XX));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_XX_DAG", &Tableau<W>::prepend_SQRT_XX_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_YY", &Tableau<W>::prepend_SQRT_YY));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_YY_DAG", &Tableau<W>::prepend_SQRT_YY_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_ZZ", &Tableau<W>::prepend_SQRT_ZZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent<W>("SQRT_ZZ_DAG", &Tableau<W>::prepend_SQRT_ZZ_DAG));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            1,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("X").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_X(targets[0]);
            }));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            1,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_H_XZ(targets[0]);
            }));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            1,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("H_XY").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_H_XY(targets[0]);
            }));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            1,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("H_YZ").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_H_YZ(targets[0]);
            }));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            1,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("S").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_S(targets[0]);
            }));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            2,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("ZCX").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_ZCX(targets[0], targets[1]);
            }));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            2,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("ZCY").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_ZCY(targets[0], targets[1]);
            }));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            2,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("ZCZ").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_ZCZ(targets[0], targets[1]);
            }));

    EXPECT_TRUE(
        are_tableau_mutations_equivalent<W>(
            2,
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                t.inplace_scatter_append(GATE_DATA.at("SWAP").tableau<W>(), targets);
            },
            [](Tableau<W> &t, const std::vector<size_t> &targets) {
                TableauTransposedRaii<W>(t).append_SWAP(targets[0], targets[1]);
            }));
})

TEST_EACH_WORD_SIZE_W(tableau, expand, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = Tableau<W>::random(4, rng);
    auto t2 = t;
    for (size_t n = 8; n < 500; n += 255) {
        t2.expand(n, 1.0);
        ASSERT_EQ(t2.num_qubits, n);
        for (size_t k = 0; k < n; k++) {
            if (k < 4) {
                ASSERT_EQ(t.xs[k].sign, t2.xs[k].sign);
                ASSERT_EQ(t.zs[k].sign, t2.zs[k].sign);
            } else {
                ASSERT_EQ(t2.xs[k].sign, false);
                ASSERT_EQ(t2.zs[k].sign, false);
            }
            for (size_t k2 = 0; k2 < n; k2++) {
                if (k < 4 && k2 < 4) {
                    ASSERT_EQ(t.xs[k].xs[k2], t2.xs[k].xs[k2]);
                    ASSERT_EQ(t.xs[k].zs[k2], t2.xs[k].zs[k2]);
                    ASSERT_EQ(t.zs[k].xs[k2], t2.zs[k].xs[k2]);
                    ASSERT_EQ(t.zs[k].zs[k2], t2.zs[k].zs[k2]);
                } else if (k == k2) {
                    ASSERT_EQ(t2.xs[k].xs[k2], true);
                    ASSERT_EQ(t2.xs[k].zs[k2], false);
                    ASSERT_EQ(t2.zs[k].xs[k2], false);
                    ASSERT_EQ(t2.zs[k].zs[k2], true);
                } else {
                    ASSERT_EQ(t2.xs[k].xs[k2], false);
                    ASSERT_EQ(t2.xs[k].zs[k2], false);
                    ASSERT_EQ(t2.zs[k].xs[k2], false);
                    ASSERT_EQ(t2.zs[k].zs[k2], false);
                }
            }
        }
    }
})

TEST_EACH_WORD_SIZE_W(tableau, expand_pad, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = Tableau<W>::random(4, rng);
    auto t2 = t;
    size_t n = 8;
    while (n < 10000) {
        n++;
        t2.expand(n, 2);
    }

    ASSERT_EQ(t2.num_qubits, n);
    for (size_t k = 0; k < n; k++) {
        if (k < 4) {
            ASSERT_EQ(t.xs[k].sign, t2.xs[k].sign);
            ASSERT_EQ(t.zs[k].sign, t2.zs[k].sign);
        } else {
            ASSERT_EQ(t2.xs[k].sign, false);
            ASSERT_EQ(t2.zs[k].sign, false);
        }
        for (size_t k2 = 0; k2 < n; k2++) {
            if (k2 == 4 && k > 8) {
                k2 = k - 2;
            }
            if (k2 == k + 2) {
                k2 = n - 1;
            }
            if (k < 4 && k2 < 4) {
                ASSERT_EQ(t.xs[k].xs[k2], t2.xs[k].xs[k2]);
                ASSERT_EQ(t.xs[k].zs[k2], t2.xs[k].zs[k2]);
                ASSERT_EQ(t.zs[k].xs[k2], t2.zs[k].xs[k2]);
                ASSERT_EQ(t.zs[k].zs[k2], t2.zs[k].zs[k2]);
            } else if (k == k2) {
                ASSERT_EQ(t2.xs[k].xs[k2], true);
                ASSERT_EQ(t2.xs[k].zs[k2], false);
                ASSERT_EQ(t2.zs[k].xs[k2], false);
                ASSERT_EQ(t2.zs[k].zs[k2], true);
            } else {
                ASSERT_EQ(t2.xs[k].xs[k2], false);
                ASSERT_EQ(t2.xs[k].zs[k2], false);
                ASSERT_EQ(t2.zs[k].xs[k2], false);
                ASSERT_EQ(t2.zs[k].zs[k2], false);
            }
        }
    }
})

TEST_EACH_WORD_SIZE_W(tableau, expand_pad_equals, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = Tableau<W>::random(15, rng);
    auto t2 = t;
    t.expand(500, 1.0);
    t2.expand(500, 2.0);
    ASSERT_EQ(t, t2);
})

TEST_EACH_WORD_SIZE_W(tableau, transposed_access, {
    auto rng = INDEPENDENT_TEST_RNG();
    size_t n = W > 256 ? 1000 : 400;
    Tableau<W> t(n);
    auto m = t.xs.xt.data.num_bits_padded();
    t.xs.xt.data.randomize(m, rng);
    t.xs.zt.data.randomize(m, rng);
    t.zs.xt.data.randomize(m, rng);
    t.zs.zt.data.randomize(m, rng);
    for (size_t inp_qubit = 0; inp_qubit < n; inp_qubit += 99) {
        for (size_t out_qubit = 0; out_qubit < n; out_qubit += 99) {
            bool bxx = t.xs.xt[inp_qubit][out_qubit];
            bool bxz = t.xs.zt[inp_qubit][out_qubit];
            bool bzx = t.zs.xt[inp_qubit][out_qubit];
            bool bzz = t.zs.zt[inp_qubit][out_qubit];

            ASSERT_EQ(t.xs[inp_qubit].xs[out_qubit], bxx) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.xs[inp_qubit].zs[out_qubit], bxz) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.zs[inp_qubit].xs[out_qubit], bzx) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.zs[inp_qubit].zs[out_qubit], bzz) << inp_qubit << ", " << out_qubit;

            {
                TableauTransposedRaii<W> trans(t);
                ASSERT_EQ(t.xs.xt[out_qubit][inp_qubit], bxx);
                ASSERT_EQ(t.xs.zt[out_qubit][inp_qubit], bxz);
                ASSERT_EQ(t.zs.xt[out_qubit][inp_qubit], bzx);
                ASSERT_EQ(t.zs.zt[out_qubit][inp_qubit], bzz);
            }
        }
    }
})

TEST_EACH_WORD_SIZE_W(tableau, inverse, {
    auto rng = INDEPENDENT_TEST_RNG();
    Tableau<W> t1(1);
    ASSERT_EQ(t1, t1.inverse());
    t1.prepend_X(0);
    ASSERT_EQ(t1, GATE_DATA.at("X").tableau<W>());
    auto t2 = t1.inverse();
    ASSERT_EQ(t1, GATE_DATA.at("X").tableau<W>());
    ASSERT_EQ(t2, GATE_DATA.at("X").tableau<W>());

    for (size_t k = 5; k < 20; k += 7) {
        t1 = Tableau<W>::random(k, rng);
        t2 = t1.inverse();
        ASSERT_TRUE(t2.satisfies_invariants());
        auto p = PauliString<W>::random(k, rng);
        auto p2 = t1(t2(p));
        auto x1 = p.xs.str();
        auto x2 = p2.xs.str();
        auto z1 = p.zs.str();
        auto z2 = p2.zs.str();
        ASSERT_EQ(p, p2);
        ASSERT_EQ(p, t2(t1(p)));
    }
})

TEST_EACH_WORD_SIZE_W(tableau, prepend_pauli_product, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = Tableau<W>::random(6, rng);
    auto ref = t;
    t.prepend_pauli_product(PauliString<W>::from_str("_XYZ__"));
    ref.prepend_X(1);
    ref.prepend_Y(2);
    ref.prepend_Z(3);
    ASSERT_EQ(t, ref);
    t.prepend_pauli_product(PauliString<W>::from_str("Y_ZX__"));
    ref.prepend_X(3);
    ref.prepend_Y(0);
    ref.prepend_Z(2);
    ASSERT_EQ(t, ref);
})

TEST_EACH_WORD_SIZE_W(tableau, then, {
    auto cnot = GATE_DATA.at("CNOT").tableau<W>();
    auto swap = GATE_DATA.at("SWAP").tableau<W>();
    Tableau<W> hh(2);
    hh.inplace_scatter_append(GATE_DATA.at("H").tableau<W>(), {0});
    hh.inplace_scatter_append(GATE_DATA.at("H").tableau<W>(), {1});

    ASSERT_EQ(cnot.then(cnot), Tableau<W>(2));
    ASSERT_EQ(hh.then(cnot).then(hh), swap.then(cnot).then(swap));
    ASSERT_EQ(cnot.then(cnot), Tableau<W>(2));

    Tableau<W> t(2);
    t.inplace_scatter_append(GATE_DATA.at("SQRT_X_DAG").tableau<W>(), {1});
    t = t.then(GATE_DATA.at("CY").tableau<W>());
    t.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau<W>(), {1});
    ASSERT_EQ(t, GATE_DATA.at("CZ").tableau<W>());
})

TEST_EACH_WORD_SIZE_W(tableau, raised_to, {
    auto cnot = GATE_DATA.at("CNOT").tableau<W>();
    ASSERT_EQ(cnot.raised_to(-97268202), Tableau<W>(2));
    ASSERT_EQ(cnot.raised_to(-97268201), cnot);
    ASSERT_EQ(cnot.raised_to(-3), cnot);
    ASSERT_EQ(cnot.raised_to(-2), Tableau<W>(2));
    ASSERT_EQ(cnot.raised_to(-1), cnot);
    ASSERT_EQ(cnot.raised_to(0), Tableau<W>(2));
    ASSERT_EQ(cnot.raised_to(1), cnot);
    ASSERT_EQ(cnot.raised_to(2), Tableau<W>(2));
    ASSERT_EQ(cnot.raised_to(3), cnot);
    ASSERT_EQ(cnot.raised_to(4), Tableau<W>(2));
    ASSERT_EQ(cnot.raised_to(97268201), cnot);
    ASSERT_EQ(cnot.raised_to(97268202), Tableau<W>(2));

    auto s = GATE_DATA.at("S").tableau<W>();
    auto z = GATE_DATA.at("Z").tableau<W>();
    auto s_dag = GATE_DATA.at("S_DAG").tableau<W>();
    ASSERT_EQ(s.raised_to(4 * -437829 + 0), Tableau<W>(1));
    ASSERT_EQ(s.raised_to(4 * -437829 + 1), s);
    ASSERT_EQ(s.raised_to(4 * -437829 + 2), z);
    ASSERT_EQ(s.raised_to(4 * -437829 + 3), s_dag);
    ASSERT_EQ(s.raised_to(-5), s_dag);
    ASSERT_EQ(s.raised_to(-4), Tableau<W>(1));
    ASSERT_EQ(s.raised_to(-3), s);
    ASSERT_EQ(s.raised_to(-2), z);
    ASSERT_EQ(s.raised_to(-1), s_dag);
    ASSERT_EQ(s.raised_to(0), Tableau<W>(1));
    ASSERT_EQ(s.raised_to(1), s);
    ASSERT_EQ(s.raised_to(2), z);
    ASSERT_EQ(s.raised_to(3), s_dag);
    ASSERT_EQ(s.raised_to(4), Tableau<W>(1));
    ASSERT_EQ(s.raised_to(5), s);
    ASSERT_EQ(s.raised_to(6), z);
    ASSERT_EQ(s.raised_to(7), s_dag);
    ASSERT_EQ(s.raised_to(4 * 437829 + 0), Tableau<W>(1));
    ASSERT_EQ(s.raised_to(4 * 437829 + 1), s);
    ASSERT_EQ(s.raised_to(4 * 437829 + 2), z);
    ASSERT_EQ(s.raised_to(4 * 437829 + 3), s_dag);

    Tableau<W> p15(3);
    p15.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau<W>(), {0});
    p15.inplace_scatter_append(s, {2});
    p15.inplace_scatter_append(cnot, {0, 1});
    p15.inplace_scatter_append(cnot, {1, 2});
    for (size_t k = 1; k < 15; k++) {
        ASSERT_NE(p15.raised_to(k), Tableau<W>(3));
    }
    ASSERT_EQ(p15.raised_to(15), Tableau<W>(3));
    ASSERT_EQ(p15.raised_to(15 * 47321 + 4), p15.raised_to(4));
    ASSERT_EQ(p15.raised_to(15 * 47321 + 1), p15);
    ASSERT_EQ(p15.raised_to(15 * -47321 + 1), p15);
})

TEST_EACH_WORD_SIZE_W(tableau, transposed_xz_input, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = Tableau<W>::random(4, rng);
    PauliString<W> x0(0);
    PauliString<W> x1(0);
    {
        TableauTransposedRaii<W> tmp(t);
        x0 = tmp.unsigned_x_input(0);
        x1 = tmp.unsigned_x_input(1);
    }
    auto tx0 = t(x0);
    auto tx1 = t(x1);
    tx0.sign = false;
    tx1.sign = false;
    ASSERT_EQ(tx0, PauliString<W>::from_str("X___"));
    ASSERT_EQ(tx1, PauliString<W>::from_str("_X__"));
})

TEST_EACH_WORD_SIZE_W(tableau, direct_sum, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t1 = Tableau<W>::random(160, rng);
    auto t2 = Tableau<W>::random(170, rng);
    auto t3 = t1;
    t3 += t2;
    ASSERT_EQ(t3, t1 + t2);

    PauliString<W> p1 = t1.xs[5];
    p1.ensure_num_qubits(160 + 170, 1.0);
    ASSERT_EQ(t3.xs[5], p1);

    std::string p2 = t2.xs[6].str();
    std::string p3 = t3.xs[166].str();
    ASSERT_EQ(p2[0], p3[0]);
    p2 = p2.substr(1);
    p3 = p3.substr(1);
    for (size_t k = 0; k < 160; k++) {
        ASSERT_EQ(p3[k], '_');
    }
    for (size_t k = 0; k < 170; k++) {
        ASSERT_EQ(p3[160 + k], p2[k]);
    }
})

TEST_EACH_WORD_SIZE_W(tableau, pauli_access_methods, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = Tableau<W>::random(3, rng);
    auto t_inv = t.inverse();
    for (size_t i = 0; i < 3; i++) {
        auto x = t.xs[i];
        auto y = t.eval_y_obs(i);
        auto z = t.zs[i];
        for (size_t j = 0; j < 3; j++) {
            ASSERT_EQ(t.x_output_pauli_xyz(i, j), pauli_xz_to_xyz(x.xs[j], x.zs[j]));
            ASSERT_EQ(t.y_output_pauli_xyz(i, j), pauli_xz_to_xyz(y.xs[j], y.zs[j]));
            ASSERT_EQ(t.z_output_pauli_xyz(i, j), pauli_xz_to_xyz(z.xs[j], z.zs[j]));
            ASSERT_EQ(t_inv.inverse_x_output_pauli_xyz(i, j), pauli_xz_to_xyz(x.xs[j], x.zs[j]));
            ASSERT_EQ(t_inv.inverse_y_output_pauli_xyz(i, j), pauli_xz_to_xyz(y.xs[j], y.zs[j]));
            ASSERT_EQ(t_inv.inverse_z_output_pauli_xyz(i, j), pauli_xz_to_xyz(z.xs[j], z.zs[j]));
        }
    }

    t = Tableau<W>(3);
    t.xs[0] = PauliString<W>::from_str("+XXX");
    t.xs[1] = PauliString<W>::from_str("-XZY");
    t.xs[2] = PauliString<W>::from_str("+Z_Z");
    t.zs[0] = PauliString<W>::from_str("-_XZ");
    t.zs[1] = PauliString<W>::from_str("-_X_");
    t.zs[2] = PauliString<W>::from_str("-X__");

    ASSERT_EQ(t.x_output_pauli_xyz(0, 0), 1);
    ASSERT_EQ(t.x_output_pauli_xyz(0, 1), 1);
    ASSERT_EQ(t.x_output_pauli_xyz(0, 2), 1);
    ASSERT_EQ(t.x_output_pauli_xyz(1, 0), 1);
    ASSERT_EQ(t.x_output_pauli_xyz(1, 1), 3);
    ASSERT_EQ(t.x_output_pauli_xyz(1, 2), 2);
    ASSERT_EQ(t.x_output_pauli_xyz(2, 0), 3);
    ASSERT_EQ(t.x_output_pauli_xyz(2, 1), 0);
    ASSERT_EQ(t.x_output_pauli_xyz(2, 2), 3);

    ASSERT_EQ(t.z_output_pauli_xyz(0, 0), 0);
    ASSERT_EQ(t.z_output_pauli_xyz(0, 1), 1);
    ASSERT_EQ(t.z_output_pauli_xyz(0, 2), 3);
    ASSERT_EQ(t.z_output_pauli_xyz(1, 0), 0);
    ASSERT_EQ(t.z_output_pauli_xyz(1, 1), 1);
    ASSERT_EQ(t.z_output_pauli_xyz(1, 2), 0);
    ASSERT_EQ(t.z_output_pauli_xyz(2, 0), 1);
    ASSERT_EQ(t.z_output_pauli_xyz(2, 1), 0);
    ASSERT_EQ(t.z_output_pauli_xyz(2, 2), 0);

    t = t.inverse();
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(0, 0), 1);
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(0, 1), 1);
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(0, 2), 1);
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(1, 0), 1);
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(1, 1), 3);
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(1, 2), 2);
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(2, 0), 3);
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(2, 1), 0);
    ASSERT_EQ(t.inverse_x_output_pauli_xyz(2, 2), 3);

    ASSERT_EQ(t.inverse_z_output_pauli_xyz(0, 0), 0);
    ASSERT_EQ(t.inverse_z_output_pauli_xyz(0, 1), 1);
    ASSERT_EQ(t.inverse_z_output_pauli_xyz(0, 2), 3);
    ASSERT_EQ(t.inverse_z_output_pauli_xyz(1, 0), 0);
    ASSERT_EQ(t.inverse_z_output_pauli_xyz(1, 1), 1);
    ASSERT_EQ(t.inverse_z_output_pauli_xyz(1, 2), 0);
    ASSERT_EQ(t.inverse_z_output_pauli_xyz(2, 0), 1);
    ASSERT_EQ(t.inverse_z_output_pauli_xyz(2, 1), 0);
    ASSERT_EQ(t.inverse_z_output_pauli_xyz(2, 2), 0);
})

TEST_EACH_WORD_SIZE_W(tableau, inverse_pauli_string_acces_methods, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = Tableau<W>::random(5, rng);
    auto t_inv = t.inverse();
    auto y0 = t_inv.eval_y_obs(0);
    auto y1 = t_inv.eval_y_obs(1);
    auto y2 = t_inv.eval_y_obs(2);
    ASSERT_EQ(t.inverse_x_output(0), t_inv.xs[0]);
    ASSERT_EQ(t.inverse_x_output(1), t_inv.xs[1]);
    ASSERT_EQ(t.inverse_x_output(2), t_inv.xs[2]);
    ASSERT_EQ(t.inverse_y_output(0), y0);
    ASSERT_EQ(t.inverse_y_output(1), y1);
    ASSERT_EQ(t.inverse_y_output(2), y2);
    ASSERT_EQ(t.inverse_z_output(0), t_inv.zs[0]);
    ASSERT_EQ(t.inverse_z_output(1), t_inv.zs[1]);
    ASSERT_EQ(t.inverse_z_output(2), t_inv.zs[2]);

    t_inv.xs.signs.clear();
    t_inv.zs.signs.clear();
    y0.sign = false;
    y1.sign = false;
    y2.sign = false;
    ASSERT_EQ(t.inverse_x_output(0, true), t_inv.xs[0]);
    ASSERT_EQ(t.inverse_x_output(1, true), t_inv.xs[1]);
    ASSERT_EQ(t.inverse_x_output(2, true), t_inv.xs[2]);
    ASSERT_EQ(t.inverse_y_output(0, true), y0);
    ASSERT_EQ(t.inverse_y_output(1, true), y1);
    ASSERT_EQ(t.inverse_y_output(2, true), y2);
    ASSERT_EQ(t.inverse_z_output(0, true), t_inv.zs[0]);
    ASSERT_EQ(t.inverse_z_output(1, true), t_inv.zs[1]);
    ASSERT_EQ(t.inverse_z_output(2, true), t_inv.zs[2]);
})

TEST_EACH_WORD_SIZE_W(tableau, unitary_little_endian, {
    Tableau<W> t(1);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{1, 0, 0, 1}));
    t.prepend_SQRT_Y(0);
    auto s = sqrtf(0.5);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{s, -s, s, s}));
    t.prepend_SQRT_Y(0);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{0, 1, -1, 0}));
    t.prepend_SQRT_Y(0);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{s, s, -s, s}));

    t = Tableau<W>(2);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}));
    t.prepend_X(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0}));
    t.prepend_X(0);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0}));
    t.prepend_X(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0}));
    t.prepend_X(0);

    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}));
    t.prepend_Z(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1}));
    t.prepend_Z(0);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1}));
    t.prepend_Z(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1}));
    t.prepend_Z(0);

    t.prepend_SQRT_Z(0);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, {0, 1}, 0, 0, 0, 0, {0, 1}}));
    t.prepend_SQRT_Z_DAG(0);

    t.prepend_H_XZ(0);
    t.prepend_H_XZ(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(false),
        (std::vector<std::complex<float>>{
            0.5,
            0.5,
            0.5,
            0.5,
            0.5,
            -0.5,
            0.5,
            -0.5,
            0.5,
            0.5,
            -0.5,
            -0.5,
            0.5,
            -0.5,
            -0.5,
            0.5,
        }));
})

TEST_EACH_WORD_SIZE_W(tableau, unitary_big_endian, {
    Tableau<W> t(1);
    ASSERT_EQ(t.to_flat_unitary_matrix(true), (std::vector<std::complex<float>>{1, 0, 0, 1}));
    t.prepend_SQRT_Y(0);
    auto s = sqrtf(0.5);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{s, -s, s, s}));
    t.prepend_SQRT_Y(0);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{0, 1, -1, 0}));
    t.prepend_SQRT_Y(0);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{s, s, -s, s}));

    t = Tableau<W>(2);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}));
    t.prepend_X(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0}));
    t.prepend_X(0);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0}));
    t.prepend_X(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0}));
    t.prepend_X(0);

    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}));
    t.prepend_Z(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1}));
    t.prepend_Z(0);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1}));
    t.prepend_Z(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1}));
    t.prepend_Z(0);

    t.prepend_SQRT_Z(0);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{1, 0, 0, 0, 0, {0, 1}, 0, 0, 0, 0, 1, 0, 0, 0, 0, {0, 1}}));
    t.prepend_SQRT_Z_DAG(0);

    t.prepend_H_XZ(0);
    t.prepend_H_XZ(1);
    ASSERT_EQ(
        t.to_flat_unitary_matrix(true),
        (std::vector<std::complex<float>>{
            0.5,
            0.5,
            0.5,
            0.5,
            0.5,
            -0.5,
            0.5,
            -0.5,
            0.5,
            0.5,
            -0.5,
            -0.5,
            0.5,
            -0.5,
            -0.5,
            0.5,
        }));
})

TEST_EACH_WORD_SIZE_W(tableau, unitary_vs_gate_data, {
    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            std::vector<std::complex<float>> flat_expected;
            for (const auto &row : gate.unitary()) {
                flat_expected.insert(flat_expected.end(), row.begin(), row.end());
            }
            VectorSimulator v(0);
            v.state = std::move(flat_expected);
            v.canonicalize_assuming_stabilizer_state((gate.flags & stim::GATE_TARGETS_PAIRS) ? 4 : 2);
            EXPECT_EQ(gate.tableau<W>().to_flat_unitary_matrix(true), v.state) << gate.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(tableau, inverse_not_confused_by_size_padding, {
    // Create a tableau where the avoid-quadratic-overhead padding causes it to pad over a simd word size boundary.
    Tableau<W> t(1);
    t += Tableau<W>(500);

    // Check that inverting it doesn't produce garbage.
    Tableau<W> t_inv = t.inverse();
    ASSERT_EQ(t_inv, t);
})
