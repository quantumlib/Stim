#include "gtest/gtest.h"
#include "tableau.h"
#include "vector_sim.h"
#include <random>

static float complex_distance(std::complex<float> a, std::complex<float> b) {
    auto d = a - b;
    return sqrtf(d.real()*d.real() + d.imag()*d.imag());
}

TEST(tableau, identity) {
    auto t = Tableau::identity(4);
    ASSERT_EQ(t.str(), ""
                       "Tableau {\n"
                       "  qubit 0_x: +X___\n"
                       "  qubit 0_z: +Z___\n"
                       "  qubit 1_x: +_X__\n"
                       "  qubit 1_z: +_Z__\n"
                       "  qubit 2_x: +__X_\n"
                       "  qubit 2_z: +__Z_\n"
                       "  qubit 3_x: +___X\n"
                       "  qubit 3_z: +___Z\n"
                       "}");
}

TEST(tableau, gate1) {
    auto gate1 = Tableau::gate1("+X", "+Z");
    ASSERT_EQ(gate1.x_obs_ptr(0).str(), "+X");
    ASSERT_EQ(gate1.eval_y_obs(0).str(), "+Y");
    ASSERT_EQ(gate1.z_obs_ptr(0).str(), "+Z");
}

bool tableau_agrees_with_unitary(const Tableau &tableau,
                                 const std::vector<std::vector<std::complex<float>>> &unitary) {
    auto n = tableau.num_qubits;
    assert(unitary.size() == 1 << n);

    std::vector<PauliStringVal> basis;
    for (size_t x = 0; x < 2; x++) {
        for (size_t k = 0; k < n; k++) {
            basis.emplace_back(n);
            if (x) {
                basis.back().ptr().toggle_x_bit(k);
            } else {
                basis.back().ptr().toggle_z_bit(k);
            }
        }
    }

    for (const auto &input_side_obs : basis) {
        VectorSim sim(n*2);
        // Create EPR pairs to test all possible inputs via state channel duality.
        for (size_t q = 0; q < n; q++) {
            sim.apply("H", q);
            sim.apply("CNOT", q, q + n);
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
    auto s = t.x_obs_ptr(0).str();
    size_t n = 0;
    for (size_t k = 1; k < s.size(); k++) {
        if (s[k] != '_') {
            n += 1;
        }
    }
    ASSERT_EQ(n, 1) << s;
}

TEST(tableau, str) {
    ASSERT_EQ(GATE_TABLEAUS.at("H").str(),
              "Tableau {\n"
              "  qubit 0_x: +Z\n"
              "  qubit 0_z: +X\n"
              "}");
}

TEST(tableau, gate_tableau_data_vs_unitary_data) {
    for (const auto &kv : GATE_TABLEAUS) {
        const auto &name = kv.first;
        const auto &tab = kv.second;
        const auto &u = GATE_UNITARIES.at(name);
        ASSERT_TRUE(tableau_agrees_with_unitary(tab, u)) << name;
    }
}

TEST(tableau, inverse_data) {
    for (const auto &kv : GATE_TABLEAUS) {
        const auto &name = kv.first;
        Tableau tab = kv.second;
        const auto &inverse_name = GATE_INVERSE_NAMES.at(name);
        const auto &tab2 = GATE_TABLEAUS.at(inverse_name);
        std::vector<size_t> targets {0};
        while (targets.size() < tab.num_qubits) {
            targets.push_back(targets.size());
        }
        tab.inplace_scatter_append(tab2, targets);
        ASSERT_EQ(tab, Tableau::identity(tab.num_qubits)) << name << " -> " << inverse_name;
    }
}

TEST(tableau, eval) {
    const auto &cnot = GATE_TABLEAUS.at("CNOT");
    ASSERT_EQ(cnot(PauliStringVal::from_str("-XX")), PauliStringVal::from_str("-XI"));
    ASSERT_EQ(cnot(PauliStringVal::from_str("+XX")), PauliStringVal::from_str("+XI"));
    ASSERT_EQ(cnot(PauliStringVal::from_str("+ZZ")), PauliStringVal::from_str("+IZ"));
    ASSERT_EQ(cnot(PauliStringVal::from_str("+IY")), PauliStringVal::from_str("+ZY"));
    ASSERT_EQ(cnot(PauliStringVal::from_str("+YI")), PauliStringVal::from_str("+YX"));
    ASSERT_EQ(cnot(PauliStringVal::from_str("+YY")), PauliStringVal::from_str("-XZ"));

    const auto &x2 = GATE_TABLEAUS.at("SQRT_X");
    ASSERT_EQ(x2(PauliStringVal::from_str("+X")), PauliStringVal::from_str("+X"));
    ASSERT_EQ(x2(PauliStringVal::from_str("+Y")), PauliStringVal::from_str("+Z"));
    ASSERT_EQ(x2(PauliStringVal::from_str("+Z")), PauliStringVal::from_str("-Y"));

    const auto &s = GATE_TABLEAUS.at("S");
    ASSERT_EQ(s(PauliStringVal::from_str("+X")), PauliStringVal::from_str("+Y"));
    ASSERT_EQ(s(PauliStringVal::from_str("+Y")), PauliStringVal::from_str("-X"));
    ASSERT_EQ(s(PauliStringVal::from_str("+Z")), PauliStringVal::from_str("+Z"));
}

TEST(tableau, apply_within) {
    const auto &cnot = GATE_TABLEAUS.at("CNOT");

    auto p1 = PauliStringVal::from_str("-XX");
    auto p1_ptr = p1.ptr();
    cnot.apply_within(p1_ptr, {0, 1});
    ASSERT_EQ(p1, PauliStringVal::from_str("-XI"));

    auto p2 = PauliStringVal::from_str("+XX");
    auto p2_ptr = p2.ptr();
    cnot.apply_within(p2_ptr, {0, 1});
    ASSERT_EQ(p2, PauliStringVal::from_str("+XI"));
}

TEST(tableau, equality) {
    ASSERT_TRUE(GATE_TABLEAUS.at("S") == GATE_TABLEAUS.at("SQRT_Z"));
    ASSERT_FALSE(GATE_TABLEAUS.at("S") != GATE_TABLEAUS.at("SQRT_Z"));
    ASSERT_FALSE(GATE_TABLEAUS.at("S") == GATE_TABLEAUS.at("CNOT"));
    ASSERT_TRUE(GATE_TABLEAUS.at("S") != GATE_TABLEAUS.at("CNOT"));
    ASSERT_FALSE(GATE_TABLEAUS.at("S") == GATE_TABLEAUS.at("SQRT_X"));
    ASSERT_TRUE(GATE_TABLEAUS.at("S") != GATE_TABLEAUS.at("SQRT_X"));
}

TEST(tableau, inplace_scatter_append) {
    auto t1 = Tableau::identity(1);
    t1.inplace_scatter_append(GATE_TABLEAUS.at("S"), {0});
    ASSERT_EQ(t1, GATE_TABLEAUS.at("S"));
    t1.inplace_scatter_append(GATE_TABLEAUS.at("S"), {0});
    ASSERT_EQ(t1, GATE_TABLEAUS.at("Z"));
    t1.inplace_scatter_append(GATE_TABLEAUS.at("S"), {0});
    ASSERT_EQ(t1, GATE_TABLEAUS.at("S_DAG"));

    // Test swap decomposition into exp(i pi/2 (XX + YY + ZZ)).
    auto t2 = Tableau::identity(2);
    // ZZ^0.5
    t2.inplace_scatter_append(GATE_TABLEAUS.at("SQRT_Z"), {0});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("SQRT_Z"), {1});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("CZ"), {0, 1});
    // YY^0.5
    t2.inplace_scatter_append(GATE_TABLEAUS.at("SQRT_Y"), {0});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("SQRT_Y"), {1});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("H_YZ"), {0});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("CY"), {0, 1});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("H_YZ"), {0});
    // XX^0.5
    t2.inplace_scatter_append(GATE_TABLEAUS.at("SQRT_X"), {0});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("SQRT_X"), {1});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("H"), {0});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("CX"), {0, 1});
    t2.inplace_scatter_append(GATE_TABLEAUS.at("H"), {0});
    ASSERT_EQ(t2, GATE_TABLEAUS.at("SWAP"));

    // Test order dependence.
    auto t3 = Tableau::identity(2);
    t3.inplace_scatter_append(GATE_TABLEAUS.at("H"), {0});
    t3.inplace_scatter_append(GATE_TABLEAUS.at("SQRT_X"), {1});
    t3.inplace_scatter_append(GATE_TABLEAUS.at("CNOT"), {0, 1});
    ASSERT_EQ(t3(PauliStringVal::from_str("XI")), PauliStringVal::from_str("ZI"));
    ASSERT_EQ(t3(PauliStringVal::from_str("ZI")), PauliStringVal::from_str("XX"));
    ASSERT_EQ(t3(PauliStringVal::from_str("IX")), PauliStringVal::from_str("IX"));
    ASSERT_EQ(t3(PauliStringVal::from_str("IZ")), PauliStringVal::from_str("-ZY"));
}

