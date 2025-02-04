#include "stim/simulators/graph_simulator.h"

#include "gtest/gtest.h"

#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

void expect_graph_circuit_is_equivalent(const Circuit &circuit) {
    auto n = circuit.count_qubits();
    GraphSimulator sim(n);
    sim.do_circuit(circuit);
    sim.verify_invariants();
    Circuit converted = sim.to_circuit();
    TableauSimulator<64> sim1(std::mt19937_64{}, n);
    TableauSimulator<64> sim2(std::mt19937_64{}, n);
    sim1.safe_do_circuit(circuit);
    sim2.safe_do_circuit(converted);
    auto s1 = sim1.canonical_stabilizers();
    auto s2 = sim2.canonical_stabilizers();
    if (s1 != s2) {
        EXPECT_EQ(s1, s2) << "\nACTUAL:\n"
                          << stim_draw_internal::DiagramTimelineAsciiDrawer::make_diagram(circuit) << "\nCONVERTED:\n"
                          << stim_draw_internal::DiagramTimelineAsciiDrawer::make_diagram(converted);
    }
}

void expect_graph_sim_effect_matches_tableau_sim(const GraphSimulator &state, const Circuit &effect) {
    GraphSimulator state_after_effect = state;
    auto n = state_after_effect.num_qubits;
    state_after_effect.do_circuit(effect);
    state_after_effect.verify_invariants();

    TableauSimulator<64> tableau_sim1(std::mt19937_64{}, n);
    TableauSimulator<64> tableau_sim2(std::mt19937_64{}, n);
    tableau_sim1.safe_do_circuit(state.to_circuit());
    tableau_sim1.safe_do_circuit(effect);
    tableau_sim2.safe_do_circuit(state_after_effect.to_circuit());
    auto s1 = tableau_sim1.canonical_stabilizers();
    auto s2 = tableau_sim2.canonical_stabilizers();
    if (s1 != s2) {
        EXPECT_EQ(s1, s2) << "EFFECT:\nstim::Circuit(R\"CIRCUIT(\n"
                          << effect << "\n)CIRCUIT\")" << "\n"
                          << "STATE:\n"
                          << state << "\n";
    }
}

