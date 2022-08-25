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

#include "stim/circuit/gate_data.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/tableau_transposed_raii.h"
#include "stim/test_util.test.h"

using namespace stim;

static float complex_distance(std::complex<float> a, std::complex<float> b) {
    auto d = a - b;
    return sqrtf(d.real() * d.real() + d.imag() * d.imag());
}

TEST(tableau, identity) {
    auto t = Tableau::identity(4);
    ASSERT_EQ(
        t.str(),
        "+-xz-xz-xz-xz-\n"
        "| ++ ++ ++ ++\n"
        "| XZ __ __ __\n"
        "| __ XZ __ __\n"
        "| __ __ XZ __\n"
        "| __ __ __ XZ");
}

TEST(tableau, gate1) {
    auto gate1 = Tableau::gate1("+X", "+Z");
    ASSERT_EQ(gate1.xs[0].str(), "+X");
    ASSERT_EQ(gate1.eval_y_obs(0).str(), "+Y");
    ASSERT_EQ(gate1.zs[0].str(), "+Z");
}

bool tableau_agrees_with_unitary(const Tableau &tableau, const std::vector<std::vector<std::complex<float>>> &unitary) {
    auto n = tableau.num_qubits;
    assert(unitary.size() == 1ULL << n);

    std::vector<PauliString> basis;
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
            sim.apply("H_XZ", q);
            sim.apply("ZCX", q, q + n);
        }
        // Apply input-side observable.
        sim.apply(input_side_obs, n);
        // Apply operation's unitary.
        std::vector<size_t> qs;
        for (size_t q = 0; q < n; q++) {
            qs.push_back(q + n);
        }
        sim.apply(unitary, {qs});
        // Apply output-side observable, which should cancel input-side.
        sim.apply(tableau(input_side_obs), n);

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

TEST(tableau, big_not_seeing_double) {
    Tableau t(500);
    auto s = t.xs[0].str();
    size_t n = 0;
    for (size_t k = 1; k < s.size(); k++) {
        if (s[k] != '_') {
            n += 1;
        }
    }
    ASSERT_EQ(n, 1) << s;
}

TEST(tableau, str) {
    ASSERT_EQ(
        Tableau::gate1("+X", "-Z").str(),
        "+-xz-\n"
        "| +-\n"
        "| XZ");
    ASSERT_EQ(
        GATE_DATA.at("X").tableau().str(),
        "+-xz-\n"
        "| +-\n"
        "| XZ");
    ASSERT_EQ(
        GATE_DATA.at("SQRT_Z").tableau().str(),
        "+-xz-\n"
        "| ++\n"
        "| YZ");
    ASSERT_EQ(
        GATE_DATA.at("SQRT_Z_DAG").tableau().str(),
        "+-xz-\n"
        "| -+\n"
        "| YZ");
    ASSERT_EQ(
        GATE_DATA.at("H_XZ").tableau().str(),
        "+-xz-\n"
        "| ++\n"
        "| ZX");
    ASSERT_EQ(
        GATE_DATA.at("ZCX").tableau().str(),
        "+-xz-xz-\n"
        "| ++ ++\n"
        "| XZ _Z\n"
        "| X_ XZ");

    Tableau t(4);
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
}

TEST(tableau, gate_tableau_data_vs_unitary_data) {
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            EXPECT_TRUE(tableau_agrees_with_unitary(gate.tableau(), gate.unitary())) << gate.name;
        }
    }
}

TEST(tableau, inverse_data) {
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            auto &inv_gate = gate.inverse();
            Tableau tab = gate.tableau();
            Tableau inv_tab = inv_gate.tableau();
            ASSERT_EQ(tab.then(inv_tab), Tableau::identity(tab.num_qubits)) << gate.name << " -> " << inv_gate.name;
        }
    }
}

