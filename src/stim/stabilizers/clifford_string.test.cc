#include "stim/stabilizers/clifford_string.h"

#include "gtest/gtest.h"
#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(clifford_string, mul, {
    std::vector<Gate> single_qubit_gates;
    for (size_t g = 0; g < NUM_DEFINED_GATES; g++) {
        Gate gate = GATE_DATA[(GateType)g];
        if ((gate.flags & GateFlags::GATE_IS_SINGLE_QUBIT_GATE) && (gate.flags & GateFlags::GATE_IS_UNITARY)) {
            single_qubit_gates.push_back(gate);
        }
    }
    ASSERT_EQ(single_qubit_gates.size(), 24);

    std::map<std::string, GateType> t2g;
    for (const auto &g : single_qubit_gates) {
        t2g[g.tableau<W>().str()] = g.id;
    }

    CliffordString<W> p1 = CliffordString<W>::uninitialized(24 * 24);
    CliffordString<W> p2 = CliffordString<W>::uninitialized(24 * 24);
    CliffordString<W> p12 = CliffordString<W>::uninitialized(24 * 24);
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
})
