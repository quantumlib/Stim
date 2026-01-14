#include "stim/stabilizers/clifford_string.h"

#include "stim/util_bot/test_util.test.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/stabilizers/tableau.h"

using namespace stim;

std::vector<Gate> single_qubit_clifford_rotations() {
    std::vector<Gate> result;
    for (size_t g = 0; g < NUM_DEFINED_GATES; g++) {
        const Gate &gate = GATE_DATA[(GateType)g];
        if ((gate.flags & GateFlags::GATE_IS_SINGLE_QUBIT_GATE) && (gate.flags & GateFlags::GATE_IS_UNITARY)) {
            result.push_back(gate);
        }
    }
    assert(result.size() == 24);
    return result;
}

TEST_EACH_WORD_SIZE_W(clifford_string, set_gate_at_vs_str_vs_gate_at, {
    CliffordString<W> p = CliffordString<W>(24);
    int x = 0;

    p.set_gate_at(x++, GateType::I);
    p.set_gate_at(x++, GateType::X);
    p.set_gate_at(x++, GateType::Y);
    p.set_gate_at(x++, GateType::Z);

    p.set_gate_at(x++, GateType::H);
    p.set_gate_at(x++, GateType::SQRT_Y_DAG);
    p.set_gate_at(x++, GateType::H_NXZ);
    p.set_gate_at(x++, GateType::SQRT_Y);

    p.set_gate_at(x++, GateType::S);
    p.set_gate_at(x++, GateType::H_XY);
    p.set_gate_at(x++, GateType::H_NXY);
    p.set_gate_at(x++, GateType::S_DAG);

    p.set_gate_at(x++, GateType::SQRT_X_DAG);
    p.set_gate_at(x++, GateType::SQRT_X);
    p.set_gate_at(x++, GateType::H_NYZ);
    p.set_gate_at(x++, GateType::H_YZ);

    p.set_gate_at(x++, GateType::C_XYZ);
    p.set_gate_at(x++, GateType::C_XYNZ);
    p.set_gate_at(x++, GateType::C_NXYZ);
    p.set_gate_at(x++, GateType::C_XNYZ);

    p.set_gate_at(x++, GateType::C_ZYX);
    p.set_gate_at(x++, GateType::C_ZNYX);
    p.set_gate_at(x++, GateType::C_NZYX);
    p.set_gate_at(x++, GateType::C_ZYNX);

    ASSERT_EQ(p.str(), "_I _X _Y _Z HI HX HY HZ SI SX SY SZ VI VX VY VZ uI uX uY uZ dI dX dY dZ");

    x = 0;

    EXPECT_EQ(p.gate_at(x++), GateType::I);
    EXPECT_EQ(p.gate_at(x++), GateType::X);
    EXPECT_EQ(p.gate_at(x++), GateType::Y);
    EXPECT_EQ(p.gate_at(x++), GateType::Z);

    EXPECT_EQ(p.gate_at(x++), GateType::H);
    EXPECT_EQ(p.gate_at(x++), GateType::SQRT_Y_DAG);
    EXPECT_EQ(p.gate_at(x++), GateType::H_NXZ);
    EXPECT_EQ(p.gate_at(x++), GateType::SQRT_Y);

    EXPECT_EQ(p.gate_at(x++), GateType::S);
    EXPECT_EQ(p.gate_at(x++), GateType::H_XY);
    EXPECT_EQ(p.gate_at(x++), GateType::H_NXY);
    EXPECT_EQ(p.gate_at(x++), GateType::S_DAG);

    EXPECT_EQ(p.gate_at(x++), GateType::SQRT_X_DAG);
    EXPECT_EQ(p.gate_at(x++), GateType::SQRT_X);
    EXPECT_EQ(p.gate_at(x++), GateType::H_NYZ);
    EXPECT_EQ(p.gate_at(x++), GateType::H_YZ);

    EXPECT_EQ(p.gate_at(x++), GateType::C_XYZ);
    EXPECT_EQ(p.gate_at(x++), GateType::C_XYNZ);
    EXPECT_EQ(p.gate_at(x++), GateType::C_NXYZ);
    EXPECT_EQ(p.gate_at(x++), GateType::C_XNYZ);

    EXPECT_EQ(p.gate_at(x++), GateType::C_ZYX);
    EXPECT_EQ(p.gate_at(x++), GateType::C_ZNYX);
    EXPECT_EQ(p.gate_at(x++), GateType::C_NZYX);
    EXPECT_EQ(p.gate_at(x++), GateType::C_ZYNX);
});