TEST(tableau, eval) {
    const auto &cnot = GATE_DATA.at("ZCX").tableau();
    ASSERT_EQ(cnot(PauliString::from_str("-XX")), PauliString::from_str("-XI"));
    ASSERT_EQ(cnot(PauliString::from_str("+XX")), PauliString::from_str("+XI"));
    ASSERT_EQ(cnot(PauliString::from_str("+ZZ")), PauliString::from_str("+IZ"));
    ASSERT_EQ(cnot(PauliString::from_str("+IY")), PauliString::from_str("+ZY"));
    ASSERT_EQ(cnot(PauliString::from_str("+YI")), PauliString::from_str("+YX"));
    ASSERT_EQ(cnot(PauliString::from_str("+YY")), PauliString::from_str("-XZ"));

    const auto &x2 = GATE_DATA.at("SQRT_X").tableau();
    ASSERT_EQ(x2(PauliString::from_str("+X")), PauliString::from_str("+X"));
    ASSERT_EQ(x2(PauliString::from_str("+Y")), PauliString::from_str("+Z"));
    ASSERT_EQ(x2(PauliString::from_str("+Z")), PauliString::from_str("-Y"));

    const auto &s = GATE_DATA.at("SQRT_Z").tableau();
    ASSERT_EQ(s(PauliString::from_str("+X")), PauliString::from_str("+Y"));
    ASSERT_EQ(s(PauliString::from_str("+Y")), PauliString::from_str("-X"));
    ASSERT_EQ(s(PauliString::from_str("+Z")), PauliString::from_str("+Z"));
}

TEST(tableau, apply_within) {
    const auto &cnot = GATE_DATA.at("ZCX").tableau();

    auto p1 = PauliString::from_str("-XX");
    PauliStringRef p1_ptr(p1);
    cnot.apply_within(p1_ptr, {0, 1});
    ASSERT_EQ(p1, PauliString::from_str("-XI"));

    auto p2 = PauliString::from_str("+XX");
    PauliStringRef p2_ptr(p2);
    cnot.apply_within(p2_ptr, {0, 1});
    ASSERT_EQ(p2, PauliString::from_str("+XI"));
}

TEST(tableau, equality) {
    ASSERT_TRUE(GATE_DATA.at("SQRT_Z").tableau() == GATE_DATA.at("SQRT_Z").tableau());
    ASSERT_FALSE(GATE_DATA.at("SQRT_Z").tableau() != GATE_DATA.at("SQRT_Z").tableau());
    ASSERT_FALSE(GATE_DATA.at("SQRT_Z").tableau() == GATE_DATA.at("ZCX").tableau());
    ASSERT_TRUE(GATE_DATA.at("SQRT_Z").tableau() != GATE_DATA.at("ZCX").tableau());
    ASSERT_FALSE(GATE_DATA.at("SQRT_Z").tableau() == GATE_DATA.at("SQRT_X").tableau());
    ASSERT_TRUE(GATE_DATA.at("SQRT_Z").tableau() != GATE_DATA.at("SQRT_X").tableau());
    ASSERT_EQ(Tableau(1), GATE_DATA.at("I").tableau());
    ASSERT_NE(Tableau(1), GATE_DATA.at("X").tableau());
}

TEST(tableau, inplace_scatter_append) {
    auto t1 = Tableau::identity(1);
    t1.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("SQRT_Z").tableau());
    t1.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("Z").tableau());
    t1.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("SQRT_Z_DAG").tableau());

    // Test swap decomposition into exp(i pi/2 (XX + YY + ZZ)).
    auto t2 = Tableau::identity(2);
    // ZZ^0.5
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_Z").tableau(), {1});
    t2.inplace_scatter_append(GATE_DATA.at("ZCZ").tableau(), {0, 1});
    // YY^0.5
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_Y").tableau(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_Y").tableau(), {1});
    t2.inplace_scatter_append(GATE_DATA.at("H_YZ").tableau(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("ZCY").tableau(), {0, 1});
    t2.inplace_scatter_append(GATE_DATA.at("H_YZ").tableau(), {0});
    // XX^0.5
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau(), {1});
    t2.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau(), {0});
    t2.inplace_scatter_append(GATE_DATA.at("ZCX").tableau(), {0, 1});
    t2.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau(), {0});
    ASSERT_EQ(t2, GATE_DATA.at("SWAP").tableau());

    // Test order dependence.
    auto t3 = Tableau::identity(2);
    t3.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau(), {0});
    t3.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau(), {1});
    t3.inplace_scatter_append(GATE_DATA.at("ZCX").tableau(), {0, 1});
    ASSERT_EQ(t3(PauliString::from_str("XI")), PauliString::from_str("ZI"));
    ASSERT_EQ(t3(PauliString::from_str("ZI")), PauliString::from_str("XX"));
    ASSERT_EQ(t3(PauliString::from_str("IX")), PauliString::from_str("IX"));
    ASSERT_EQ(t3(PauliString::from_str("IZ")), PauliString::from_str("-ZY"));
}