TEST(tableau, inplace_scatter_prepend) {
    auto t1 = Tableau::identity(1);
    t1.inplace_scatter_prepend(GATE_TABLEAUS.at("S"), {0});
    ASSERT_EQ(t1, GATE_TABLEAUS.at("S"));
    t1.inplace_scatter_prepend(GATE_TABLEAUS.at("S"), {0});
    ASSERT_EQ(t1, GATE_TABLEAUS.at("Z"));
    t1.inplace_scatter_prepend(GATE_TABLEAUS.at("S"), {0});
    ASSERT_EQ(t1, GATE_TABLEAUS.at("S_DAG"));

    // Test swap decomposition into exp(i pi/2 (XX + YY + ZZ)).
    auto t2 = Tableau::identity(2);
    // ZZ^0.5
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Z"), {0});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Z"), {1});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("CZ"), {0, 1});
    // YY^0.5
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Y"), {0});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Y"), {1});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("H_YZ"), {0});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("CY"), {0, 1});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("H_YZ"), {0});
    // XX^0.5
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_X"), {0});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_X"), {1});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("H"), {0});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("CX"), {0, 1});
    t2.inplace_scatter_prepend(GATE_TABLEAUS.at("H"), {0});
    ASSERT_EQ(t2, GATE_TABLEAUS.at("SWAP"));

    // Test order dependence.
    auto t3 = Tableau::identity(2);
    t3.inplace_scatter_prepend(GATE_TABLEAUS.at("H"), {0});
    t3.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_X"), {1});
    t3.inplace_scatter_prepend(GATE_TABLEAUS.at("CNOT"), {0, 1});
    ASSERT_EQ(t3(PauliStringVal::from_str("XI")), PauliStringVal::from_str("ZX"));
    ASSERT_EQ(t3(PauliStringVal::from_str("ZI")), PauliStringVal::from_str("XI"));
    ASSERT_EQ(t3(PauliStringVal::from_str("IX")), PauliStringVal::from_str("IX"));
    ASSERT_EQ(t3(PauliStringVal::from_str("IZ")), PauliStringVal::from_str("-XY"));
}

