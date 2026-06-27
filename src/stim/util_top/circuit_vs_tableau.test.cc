#include "stim/util_top/circuit_vs_tableau.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(conversions, circuit_to_tableau_ignoring_gates, {
    Circuit unitary(R"CIRCUIT(
        I 0
        X 0
        Y 0
        Z 0
        C_XYZ 0
        C_ZYX 0
        H 0
        H_XY 0
        H_XZ 0
        H_YZ 0
        S 0
        SQRT_X 0
        SQRT_X_DAG 0
        SQRT_Y 0
        SQRT_Y_DAG 0
        SQRT_Z 0
        SQRT_Z_DAG 0
        S_DAG 0
        CNOT 0 1
        CX 0 1
        CY 0 1
        CZ 0 1
        ISWAP 0 1
        ISWAP_DAG 0 1
        SQRT_XX 0 1
        SQRT_XX_DAG 0 1
        SQRT_YY 0 1
        SQRT_YY_DAG 0 1
        SQRT_ZZ 0 1
        SQRT_ZZ_DAG 0 1
        SWAP 0 1
        XCX 0 1
        XCY 0 1
        XCZ 0 1
        YCX 0 1
        YCY 0 1
        YCZ 0 1
        ZCX 0 1
        ZCY 0 1
        ZCZ 0 1
    )CIRCUIT");
    Circuit noise(R"CIRCUIT(
        CORRELATED_ERROR(0.1) X0
        DEPOLARIZE1(0.1) 0
        DEPOLARIZE2(0.1) 0 1
        E(0.1) X0
        ELSE_CORRELATED_ERROR(0.1) Y1
        PAULI_CHANNEL_1(0.1,0.2,0.3) 0
        PAULI_CHANNEL_2(0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01) 0 1
        X_ERROR(0.1) 0
        Y_ERROR(0.1) 0
        Z_ERROR(0.1) 0
    )CIRCUIT");
    Circuit measure(R"CIRCUIT(
        M 0
        MPP X0
        MX 0
        MY 0
        MZ 0
    )CIRCUIT");
    Circuit reset(R"CIRCUIT(
        R 0
        RX 0
        RY 0
        RZ 0
    )CIRCUIT");
    Circuit measure_reset(R"CIRCUIT(
        MR 0
        MRX 0
        MRY 0
        MRZ 0
    )CIRCUIT");
    Circuit annotations(R"CIRCUIT(
        REPEAT 10 {
            I 0
        }
        DETECTOR(1, 2)
        OBSERVABLE_INCLUDE(1)
        QUBIT_COORDS(0,1,2) 0
        SHIFT_COORDS(2, 3, 4)
        TICK
    )CIRCUIT");

    ASSERT_EQ(circuit_to_tableau<W>(unitary, false, false, false).num_qubits, 2);

    ASSERT_THROW({ circuit_to_tableau<W>(noise, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(noise, false, true, true); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau<W>(noise, true, false, false), Tableau<W>(2));

    ASSERT_THROW({ circuit_to_tableau<W>(measure, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(measure, true, false, true); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau<W>(measure, false, true, false), Tableau<W>(1));

    ASSERT_THROW({ circuit_to_tableau<W>(reset, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(reset, true, true, false); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau<W>(reset, false, false, true), Tableau<W>(1));

    ASSERT_THROW({ circuit_to_tableau<W>(measure_reset, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(measure_reset, true, false, true); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(measure_reset, true, true, false); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau<W>(measure_reset, false, true, true), Tableau<W>(1));

    ASSERT_EQ(circuit_to_tableau<W>(annotations, false, false, false), Tableau<W>(1));

    ASSERT_EQ(
        circuit_to_tableau<W>(annotations + measure_reset + measure + reset + unitary + noise, true, true, true)
            .num_qubits,
        2);
})

TEST_EACH_WORD_SIZE_W(conversions, circuit_to_tableau, {
    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
        )CIRCUIT"),
            false,
            false,
            false),
        Tableau<W>(0));

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            REPEAT 10 {
                X 0
                TICK
            }
        )CIRCUIT"),
            false,
            false,
            false),
        Tableau<W>(1));

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            REPEAT 11 {
                X 0
                TICK
            }
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("X").tableau<W>());

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            S 0
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("S").tableau<W>());

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            SQRT_Y_DAG 1
            CZ 0 1
            SQRT_Y 1
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("CX").tableau<W>());

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            R 0
            X_ERROR(0.1) 0
            SQRT_Y_DAG 1
            CZ 0 1
            SQRT_Y 1
            M 0
        )CIRCUIT"),
            true,
            true,
            true),
        GATE_DATA.at("CX").tableau<W>());
})