TEST(tableau, inplace_scatter_prepend) {
    auto t1 = Tableau::identity(1);
    t1.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("SQRT_Z").tableau());
    t1.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("Z").tableau());
    t1.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau(), {0});
    ASSERT_EQ(t1, GATE_DATA.at("SQRT_Z_DAG").tableau());

    // Test swap decomposition into exp(i pi/2 (XX + YY + ZZ)).
    auto t2 = Tableau::identity(2);
    // ZZ^0.5
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau(), {1});
    t2.inplace_scatter_prepend(GATE_DATA.at("ZCZ").tableau(), {0, 1});
    // YY^0.5
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_Y").tableau(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_Y").tableau(), {1});
    t2.inplace_scatter_prepend(GATE_DATA.at("H_YZ").tableau(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("ZCY").tableau(), {0, 1});
    t2.inplace_scatter_prepend(GATE_DATA.at("H_YZ").tableau(), {0});
    // XX^0.5
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_X").tableau(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("SQRT_X").tableau(), {1});
    t2.inplace_scatter_prepend(GATE_DATA.at("H_XZ").tableau(), {0});
    t2.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau(), {0, 1});
    t2.inplace_scatter_prepend(GATE_DATA.at("H_XZ").tableau(), {0});
    ASSERT_EQ(t2, GATE_DATA.at("SWAP").tableau());

    // Test order dependence.
    auto t3 = Tableau::identity(2);
    t3.inplace_scatter_prepend(GATE_DATA.at("H_XZ").tableau(), {0});
    t3.inplace_scatter_prepend(GATE_DATA.at("SQRT_X").tableau(), {1});
    t3.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau(), {0, 1});
    ASSERT_EQ(t3(PauliString::from_str("XI")), PauliString::from_str("ZX"));
    ASSERT_EQ(t3(PauliString::from_str("ZI")), PauliString::from_str("XI"));
    ASSERT_EQ(t3(PauliString::from_str("IX")), PauliString::from_str("IX"));
    ASSERT_EQ(t3(PauliString::from_str("IZ")), PauliString::from_str("-XY"));
}