TEST(graph_simulator, converts_circuits) {
    expect_graph_circuit_is_equivalent(Circuit(R"CIRCUIT(
        H 2 3 5
        S 0 1 2
        C_XYZ 1 2 3
        S_DAG 1
        H_YZ 5 4
        SQRT_Y 2 4
        X 1
    )CIRCUIT"));

    expect_graph_circuit_is_equivalent(Circuit(R"CIRCUIT(
        H 0 1
        CZ 0 1
    )CIRCUIT"));

    expect_graph_circuit_is_equivalent(Circuit(R"CIRCUIT(
        H 0 1 2 3
        CZ 0 1 0 2 0 3
    )CIRCUIT"));

    expect_graph_circuit_is_equivalent(Circuit(R"CIRCUIT(
        H 0 1 2 3
        CZ 0 1
        CX 2 1
    )CIRCUIT"));
}

TEST(graph_simulator, after2inside_basis_transform) {
    GraphSimulator sim(6);
    //                            VI+SH-
    sim.x2outs = PauliString<64>("XXYYZZ");
    sim.z2outs = PauliString<64>("YZXZXY");

    ASSERT_EQ(sim.after2inside_basis_transform(0u, 1, 0), (std::make_tuple<bool, bool, bool>(1, 0, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(0u, 1, 1), (std::make_tuple<bool, bool, bool>(0, 1, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(0u, 0, 1), (std::make_tuple<bool, bool, bool>(1, 1, true)));

    ASSERT_EQ(sim.after2inside_basis_transform(1u, 1, 0), (std::make_tuple<bool, bool, bool>(1, 0, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(1u, 0, 1), (std::make_tuple<bool, bool, bool>(0, 1, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(1u, 1, 1), (std::make_tuple<bool, bool, bool>(1, 1, false)));

    ASSERT_EQ(sim.after2inside_basis_transform(2u, 1, 1), (std::make_tuple<bool, bool, bool>(1, 0, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(2u, 1, 0), (std::make_tuple<bool, bool, bool>(0, 1, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(2u, 0, 1), (std::make_tuple<bool, bool, bool>(1, 1, false)));

    ASSERT_EQ(sim.after2inside_basis_transform(3u, 1, 1), (std::make_tuple<bool, bool, bool>(1, 0, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(3u, 0, 1), (std::make_tuple<bool, bool, bool>(0, 1, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(3u, 1, 0), (std::make_tuple<bool, bool, bool>(1, 1, true)));

    ASSERT_EQ(sim.after2inside_basis_transform(4u, 0, 1), (std::make_tuple<bool, bool, bool>(1, 0, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(4u, 1, 0), (std::make_tuple<bool, bool, bool>(0, 1, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(4u, 1, 1), (std::make_tuple<bool, bool, bool>(1, 1, true)));

    ASSERT_EQ(sim.after2inside_basis_transform(5u, 0, 1), (std::make_tuple<bool, bool, bool>(1, 0, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(5u, 1, 1), (std::make_tuple<bool, bool, bool>(0, 1, false)));
    ASSERT_EQ(sim.after2inside_basis_transform(5u, 1, 0), (std::make_tuple<bool, bool, bool>(1, 1, false)));
}

TEST(graph_simulator, to_hs_xyz) {
    GraphSimulator sim(10);
    sim.do_circuit(Circuit(R"CIRCUIT(
        H 0 1 2 3 4 5 6 7 8 9
        I 0
        H 1
        S 2
        SQRT_X_DAG 3
        C_XYZ 4
        C_ZYX 5
        X 6
        Y 7
        Z 8
        H 9
        Z 9
    )CIRCUIT"));
    ASSERT_EQ(sim.to_circuit(true), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5 6 7 8 9
        TICK
        X 6 9
        Y 7
        Z 4 8
        S 2 3 4
        H 1 3 4 5 9
        S 3 5
    )CIRCUIT"));
}

TEST(graph_simulator, inside_do_sqrt_z) {
    GraphSimulator sim(10);
    sim.do_circuit(Circuit(R"CIRCUIT(
        H 0 1 2 3 4 5 6 7 8 9
        I 0
        H 1
        S 2
        SQRT_X_DAG 3
        C_XYZ 4
        C_ZYX 5
        X 6
        Y 7
        Z 8
        H 9
        Z 9
    )CIRCUIT"));
    ASSERT_EQ(sim.to_circuit(), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5 6 7 8 9
        TICK
        X 6 9
        Y 7
        Z 8
        C_XYZ 4
        C_ZYX 5
        H 1 9
        S 2
        SQRT_X_DAG 3
    )CIRCUIT"));

    for (size_t q = 0; q < sim.num_qubits; q++) {
        sim.inside_do_sqrt_z(q);
    }
    sim.verify_invariants();
    ASSERT_EQ(sim.to_circuit(), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5 6 7 8 9
        TICK
        X 7 9
        Y 6
        Z 1 2 3 8
        C_XYZ 1 9
        C_ZYX 3
        H 4
        S 0 6 7 8
        SQRT_X_DAG 5
    )CIRCUIT"));
}

TEST(graph_simulator, inside_do_sqrt_x_dag) {
    GraphSimulator sim(10);
    sim.do_circuit(Circuit(R"CIRCUIT(
        H 0 1 2 3 4 5 6 7 8 9
        I 0
        H 1
        S 2
        SQRT_X_DAG 3
        C_XYZ 4
        C_ZYX 5
        X 6
        Y 7
        Z 8
        H 9
        Z 9
    )CIRCUIT"));
    ASSERT_EQ(sim.to_circuit(), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5 6 7 8 9
        TICK
        X 6 9
        Y 7
        Z 8
        C_XYZ 4
        C_ZYX 5
        H 1 9
        S 2
        SQRT_X_DAG 3
    )CIRCUIT"));

    for (size_t q = 0; q < sim.num_qubits; q++) {
        sim.inside_do_sqrt_x_dag(q);
    }
    sim.verify_invariants();
    ASSERT_EQ(sim.to_circuit(), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5 6 7 8 9
        TICK
        X 1 2 3 6
        Y 8
        Z 7
        C_XYZ 2
        C_ZYX 1 9
        H 5
        S 4
        SQRT_X_DAG 0 6 7 8
    )CIRCUIT"));
}

TEST(graph_simulator, do_complementation) {
    for (size_t k = 0; k < 50; k++) {
        auto rng = INDEPENDENT_TEST_RNG();
        GraphSimulator sim = GraphSimulator::random_state(8, rng);
        GraphSimulator sim2 = sim;
        sim2.do_complementation(3);
        sim.verify_invariants();
        sim2.verify_invariants();

        TableauSimulator<64> tableau_sim1(std::mt19937_64{}, 8);
        TableauSimulator<64> tableau_sim2(std::mt19937_64{}, 8);
        tableau_sim1.safe_do_circuit(sim.to_circuit());
        tableau_sim2.safe_do_circuit(sim2.to_circuit());
        auto s1 = tableau_sim1.canonical_stabilizers();
        auto s2 = tableau_sim2.canonical_stabilizers();
        if (s1 != s2) {
            ASSERT_EQ(s1, s2) << sim;
        }
    }
}

TEST(graph_simulator, all_unitary_gates_work) {
    auto rng = INDEPENDENT_TEST_RNG();
    std::vector<GateTarget> targets{GateTarget::qubit(2), GateTarget::qubit(5)};
    SpanRef<GateTarget> t2 = targets;
    SpanRef<GateTarget> t1 = t2;
    t1.ptr_end--;
    for (const auto &gate : GATE_DATA.items) {
        if (!gate.has_known_unitary_matrix()) {
            continue;
        }
        Circuit circuit;
        circuit.safe_append(CircuitInstruction(gate.id, {}, (gate.flags & GATE_TARGETS_PAIRS) ? t2 : t1, ""));
        for (size_t k = 0; k < 20; k++) {
            expect_graph_sim_effect_matches_tableau_sim(GraphSimulator::random_state(8, rng), circuit);
        }
    }
}

TEST(graph_simulator, to_circuit_single_qubit_gates) {
    GraphSimulator sim(6);
    ASSERT_EQ(sim.to_circuit(), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5
        TICK
        H 0 1 2 3 4 5
    )CIRCUIT"));

    sim.do_circuit(Circuit(R"CIRCUIT(
        H 0 1 2 3 4 5
    )CIRCUIT"));
    ASSERT_EQ(sim.to_circuit(), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5
        TICK
    )CIRCUIT"));

    sim.do_circuit(Circuit(R"CIRCUIT(
        H 0
        S 1
        C_XYZ 2 3 3
        SQRT_X_DAG 4
    )CIRCUIT"));
    ASSERT_EQ(sim.to_circuit(), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5
        TICK
        C_XYZ 2
        C_ZYX 3
        H 0
        S 1
        SQRT_X_DAG 4
    )CIRCUIT"));

    sim.do_circuit(Circuit(R"CIRCUIT(
        X 0
        S 1
        Y 2
        Z 3
    )CIRCUIT"));
    ASSERT_EQ(sim.to_circuit(), Circuit(R"CIRCUIT(
        RX 0 1 2 3 4 5
        TICK
        X 2 3
        Z 0 1
        C_XYZ 2
        C_ZYX 3
        H 0
        SQRT_X_DAG 4
    )CIRCUIT"));
}