TEST_EACH_WORD_SIZE_W(clifford_string, multiplication_table_vs_tableau_multiplication, {
    std::vector<Gate> single_qubit_gates = single_qubit_clifford_rotations();

    std::map<std::string, GateType> t2g;
    for (const auto &g : single_qubit_gates) {
        t2g[g.tableau<W>().str()] = g.id;
    }

    CliffordString<W> p1 = CliffordString<W>(24 * 24);
    CliffordString<W> p2 = CliffordString<W>(24 * 24);
    CliffordString<W> p12 = CliffordString<W>(24 * 24);
    for (size_t k1 = 0; k1 < 24; k1++) {
        for (size_t k2 = 0; k2 < 24; k2++) {
            size_t k = k1 * 24 + k2;
            Gate g1 = single_qubit_gates[k1];
            Gate g2 = single_qubit_gates[k2];
            p1.set_gate_at(k, g1.id);
            p2.set_gate_at(k, g2.id);
            auto t1 = g1.tableau<W>();
            auto t2 = g2.tableau<W>();
            auto t3 = t2.then(t1);
            auto g3 = t2g[t3.str()];
            p12.set_gate_at(k, g3);
        }
    }
    ASSERT_EQ(p1 * p2, p12);
})

TEST_EACH_WORD_SIZE_W(clifford_string, to_from_circuit, {
    Circuit circuit(R"CIRCUIT(
        H 0
        H_XY 1
        H_YZ 2
        H_NXY 3
        H_NXZ 4
        H_NYZ 5
        S_DAG 6
        X 7
        Y 8
        Z 9
        C_XYZ 10
        C_ZYX 11
        C_NXYZ 12
        C_XNYZ 13
        C_XYNZ 14
        C_NZYX 15
        C_ZNYX 16
        C_ZYNX 17
        SQRT_X 18
        SQRT_X_DAG 19
        SQRT_Y 20
        SQRT_Y_DAG 21
        S 22
        I 23
    )CIRCUIT");
    CliffordString<W> s = CliffordString<W>::from_circuit(circuit);
    ASSERT_EQ(s.to_circuit(), circuit);
})

TEST_EACH_WORD_SIZE_W(clifford_string, known_identities, {
    auto s1 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        H 0
    )CIRCUIT"));
    auto s2 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        H 0
    )CIRCUIT"));
    auto s3 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        I 0
    )CIRCUIT"));
    ASSERT_EQ(s2 * s1, s3);

    s1 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        S 0
    )CIRCUIT"));
    s2 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        S 0
    )CIRCUIT"));
    s3 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        Z 0
    )CIRCUIT"));
    ASSERT_EQ(s2 * s1, s3);

    s1 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        S_DAG 0
    )CIRCUIT"));
    s2 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        H 0
    )CIRCUIT"));
    s3 = CliffordString<W>::from_circuit(Circuit(R"CIRCUIT(
        C_XYZ 0
    )CIRCUIT"));
    ASSERT_EQ(s2 * s1, s3);
})

TEST_EACH_WORD_SIZE_W(clifford_string, random, {
    auto rng = INDEPENDENT_TEST_RNG();
    CliffordString<W> c = CliffordString<W>::random(256, rng);
    std::array<uint64_t, NUM_DEFINED_GATES> counts{};
    for (size_t k = 0; k < 256; k++) {
        for (size_t q = 0; q < 256; q++) {
            GateType t = c.gate_at(q);
            counts[(uint8_t)t] += 1;
        }
        c.randomize(rng);
    }
    ASSERT_EQ(counts[(uint8_t)GateType::NOT_A_GATE], 0);
    size_t seen_gates = 0;
    for (const auto &g : GATE_DATA.items) {
        if ((g.flags & GATE_IS_UNITARY) && (g.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
            ASSERT_LT(counts[(uint8_t)g.id], 256.0*256.0/24.0*(1.0 + 0.5));
            ASSERT_GT(counts[(uint8_t)g.id], 256.0*256.0/24.0*(1.0 - 0.5));
            seen_gates++;
        }
    }
    ASSERT_EQ(seen_gates, 24);
})