TEST(tableau, eval_y) {
    ASSERT_EQ(GATE_DATA.at("H_XZ").tableau().zs[0], PauliString::from_str("+X"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Z").tableau().zs[0], PauliString::from_str("+Z"));
    ASSERT_EQ(GATE_DATA.at("H_YZ").tableau().zs[0], PauliString::from_str("+Y"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Y").tableau().zs[0], PauliString::from_str("X"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Y_DAG").tableau().zs[0], PauliString::from_str("-X"));
    ASSERT_EQ(GATE_DATA.at("ZCX").tableau().zs[1], PauliString::from_str("ZZ"));

    ASSERT_EQ(GATE_DATA.at("H_XZ").tableau().eval_y_obs(0), PauliString::from_str("-Y"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Z").tableau().eval_y_obs(0), PauliString::from_str("-X"));
    ASSERT_EQ(GATE_DATA.at("H_YZ").tableau().eval_y_obs(0), PauliString::from_str("+Z"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Y").tableau().eval_y_obs(0), PauliString::from_str("+Y"));
    ASSERT_EQ(GATE_DATA.at("SQRT_Y_DAG").tableau().eval_y_obs(0), PauliString::from_str("+Y"));
    ASSERT_EQ(GATE_DATA.at("SQRT_X").tableau().eval_y_obs(0), PauliString::from_str("+Z"));
    ASSERT_EQ(GATE_DATA.at("SQRT_X_DAG").tableau().eval_y_obs(0), PauliString::from_str("-Z"));
    ASSERT_EQ(GATE_DATA.at("ZCX").tableau().eval_y_obs(1), PauliString::from_str("ZY"));
}

bool are_tableau_mutations_equivalent(
    size_t n,
    const std::function<void(Tableau &t, const std::vector<size_t> &)> &mutation1,
    const std::function<void(Tableau &t, const std::vector<size_t> &)> &mutation2) {
    auto test_tableau_dual = Tableau::identity(2 * n);
    std::vector<size_t> targets1;
    std::vector<size_t> targets2;
    std::vector<size_t> targets3;
    for (size_t k = 0; k < n; k++) {
        test_tableau_dual.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau(), {k});
        test_tableau_dual.inplace_scatter_append(GATE_DATA.at("ZCX").tableau(), {k, k + n});
        targets1.push_back(k);
        targets2.push_back(k + n);
        targets3.push_back(k + (k % 2 == 0 ? 0 : n));
    }

    std::vector<Tableau> tableaus{
        test_tableau_dual,
        Tableau::random(n + 10, SHARED_TEST_RNG()),
        Tableau::random(n + 30, SHARED_TEST_RNG()),
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

bool are_tableau_prepends_equivalent(const std::string &name, const std::function<void(Tableau &t, size_t)> &func) {
    return are_tableau_mutations_equivalent(
        1,
        [&](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at(name).tableau(), targets);
        },
        [&](Tableau &t, const std::vector<size_t> &targets) {
            func(t, targets[0]);
        });
}

bool are_tableau_prepends_equivalent(
    const std::string &name, const std::function<void(Tableau &t, size_t, size_t)> &func) {
    return are_tableau_mutations_equivalent(
        2,
        [&](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at(name).tableau(), targets);
        },
        [&](Tableau &t, const std::vector<size_t> &targets) {
            func(t, targets[0], targets[1]);
        });
}

TEST(tableau, check_invariants) {
    ASSERT_TRUE(Tableau::gate1("X", "Z").satisfies_invariants());
    ASSERT_TRUE(Tableau::gate2("XI", "ZI", "IX", "IZ").satisfies_invariants());
    ASSERT_FALSE(Tableau::gate1("X", "X").satisfies_invariants());
    ASSERT_FALSE(Tableau::gate2("XI", "ZI", "XI", "ZI").satisfies_invariants());
    ASSERT_FALSE(Tableau::gate2("XI", "II", "IX", "IZ").satisfies_invariants());
}

TEST(tableau, is_conjugation_by_pauli) {
    Tableau tableau(8);
    ASSERT_TRUE(tableau.is_pauli_product());
    tableau.xs.signs[0] = true;
    tableau.xs.signs[3] = true;
    ASSERT_TRUE(tableau.is_pauli_product());
    tableau.xs.zt[0][2] = true;
    tableau.zs.zt[0][2] = true;
    ASSERT_FALSE(tableau.is_pauli_product());
}

TEST(tableau, to_pauli_string) {
    Tableau tableau(8);
    tableau.xs.signs[3] = true;
    PauliString pauli_string_z = tableau.to_pauli_string();
    ASSERT_EQ(pauli_string_z.str(), "+___Z____");
    tableau.zs.signs[3] = true;
    PauliString pauli_string_y = tableau.to_pauli_string();
    ASSERT_EQ(pauli_string_y.str(), "+___Y____");
    tableau.xs.signs[3] = false;
    tableau.xs.signs[5] = true;
    PauliString pauli_string_xz = tableau.to_pauli_string();
    ASSERT_EQ(pauli_string_xz.str(), "+___X_Z__");
    tableau.xs.zt[0][1] = true;
    ASSERT_THROW(tableau.to_pauli_string(), std::invalid_argument);
}

TEST(tableau, from_pauli_string) {
    PauliString pauli_string_empty = PauliString::from_str("");
    Tableau tableau_empty = Tableau::from_pauli_string(pauli_string_empty);
    ASSERT_EQ(tableau_empty.to_pauli_string(), pauli_string_empty);
    PauliString pauli_string = PauliString::from_str("+_XZX__YZZX");
    Tableau tableau = Tableau::from_pauli_string(pauli_string);
    ASSERT_EQ(tableau.to_pauli_string(), pauli_string);
}

TEST(tableau, random) {
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau::random(1, SHARED_TEST_RNG());
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau::random(2, SHARED_TEST_RNG());
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau::random(3, SHARED_TEST_RNG());
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau::random(30, SHARED_TEST_RNG());
        ASSERT_TRUE(t.satisfies_invariants());
    }
}

TEST(tableau, specialized_operations) {
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("X").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("X").tableau(), targets);
        }));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("SQRT_Z").tableau(), targets);
        }));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau(), targets);
        }));
    EXPECT_FALSE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("H_XZ").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("SQRT_Y").tableau(), targets);
        }));
    EXPECT_FALSE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("ZCX").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_prepend(GATE_DATA.at("ZCZ").tableau(), targets);
        }));

    EXPECT_TRUE(are_tableau_prepends_equivalent("H_XZ", &Tableau::prepend_H_XZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent("H_YZ", &Tableau::prepend_H_YZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent("H_XY", &Tableau::prepend_H_XY));
    EXPECT_TRUE(are_tableau_prepends_equivalent("X", &Tableau::prepend_X));
    EXPECT_TRUE(are_tableau_prepends_equivalent("Y", &Tableau::prepend_Y));
    EXPECT_TRUE(are_tableau_prepends_equivalent("Z", &Tableau::prepend_Z));
    EXPECT_TRUE(are_tableau_prepends_equivalent("C_XYZ", &Tableau::prepend_C_XYZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent("C_ZYX", &Tableau::prepend_C_ZYX));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_X", &Tableau::prepend_SQRT_X));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_Y", &Tableau::prepend_SQRT_Y));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_Z", &Tableau::prepend_SQRT_Z));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_X_DAG", &Tableau::prepend_SQRT_X_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_Y_DAG", &Tableau::prepend_SQRT_Y_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_Z_DAG", &Tableau::prepend_SQRT_Z_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SWAP", &Tableau::prepend_SWAP));
    EXPECT_TRUE(are_tableau_prepends_equivalent("ZCX", &Tableau::prepend_ZCX));
    EXPECT_TRUE(are_tableau_prepends_equivalent("ZCY", &Tableau::prepend_ZCY));
    EXPECT_TRUE(are_tableau_prepends_equivalent("ZCZ", &Tableau::prepend_ZCZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent("ISWAP", &Tableau::prepend_ISWAP));
    EXPECT_TRUE(are_tableau_prepends_equivalent("ISWAP_DAG", &Tableau::prepend_ISWAP_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent("XCX", &Tableau::prepend_XCX));
    EXPECT_TRUE(are_tableau_prepends_equivalent("XCY", &Tableau::prepend_XCY));
    EXPECT_TRUE(are_tableau_prepends_equivalent("XCZ", &Tableau::prepend_XCZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent("YCX", &Tableau::prepend_YCX));
    EXPECT_TRUE(are_tableau_prepends_equivalent("YCY", &Tableau::prepend_YCY));
    EXPECT_TRUE(are_tableau_prepends_equivalent("YCZ", &Tableau::prepend_YCZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_XX", &Tableau::prepend_SQRT_XX));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_XX_DAG", &Tableau::prepend_SQRT_XX_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_YY", &Tableau::prepend_SQRT_YY));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_YY_DAG", &Tableau::prepend_SQRT_YY_DAG));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_ZZ", &Tableau::prepend_SQRT_ZZ));
    EXPECT_TRUE(are_tableau_prepends_equivalent("SQRT_ZZ_DAG", &Tableau::prepend_SQRT_ZZ_DAG));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("X").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_X(targets[0]);
        }));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("H_XZ").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_H_XZ(targets[0]);
        }));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("H_XY").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_H_XY(targets[0]);
        }));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("H_YZ").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_H_YZ(targets[0]);
        }));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("S").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_S(targets[0]);
        }));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("ZCX").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_ZCX(targets[0], targets[1]);
        }));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("ZCY").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_ZCY(targets[0], targets[1]);
        }));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("ZCZ").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_ZCZ(targets[0], targets[1]);
        }));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets) {
            t.inplace_scatter_append(GATE_DATA.at("SWAP").tableau(), targets);
        },
        [](Tableau &t, const std::vector<size_t> &targets) {
            TableauTransposedRaii(t).append_SWAP(targets[0], targets[1]);
        }));
}