TEST(tableau, eval_y) {
    ASSERT_EQ(GATE_TABLEAUS.at("H").z_obs_ptr(0), PauliStringVal::from_str("+X"));
    ASSERT_EQ(GATE_TABLEAUS.at("S").z_obs_ptr(0), PauliStringVal::from_str("+Z"));
    ASSERT_EQ(GATE_TABLEAUS.at("H_YZ").z_obs_ptr(0), PauliStringVal::from_str("+Y"));
    ASSERT_EQ(GATE_TABLEAUS.at("SQRT_Y").z_obs_ptr(0), PauliStringVal::from_str("X"));
    ASSERT_EQ(GATE_TABLEAUS.at("SQRT_Y_DAG").z_obs_ptr(0), PauliStringVal::from_str("-X"));
    ASSERT_EQ(GATE_TABLEAUS.at("CNOT").z_obs_ptr(1), PauliStringVal::from_str("ZZ"));

    ASSERT_EQ(GATE_TABLEAUS.at("H").eval_y_obs(0), PauliStringVal::from_str("-Y"));
    ASSERT_EQ(GATE_TABLEAUS.at("S").eval_y_obs(0), PauliStringVal::from_str("-X"));
    ASSERT_EQ(GATE_TABLEAUS.at("H_YZ").eval_y_obs(0), PauliStringVal::from_str("+Z"));
    ASSERT_EQ(GATE_TABLEAUS.at("SQRT_Y").eval_y_obs(0), PauliStringVal::from_str("+Y"));
    ASSERT_EQ(GATE_TABLEAUS.at("SQRT_Y_DAG").eval_y_obs(0), PauliStringVal::from_str("+Y"));
    ASSERT_EQ(GATE_TABLEAUS.at("SQRT_X").eval_y_obs(0), PauliStringVal::from_str("+Z"));
    ASSERT_EQ(GATE_TABLEAUS.at("SQRT_X_DAG").eval_y_obs(0), PauliStringVal::from_str("-Z"));
    ASSERT_EQ(GATE_TABLEAUS.at("CNOT").eval_y_obs(1), PauliStringVal::from_str("ZY"));
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
        test_tableau_dual.inplace_scatter_append(GATE_TABLEAUS.at("H"), {k});
        test_tableau_dual.inplace_scatter_append(GATE_TABLEAUS.at("CNOT"), {k, k + n});
        targets1.push_back(k);
        targets2.push_back(k + n);
        targets3.push_back(k + (k % 2 == 0 ? 0 : n));
    }

    std::vector<Tableau> tableaus{
        test_tableau_dual,
        Tableau::random(n + 10),
        Tableau::random(n + 30),
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

TEST(tableau, check_invariants) {
    ASSERT_TRUE(Tableau::gate1("X", "Z").satisfies_invariants());
    ASSERT_TRUE(Tableau::gate2("XI", "ZI", "IX", "IZ").satisfies_invariants());
    ASSERT_FALSE(Tableau::gate1("X", "X").satisfies_invariants());
    ASSERT_FALSE(Tableau::gate2("XI", "ZI", "XI", "ZI").satisfies_invariants());
    ASSERT_FALSE(Tableau::gate2("XI", "II", "IX", "IZ").satisfies_invariants());
}

TEST(tableau, random) {
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau::random(1);
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau::random(2);
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau::random(3);
        ASSERT_TRUE(t.satisfies_invariants()) << t;
    }
    for (size_t k = 0; k < 20; k++) {
        auto t = Tableau::random(30);
        ASSERT_TRUE(t.satisfies_invariants());
    }
}

