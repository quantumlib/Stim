#include "gtest/gtest.h"
#include "sim_frame.h"
#include "sim_tableau.h"

std::string record(const std::string &program) {
    return SimFrame::recorded_from_tableau_sim(Circuit::from_text(program).operations).str();
}

bool is_output_possible_no_bare_resets(const Circuit &circuit, const aligned_bits256 &output) {
    auto tableau_sim = SimTableau(circuit.num_qubits);
    size_t out_p = 0;
    for (const auto &op : circuit.operations) {
        if (op.name == "TICK") {
            continue;
        }
        if (op.name == "M") {
            for (auto q : op.targets) {
                bool b = output.get_bit(out_p);
                if (tableau_sim.measure(q, b) != b) {
                    return false;
                }
                out_p++;
            }
        } else if (op.name == "R") {
            tableau_sim.reset_many(op.targets);
        } else {
            tableau_sim.func_op(op.name, op.targets);
        }
    }
    return true;
}

TEST(PauliFrameSimulation, test_util_is_output_possible) {
    auto circuit = Circuit::from_text(
            "H 0\n"
            "CNOT 0 1\n"
            "X 0\n"
            "M 0\n"
            "M 1\n");
    auto data = aligned_bits256(2);
    data.u64[0] = 0b00;
    ASSERT_EQ(false, is_output_possible_no_bare_resets(circuit, data));
    data.u64[0] = 0b01;
    ASSERT_EQ(true, is_output_possible_no_bare_resets(circuit, data));
    data.u64[0] = 0b10;
    ASSERT_EQ(true, is_output_possible_no_bare_resets(circuit, data));
    data.u64[0] = 0b11;
    ASSERT_EQ(false, is_output_possible_no_bare_resets(circuit, data));
}

bool is_frame_sim_output_consistent_with_tableau_sim(const std::string &program) {
    auto circuit = Circuit::from_text(program);
    auto frame_sim = SimFrame::recorded_from_tableau_sim(circuit.operations);
    std::mt19937 rng((std::random_device {})());
    auto out = aligned_bits256(frame_sim.num_measurements);
    for (size_t reps = 0; reps < 10; reps++) {
        frame_sim.sample(out, rng);
        if (!is_output_possible_no_bare_resets(circuit, out)) {
            std::cerr << "Impossible output: ";
            for (size_t k = 0; k < frame_sim.num_measurements; k++) {
                std::cerr << "01"[out.get_bit(k)];
            }
            std::cerr << "\n";
            return false;
        }
    }
    return true;
}

TEST(PauliFrameSimulation, epr_pair) {
    ASSERT_EQ(
        record(
            "H 0\n"
            "CNOT 0 1\n"
            "M 0\n"
            "M 1\n"
        ),
        "H 0\n"
        "CNOT 0 1\n"
        "RANDOM_INTO_FRAME X0*X1\n"
        "REPORT 0 1\n"
    );

    ASSERT_EQ(
        record(
            "H 0\n"
            "CNOT 0 1\n"
            "X 0\n"
            "M 0\n"
            "M 1\n"
        ),
        "H 0\n"
        "CNOT 0 1\n"
        "RANDOM_INTO_FRAME X0*X1\n"
        "REPORT !0 1\n"
    );

    ASSERT_EQ(
        record(
            "H 0\n"
            "CNOT 0 1\n"
            "X 1\n"
            "M 0\n"
            "M 1\n"
        ),
        "H 0\n"
        "CNOT 0 1\n"
        "RANDOM_INTO_FRAME X0*X1\n"
        "REPORT 0 !1\n"
    );

    ASSERT_EQ(
        record(
            "H 0\n"
            "CNOT 0 1\n"
            "X 1\n"
            "M 0\n"
            "M 1\n"
        ),
        "H 0\n"
        "CNOT 0 1\n"
        "RANDOM_INTO_FRAME X0*X1\n"
        "REPORT 0 !1\n"
    );

    ASSERT_EQ(
        record(
            "H 0\n"
            "H 1\n"
            "CNOT 0 2\n"
            "CNOT 1 3\n"
            "X 1\n"
            "M 0\n"
            "M 2\n"
            "M 3\n"
            "M 1\n"
        ),
        "H 0 1\n"
        "CNOT 0 2 1 3\n"
        "RANDOM_INTO_FRAME X0*X2\n"
        "RANDOM_INTO_FRAME X1*X3\n"
        "REPORT 0 2 3 !1\n"
    );
}

#define CHECK_CONSISTENCY(program) EXPECT_TRUE(is_frame_sim_output_consistent_with_tableau_sim(program)) << record(program)

TEST(PauliFrameSimulation, consistency) {
    CHECK_CONSISTENCY(
            "H 0\n"
            "CNOT 0 1\n"
            "M 0\n"
            "M 1\n"
    );

    CHECK_CONSISTENCY(
            "H 0\n"
            "H 1\n"
            "CNOT 0 2\n"
            "CNOT 1 3\n"
            "X 1\n"
            "M 0\n"
            "M 2\n"
            "M 3\n"
            "M 1\n"
    );

    CHECK_CONSISTENCY(
            "H 0\n"
            "CNOT 0 1\n"
            "X 0\n"
            "M 0\n"
            "M 1\n"
    );

    CHECK_CONSISTENCY(
            "H 0\n"
            "CNOT 0 1\n"
            "Z 0\n"
            "M 0\n"
            "M 1\n"
    );

    CHECK_CONSISTENCY(
            "H 0\n"
            "M 0\n"
            "H 0\n"
            "M 0\n"
            "R 0\n"
            "H 0\n"
            "M 0\n"
    );

    CHECK_CONSISTENCY(
            "H 0\n"
            "CNOT 0 1\n"
            "M 0\n"
            "R 0\n"
            "M 0\n"
            "M 1\n"
    );

    // Distance 2 surface code.
    CHECK_CONSISTENCY(
            "R 0\n"
            "R 1\n"
            "R 5\n"
            "R 6\n"
            "tick\n"
            "tick\n"
            "R 2\n"
            "R 3\n"
            "R 4\n"
            "tick\n"
            "H 2\n"
            "H 3\n"
            "H 4\n"
            "tick\n"
            "CZ 3 0\n"
            "CNOT 4 1\n"
            "tick\n"
            "CZ 3 1\n"
            "CNOT 4 6\n"
            "tick\n"
            "CNOT 2 0\n"
            "CZ 3 5\n"
            "tick\n"
            "CNOT 2 5\n"
            "CZ 3 6\n"
            "tick\n"
            "H 2\n"
            "H 3\n"
            "H 4\n"
            "tick\n"
            "M 2\n"
            "M 3\n"
            "M 4\n"
            "tick\n"
            "R 2\n"
            "R 3\n"
            "R 4\n"
            "tick\n"
            "H 2\n"
            "H 3\n"
            "H 4\n"
            "tick\n"
            "CZ 3 0\n"
            "CNOT 4 1\n"
            "tick\n"
            "CZ 3 1\n"
            "CNOT 4 6\n"
            "tick\n"
            "CNOT 2 0\n"
            "CZ 3 5\n"
            "tick\n"
            "CNOT 2 5\n"
            "CZ 3 6\n"
            "tick\n"
            "H 2\n"
            "H 3\n"
            "H 4\n"
            "tick\n"
            "M 2\n"
            "M 3\n"
            "M 4\n"
            "tick\n"
            "tick\n"
            "M 0\n"
            "M 1\n"
            "M 5\n"
            "M 6");
}