TEST(tableau, expand) {
    auto t = Tableau::random(4, SHARED_TEST_RNG());
    auto t2 = t;
    for (size_t n = 8; n < 500; n += 255) {
        t2.expand(n);
        assert(t2.num_qubits == n);
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
}

TEST(tableau, transposed_access) {
    size_t n = 1000;
    Tableau t(n);
    auto m = t.xs.xt.data.num_bits_padded();
    t.xs.xt.data.randomize(m, SHARED_TEST_RNG());
    t.xs.zt.data.randomize(m, SHARED_TEST_RNG());
    t.zs.xt.data.randomize(m, SHARED_TEST_RNG());
    t.zs.zt.data.randomize(m, SHARED_TEST_RNG());
    for (size_t inp_qubit = 0; inp_qubit < 1000; inp_qubit += 99) {
        for (size_t out_qubit = 0; out_qubit < 1000; out_qubit += 99) {
            bool bxx = t.xs.xt[inp_qubit][out_qubit];
            bool bxz = t.xs.zt[inp_qubit][out_qubit];
            bool bzx = t.zs.xt[inp_qubit][out_qubit];
            bool bzz = t.zs.zt[inp_qubit][out_qubit];

            ASSERT_EQ(t.xs[inp_qubit].xs[out_qubit], bxx) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.xs[inp_qubit].zs[out_qubit], bxz) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.zs[inp_qubit].xs[out_qubit], bzx) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.zs[inp_qubit].zs[out_qubit], bzz) << inp_qubit << ", " << out_qubit;

            {
                TableauTransposedRaii trans(t);
                ASSERT_EQ(t.xs.xt[out_qubit][inp_qubit], bxx);
                ASSERT_EQ(t.xs.zt[out_qubit][inp_qubit], bxz);
                ASSERT_EQ(t.zs.xt[out_qubit][inp_qubit], bzx);
                ASSERT_EQ(t.zs.zt[out_qubit][inp_qubit], bzz);
            }
        }
    }
}

