#include "gtest/gtest.h"
#include "tableau.h"
#include "vector_sim.h"

static float complex_distance(std::complex<float> a, std::complex<float> b) {
    auto d = a - b;
    return sqrtf(d.real()*d.real() + d.imag()*d.imag());
}

TEST(tableau, identity) {
    auto t = Tableau::identity(4);
    ASSERT_EQ(t.str(), ""
                       "Tableau {\n"
                       "  qubit 0_x: +X___\n"
                       "  qubit 0_y: +Y___\n"
                       "  qubit 1_x: +_X__\n"
                       "  qubit 1_y: +_Y__\n"
                       "  qubit 2_x: +__X_\n"
                       "  qubit 2_y: +__Y_\n"
                       "  qubit 3_x: +___X\n"
                       "  qubit 3_y: +___Y\n"
                       "}");
}

bool tableau_agrees_with_unitary(const Tableau &tab, const std::vector<std::vector<std::complex<float>>> &unitary) {
    auto n = tab.qubits.size();
    assert(unitary.size() == 1 << n);
    for (size_t xb = 0; xb < 2; xb++) {
        for (size_t k = 0; k < n; k++) {
            PauliString pre = PauliString::identity(n);
            const PauliString &post = xb ? tab.qubits[k].x : tab.qubits[k].y;
            if (xb) {
                pre.toggle_x_bit(k);
            } else {
                pre.toggle_y_bit(k);
            }
            VectorSim sim(n*2);
            for (size_t q = 0; q < n; q++) {
                sim.apply("H", q);
                sim.apply("CNOT", q, q + n);
            }
            sim.apply(pre, n);
            std::vector<size_t> qs;
            for (size_t q = 0; q < n; q++) {
                qs.push_back(q + n);
            }
            sim.apply(unitary, {qs});
            sim.apply(post, n);

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
    }
    return true;
}

TEST(tableau, gate_data) {
    ASSERT_EQ(GATE_TABLEAUS.at("H").str(),
              "Tableau {\n"
              "  qubit 0_x: +Z\n"
              "  qubit 0_y: -Y\n"
              "}");

    for (const auto &kv : GATE_TABLEAUS) {
        const auto &name = kv.first;
        const auto &tab = kv.second;
        const auto &u = GATE_UNITARIES.at(name);
        ASSERT_TRUE(tableau_agrees_with_unitary(tab, u)) << name;
    }
}

TEST(tableau, eval) {
    const auto &cnot = GATE_TABLEAUS.at("CNOT");
    ASSERT_EQ(cnot(PauliString::from_str("-XX")), PauliString::from_str("-XI"));
    ASSERT_EQ(cnot(PauliString::from_str("+XX")), PauliString::from_str("+XI"));
    ASSERT_EQ(cnot(PauliString::from_str("+ZZ")), PauliString::from_str("+IZ"));
    ASSERT_EQ(cnot(PauliString::from_str("+IY")), PauliString::from_str("+ZY"));
    ASSERT_EQ(cnot(PauliString::from_str("+YI")), PauliString::from_str("+YX"));
    ASSERT_EQ(cnot(PauliString::from_str("+YY")), PauliString::from_str("-XZ"));

    const auto &x2 = GATE_TABLEAUS.at("SQRT_X");
    ASSERT_EQ(x2(PauliString::from_str("+X")), PauliString::from_str("+X"));
    ASSERT_EQ(x2(PauliString::from_str("+Y")), PauliString::from_str("+Z"));
    ASSERT_EQ(x2(PauliString::from_str("+Z")), PauliString::from_str("-Y"));

    const auto &s = GATE_TABLEAUS.at("S");
    ASSERT_EQ(s(PauliString::from_str("+X")), PauliString::from_str("+Y"));
    ASSERT_EQ(s(PauliString::from_str("+Y")), PauliString::from_str("-X"));
    ASSERT_EQ(s(PauliString::from_str("+Z")), PauliString::from_str("+Z"));
}

TEST(tableau, apply_within) {
    const auto &cnot = GATE_TABLEAUS.at("CNOT");

    auto p1 = PauliString::from_str("-XX");
    cnot.apply_within(p1, {0, 1});
    ASSERT_EQ(p1, PauliString::from_str("-XI"));

    auto p2 = PauliString::from_str("+XX");
    cnot.apply_within(p2, {0, 1});
    ASSERT_EQ(p2, PauliString::from_str("+XI"));
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
    ASSERT_EQ(t3(PauliString::from_str("XI")), PauliString::from_str("ZI"));
    ASSERT_EQ(t3(PauliString::from_str("ZI")), PauliString::from_str("XX"));
    ASSERT_EQ(t3(PauliString::from_str("IX")), PauliString::from_str("IX"));
    ASSERT_EQ(t3(PauliString::from_str("IZ")), PauliString::from_str("-ZY"));
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
    ASSERT_EQ(t3(PauliString::from_str("XI")), PauliString::from_str("ZX"));
    ASSERT_EQ(t3(PauliString::from_str("ZI")), PauliString::from_str("XI"));
    ASSERT_EQ(t3(PauliString::from_str("IX")), PauliString::from_str("IX"));
    ASSERT_EQ(t3(PauliString::from_str("IZ")), PauliString::from_str("-XY"));
}