TEST(tableau, specialized_operations) {
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("X"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("X"), targets); }
    ));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("S"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Z"), targets); }
    ));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("CNOT"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("CX"), targets); }
    ));
    EXPECT_FALSE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("H"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Y"), targets); }
    ));
    EXPECT_FALSE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("CNOT"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("CZ"), targets); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("H"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_H(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("H_YZ"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_H_YZ(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("H_XY"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_H_XY(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("X"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_X(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("Y"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_Y(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("Z"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_Z(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SWAP"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_SWAP(targets[0], targets[1]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("CX"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_CX(targets[0], targets[1]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("CY"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_CY(targets[0], targets[1]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("CZ"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_CZ(targets[0], targets[1]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_X"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_SQRT_X(targets[0]); }
    ));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_X_DAG"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_SQRT_X_DAG(targets[0]); }
    ));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Y"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_SQRT_Y(targets[0]); }
    ));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Y_DAG"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_SQRT_Y_DAG(targets[0]); }
    ));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Z"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_SQRT_Z(targets[0]); }
    ));
    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_prepend(GATE_TABLEAUS.at("SQRT_Z_DAG"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ t.prepend_SQRT_Z_DAG(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_append(GATE_TABLEAUS.at("X"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ TempBlockTransposedTableauRaii(t).append_X(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_append(GATE_TABLEAUS.at("H"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ TempBlockTransposedTableauRaii(t).append_H(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_append(GATE_TABLEAUS.at("H_XY"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ TempBlockTransposedTableauRaii(t).append_H_XY(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        1,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_append(GATE_TABLEAUS.at("H_YZ"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ TempBlockTransposedTableauRaii(t).append_H_YZ(targets[0]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_append(GATE_TABLEAUS.at("CX"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ TempBlockTransposedTableauRaii(t).append_CX(targets[0], targets[1]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_append(GATE_TABLEAUS.at("CY"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ TempBlockTransposedTableauRaii(t).append_CY(targets[0], targets[1]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_append(GATE_TABLEAUS.at("CZ"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ TempBlockTransposedTableauRaii(t).append_CZ(targets[0], targets[1]); }
    ));

    EXPECT_TRUE(are_tableau_mutations_equivalent(
        2,
        [](Tableau &t, const std::vector<size_t> &targets){ t.inplace_scatter_append(GATE_TABLEAUS.at("SWAP"), targets); },
        [](Tableau &t, const std::vector<size_t> &targets){ TempBlockTransposedTableauRaii(t).append_SWAP(targets[0], targets[1]); }
    ));
}

TEST(tableau, bit_address) {
    size_t n = 1500;
    size_t quad_0 = 0;
    size_t quad_1 = 1;
    bool not_transposed = false;
    bool yes_transposed = true;

//    ASSERT_EQ(bit_address(0, 0, n, quad_0, not_transposed), 0);
//    ASSERT_EQ(bit_address(1, 0, n, quad_0, not_transposed), 1 << 8);
//    ASSERT_EQ(bit_address(0, 1, n, quad_0, not_transposed), 1);
//    ASSERT_EQ(bit_address(256, 0, n, quad_0, not_transposed), 6 << 16);
//    ASSERT_EQ(bit_address(0, 256, n, quad_0, not_transposed), 1 << 16);
//
//    ASSERT_EQ(bit_address(0, 0, n, quad_0, yes_transposed), 0);
//    ASSERT_EQ(bit_address(1, 0, n, quad_0, yes_transposed), 1);
//    ASSERT_EQ(bit_address(0, 1, n, quad_0, yes_transposed), 1 << 8);
//    ASSERT_EQ(bit_address(256, 0, n, quad_0, yes_transposed), 6 << 16);
//    ASSERT_EQ(bit_address(0, 256, n, quad_0, yes_transposed), 1 << 16);
//
//    ASSERT_EQ(bit_address(0, 0, n, quad_1, not_transposed), ceil256(n)*ceil256(n));
//    ASSERT_EQ(bit_address(0, 0, n, quad_1, yes_transposed), ceil256(n)*ceil256(n));
}

TEST(tableau, expand) {
    auto t = Tableau::random(4);
    auto t2 = t;
    for (size_t n = 8; n < 500; n += 255) {
        t2.expand(n);
        assert(t2.num_qubits == n);
        for (size_t k = 0; k < n; k++) {
            if (k < 4) {
                ASSERT_EQ(t.x_obs_ptr(k).bit_ptr_sign.get(), t2.x_obs_ptr(k).bit_ptr_sign.get());
                ASSERT_EQ(t.z_obs_ptr(k).bit_ptr_sign.get(), t2.z_obs_ptr(k).bit_ptr_sign.get());
            } else {
                ASSERT_EQ(t2.x_obs_ptr(k).bit_ptr_sign.get(), false);
                ASSERT_EQ(t2.z_obs_ptr(k).bit_ptr_sign.get(), false);
            }
            for (size_t k2 = 0; k2 < n; k2++) {
                if (k < 4 && k2 < 4) {
                    ASSERT_EQ(t.x_obs_ptr(k).get_x_bit(k2), t2.x_obs_ptr(k).get_x_bit(k2));
                    ASSERT_EQ(t.x_obs_ptr(k).get_z_bit(k2), t2.x_obs_ptr(k).get_z_bit(k2));
                    ASSERT_EQ(t.z_obs_ptr(k).get_x_bit(k2), t2.z_obs_ptr(k).get_x_bit(k2));
                    ASSERT_EQ(t.z_obs_ptr(k).get_z_bit(k2), t2.z_obs_ptr(k).get_z_bit(k2));
                } else if (k == k2) {
                    ASSERT_EQ(t2.x_obs_ptr(k).get_x_bit(k2), true);
                    ASSERT_EQ(t2.x_obs_ptr(k).get_z_bit(k2), false);
                    ASSERT_EQ(t2.z_obs_ptr(k).get_x_bit(k2), false);
                    ASSERT_EQ(t2.z_obs_ptr(k).get_z_bit(k2), true);
                } else {
                    ASSERT_EQ(t2.x_obs_ptr(k).get_x_bit(k2), false);
                    ASSERT_EQ(t2.x_obs_ptr(k).get_z_bit(k2), false);
                    ASSERT_EQ(t2.z_obs_ptr(k).get_x_bit(k2), false);
                    ASSERT_EQ(t2.z_obs_ptr(k).get_z_bit(k2), false);
                }
            }
        }
    }
}

TEST(tableau, transposed_access) {
    size_t X2X_QUAD = 0;
    size_t Z2X_QUAD = 1;
    size_t X2Z_QUAD = 2;
    size_t Z2Z_QUAD = 3;

    size_t n = 1000;
    Tableau t(n);
    t.data_x2x_z2x_x2z_z2z = aligned_bits256::random(t.data_x2x_z2x_x2z_z2z.num_bits);
    for (size_t inp_qubit = 0; inp_qubit < 1000; inp_qubit += 99) {
        for (size_t out_qubit = 0; out_qubit < 1000; out_qubit += 99) {
            auto bxx = t.data_x2x_z2x_x2z_z2z.get_bit(bit_address(inp_qubit, out_qubit, n, X2X_QUAD, false));
            auto bxz = t.data_x2x_z2x_x2z_z2z.get_bit(bit_address(inp_qubit, out_qubit, n, X2Z_QUAD, false));
            auto bzx = t.data_x2x_z2x_x2z_z2z.get_bit(bit_address(inp_qubit, out_qubit, n, Z2X_QUAD, false));
            auto bzz = t.data_x2x_z2x_x2z_z2z.get_bit(bit_address(inp_qubit, out_qubit, n, Z2Z_QUAD, false));

            ASSERT_EQ(t.x_obs_ptr(inp_qubit).get_x_bit(out_qubit), bxx) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.x_obs_ptr(inp_qubit).get_z_bit(out_qubit), bxz) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.z_obs_ptr(inp_qubit).get_x_bit(out_qubit), bzx) << inp_qubit << ", " << out_qubit;
            ASSERT_EQ(t.z_obs_ptr(inp_qubit).get_z_bit(out_qubit), bzz) << inp_qubit << ", " << out_qubit;

            {
                TempBlockTransposedTableauRaii trans(t);
                ASSERT_EQ(t.data_x2x_z2x_x2z_z2z.get_bit(bit_address(inp_qubit, out_qubit, n, X2X_QUAD, true)), bxx);
                ASSERT_EQ(t.data_x2x_z2x_x2z_z2z.get_bit(bit_address(inp_qubit, out_qubit, n, X2Z_QUAD, true)), bxz);
                ASSERT_EQ(t.data_x2x_z2x_x2z_z2z.get_bit(bit_address(inp_qubit, out_qubit, n, Z2X_QUAD, true)), bzx);
                ASSERT_EQ(t.data_x2x_z2x_x2z_z2z.get_bit(bit_address(inp_qubit, out_qubit, n, Z2Z_QUAD, true)), bzz);

                ASSERT_EQ(trans.z_obs_x_bit(inp_qubit, out_qubit), bzx);
                ASSERT_EQ(trans.z_obs_z_bit(inp_qubit, out_qubit), bzz);
            }
        }
    }
}