TEST(tableau, inverse) {
    Tableau t1(1);
    ASSERT_EQ(t1, t1.inverse());
    t1.prepend_X(0);
    ASSERT_EQ(t1, GATE_DATA.at("X").tableau());
    auto t2 = t1.inverse();
    ASSERT_EQ(t1, GATE_DATA.at("X").tableau());
    ASSERT_EQ(t2, GATE_DATA.at("X").tableau());

    for (size_t k = 5; k < 20; k += 7) {
        t1 = Tableau::random(k, SHARED_TEST_RNG());
        t2 = t1.inverse();
        ASSERT_TRUE(t2.satisfies_invariants());
        auto p = PauliString::random(k, SHARED_TEST_RNG());
        auto p2 = t1(t2(p));
        auto x1 = p.xs.str();
        auto x2 = p2.xs.str();
        auto z1 = p.zs.str();
        auto z2 = p2.zs.str();
        ASSERT_EQ(p, p2);
        ASSERT_EQ(p, t2(t1(p)));
    }
}

TEST(tableau, prepend_pauli_product) {
    Tableau t = Tableau::random(6, SHARED_TEST_RNG());
    Tableau ref = t;
    t.prepend_pauli_product(PauliString::from_str("_XYZ__"));
    ref.prepend_X(1);
    ref.prepend_Y(2);
    ref.prepend_Z(3);
    ASSERT_EQ(t, ref);
    t.prepend_pauli_product(PauliString::from_str("Y_ZX__"));
    ref.prepend_X(3);
    ref.prepend_Y(0);
    ref.prepend_Z(2);
    ASSERT_EQ(t, ref);
}

