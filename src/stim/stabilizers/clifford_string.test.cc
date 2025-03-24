#include "stim/stabilizers/clifford_string.h"

#include "gtest/gtest.h"
#include "stim/mem/simd_word.test.h"
#include "stim/stabilizers/tableau.h"

using namespace stim;

std::vector<Gate> single_qubit_clifford_rotations() {
    std::vector<Gate> result;
    for (size_t g = 0; g < NUM_DEFINED_GATES; g++) {
        Gate gate = GATE_DATA[(GateType)g];
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

    ASSERT_EQ(p.str(), "_I _X _Y _Z HI HX HY HZ SI SX SY SZ VI VX VY VZ UI UX UY UZ DI DX DY DZ");

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