TEST_EACH_WORD_SIZE_W(conversions, tableau_to_circuit_fuzz_vs_circuit_to_tableau, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t n = 0; n < 10; n++) {
        auto desired = Tableau<W>::random(n, rng);
        Circuit circuit = tableau_to_circuit<W>(desired, "elimination");
        auto actual = circuit_to_tableau<W>(circuit, false, false, false);
        ASSERT_EQ(actual, desired);

        for (const auto &op : circuit.operations) {
            ASSERT_TRUE(op.gate_type == GateType::S || op.gate_type == GateType::H || op.gate_type == GateType::CX)
                << op;
        }
    }
})

TEST_EACH_WORD_SIZE_W(conversions, tableau_to_circuit, {
    ASSERT_EQ(tableau_to_circuit<W>(GATE_DATA.at("I").tableau<W>(), "elimination"), Circuit(R"CIRCUIT(
            H 0
            H 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit<W>(GATE_DATA.at("X").tableau<W>(), "elimination"), Circuit(R"CIRCUIT(
            H 0
            S 0
            S 0
            H 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit<W>(GATE_DATA.at("S").tableau<W>(), "elimination"), Circuit(R"CIRCUIT(
            S 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit<W>(GATE_DATA.at("ISWAP").tableau<W>(), "elimination"), Circuit(R"CIRCUIT(
            CX 1 0 0 1 1 0
            S 0
            H 1
            CX 0 1
            H 1
            S 1
        )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(conversions, fuzz_mpp_circuit_produces_correct_state, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto tableau = Tableau<W>::random(10, rng);
    auto circuit = tableau_to_circuit_mpp_method<W>(tableau, false);
    TableauSimulator<W> sim(std::move(rng), 10);
    sim.safe_do_circuit(circuit);
    auto expected = tableau.stabilizers(true);
    auto actual = sim.canonical_stabilizers();
    ASSERT_EQ(actual, expected);
})

TEST_EACH_WORD_SIZE_W(conversions, fuzz_mpp_circuit_produces_correct_state_unsigned, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto tableau = Tableau<W>::random(10, rng);
    auto circuit = tableau_to_circuit_mpp_method<W>(tableau, true);
    TableauSimulator<W> sim(std::move(rng), 10);
    sim.safe_do_circuit(circuit);
    auto expected = tableau.stabilizers(true);
    auto actual = sim.canonical_stabilizers();
    for (auto &e : expected) {
        e.sign = false;
    }
    for (auto &e : actual) {
        e.sign = false;
    }
    ASSERT_EQ(actual, expected);
})

TEST_EACH_WORD_SIZE_W(conversions, perfect_code_mpp_circuit, {
    Tableau<W> tableau(5);

    tableau.zs[0] = PauliString<W>("XZZX_");
    tableau.zs[1] = PauliString<W>("_XZZX");
    tableau.zs[2] = PauliString<W>("X_XZZ");
    tableau.zs[3] = PauliString<W>("ZX_XZ");
    tableau.zs[4] = PauliString<W>("ZZZZZ");

    tableau.xs[0] = PauliString<W>("Z_Z__");
    tableau.xs[1] = PauliString<W>("ZZZZ_");
    tableau.xs[2] = PauliString<W>("ZZ_ZZ");
    tableau.xs[3] = PauliString<W>("_Z__Z");
    tableau.xs[4] = PauliString<W>("XXXXX");

    ASSERT_TRUE(tableau.satisfies_invariants());

    ASSERT_EQ(tableau_to_circuit_mpp_method<W>(tableau, true), Circuit(R"CIRCUIT(
        MPP X0*Z1*Z2*X3 X1*Z2*Z3*X4 X0*X2*Z3*Z4 Z0*X1*X3*Z4 Z0*Z1*Z2*Z3*Z4
    )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit_mpp_method<W>(tableau, false), Circuit(R"CIRCUIT(
        MPP X0*Z1*Z2*X3 X1*Z2*Z3*X4 X0*X2*Z3*Z4 Z0*X1*X3*Z4 Z0*Z1*Z2*Z3*Z4
        CX rec[-1] 0 rec[-1] 1 rec[-1] 2 rec[-1] 3 rec[-1] 4
        CZ rec[-5] 0 rec[-5] 2 rec[-4] 0 rec[-4] 1 rec[-4] 2 rec[-4] 3 rec[-3] 0 rec[-3] 1 rec[-3] 3 rec[-3] 4 rec[-2] 1 rec[-2] 4
    )CIRCUIT"));
})