TEST(tableau, then) {
    Tableau cnot = GATE_DATA.at("CNOT").tableau();
    Tableau swap = GATE_DATA.at("SWAP").tableau();
    Tableau hh(2);
    hh.inplace_scatter_append(GATE_DATA.at("H").tableau(), {0});
    hh.inplace_scatter_append(GATE_DATA.at("H").tableau(), {1});

    ASSERT_EQ(cnot.then(cnot), Tableau(2));
    ASSERT_EQ(hh.then(cnot).then(hh), swap.then(cnot).then(swap));
    ASSERT_EQ(cnot.then(cnot), Tableau(2));

    Tableau t(2);
    t.inplace_scatter_append(GATE_DATA.at("SQRT_X_DAG").tableau(), {1});
    t = t.then(GATE_DATA.at("CY").tableau());
    t.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau(), {1});
    ASSERT_EQ(t, GATE_DATA.at("CZ").tableau());
}

TEST(tableau, raised_to) {
    Tableau cnot = GATE_DATA.at("CNOT").tableau();
    ASSERT_EQ(cnot.raised_to(-97268202), Tableau(2));
    ASSERT_EQ(cnot.raised_to(-97268201), cnot);
    ASSERT_EQ(cnot.raised_to(-3), cnot);
    ASSERT_EQ(cnot.raised_to(-2), Tableau(2));
    ASSERT_EQ(cnot.raised_to(-1), cnot);
    ASSERT_EQ(cnot.raised_to(0), Tableau(2));
    ASSERT_EQ(cnot.raised_to(1), cnot);
    ASSERT_EQ(cnot.raised_to(2), Tableau(2));
    ASSERT_EQ(cnot.raised_to(3), cnot);
    ASSERT_EQ(cnot.raised_to(4), Tableau(2));
    ASSERT_EQ(cnot.raised_to(97268201), cnot);
    ASSERT_EQ(cnot.raised_to(97268202), Tableau(2));

    Tableau s = GATE_DATA.at("S").tableau();
    Tableau z = GATE_DATA.at("Z").tableau();
    Tableau s_dag = GATE_DATA.at("S_DAG").tableau();
    ASSERT_EQ(s.raised_to(4 * -437829 + 0), Tableau(1));
    ASSERT_EQ(s.raised_to(4 * -437829 + 1), s);
    ASSERT_EQ(s.raised_to(4 * -437829 + 2), z);
    ASSERT_EQ(s.raised_to(4 * -437829 + 3), s_dag);
    ASSERT_EQ(s.raised_to(-5), s_dag);
    ASSERT_EQ(s.raised_to(-4), Tableau(1));
    ASSERT_EQ(s.raised_to(-3), s);
    ASSERT_EQ(s.raised_to(-2), z);
    ASSERT_EQ(s.raised_to(-1), s_dag);
    ASSERT_EQ(s.raised_to(0), Tableau(1));
    ASSERT_EQ(s.raised_to(1), s);
    ASSERT_EQ(s.raised_to(2), z);
    ASSERT_EQ(s.raised_to(3), s_dag);
    ASSERT_EQ(s.raised_to(4), Tableau(1));
    ASSERT_EQ(s.raised_to(5), s);
    ASSERT_EQ(s.raised_to(6), z);
    ASSERT_EQ(s.raised_to(7), s_dag);
    ASSERT_EQ(s.raised_to(4 * 437829 + 0), Tableau(1));
    ASSERT_EQ(s.raised_to(4 * 437829 + 1), s);
    ASSERT_EQ(s.raised_to(4 * 437829 + 2), z);
    ASSERT_EQ(s.raised_to(4 * 437829 + 3), s_dag);

    Tableau p15(3);
    p15.inplace_scatter_append(GATE_DATA.at("SQRT_X").tableau(), {0});
    p15.inplace_scatter_append(s, {2});
    p15.inplace_scatter_append(cnot, {0, 1});
    p15.inplace_scatter_append(cnot, {1, 2});
    for (size_t k = 1; k < 15; k++) {
        ASSERT_NE(p15.raised_to(k), Tableau(3));
    }
    ASSERT_EQ(p15.raised_to(15), Tableau(3));
    ASSERT_EQ(p15.raised_to(15 * 47321 + 4), p15.raised_to(4));
    ASSERT_EQ(p15.raised_to(15 * 47321 + 1), p15);
    ASSERT_EQ(p15.raised_to(15 * -47321 + 1), p15);
}

TEST(tableau, transposed_xz_input) {
    Tableau t = Tableau::random(4, SHARED_TEST_RNG());
    PauliString x0(0);
    PauliString x1(0);
    {
        TableauTransposedRaii tmp(t);
        x0 = tmp.unsigned_x_input(0);
        x1 = tmp.unsigned_x_input(1);
    }
    auto tx0 = t(x0);
    auto tx1 = t(x1);
    tx0.sign = false;
    tx1.sign = false;
    ASSERT_EQ(tx0, PauliString::from_str("X___"));
    ASSERT_EQ(tx1, PauliString::from_str("_X__"));
}

TEST(tableau, direct_sum) {
    Tableau t1 = Tableau::random(260, SHARED_TEST_RNG());
    Tableau t2 = Tableau::random(270, SHARED_TEST_RNG());
    Tableau t3 = t1;
    t3 += t2;
    ASSERT_EQ(t3, t1 + t2);

    PauliString p1 = t1.xs[5];
    p1.ensure_num_qubits(260 + 270);
    ASSERT_EQ(t3.xs[5], p1);

    std::string p2 = t2.xs[6].str();
    std::string p3 = t3.xs[266].str();
    ASSERT_EQ(p2[0], p3[0]);
    p2 = p2.substr(1);
    p3 = p3.substr(1);
    for (size_t k = 0; k < 260; k++) {
        ASSERT_EQ(p3[k], '_');
    }
    for (size_t k = 0; k < 270; k++) {
        ASSERT_EQ(p3[260 + k], p2[k]);
    }
}

TEST(tableau, pauli_acces_methods) {
    auto t = Tableau::random(3, SHARED_TEST_RNG());
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

    t = Tableau(3);
    t.xs[0] = PauliString::from_str("+XXX");
    t.xs[1] = PauliString::from_str("-XZY");
    t.xs[2] = PauliString::from_str("+Z_Z");
    t.zs[0] = PauliString::from_str("-_XZ");
    t.zs[1] = PauliString::from_str("-_X_");
    t.zs[2] = PauliString::from_str("-X__");

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
}

TEST(tableau, inverse_pauli_string_acces_methods) {
    auto t = Tableau::random(5, SHARED_TEST_RNG());
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
}

TEST(tableau, unitary_little_endian) {
    Tableau t(1);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{1, 0, 0, 1}));
    t.prepend_SQRT_Y(0);
    auto s = sqrtf(0.5);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{s, -s, s, s}));
    t.prepend_SQRT_Y(0);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{0, 1, -1, 0}));
    t.prepend_SQRT_Y(0);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{s, s, -s, s}));

    t = Tableau(2);
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
}

TEST(tableau, unitary_big_endian) {
    Tableau t(1);
    ASSERT_EQ(t.to_flat_unitary_matrix(true), (std::vector<std::complex<float>>{1, 0, 0, 1}));
    t.prepend_SQRT_Y(0);
    auto s = sqrtf(0.5);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{s, -s, s, s}));
    t.prepend_SQRT_Y(0);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{0, 1, -1, 0}));
    t.prepend_SQRT_Y(0);
    ASSERT_EQ(t.to_flat_unitary_matrix(false), (std::vector<std::complex<float>>{s, s, -s, s}));

    t = Tableau(2);
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
}

TEST(tableau, unitary_vs_gate_data) {
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            std::vector<std::complex<float>> flat_expected;
            for (const auto &row : gate.unitary()) {
                flat_expected.insert(flat_expected.end(), row.begin(), row.end());
            }
            VectorSimulator v(0);
            v.state = std::move(flat_expected);
            v.canonicalize_assuming_stabilizer_state((gate.flags & stim::GATE_TARGETS_PAIRS) ? 4 : 2);
            EXPECT_EQ(gate.tableau().to_flat_unitary_matrix(true), v.state) << gate.name;
        }
    }
}
