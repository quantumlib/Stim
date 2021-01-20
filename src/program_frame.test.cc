#include "gtest/gtest.h"
#include "program_frame.h"
#include "simulators/sim_tableau.h"

std::string convert_program(const std::string &program) {
    return PauliFrameProgram::from_stabilizer_circuit(Circuit::from_text(program).operations).str();
}

TEST(PauliFrameProgram, recorded_from_tableau_sim) {
    ASSERT_EQ(
            convert_program(
                    "X 0\n"
                    "M 1\n"
                    "M 0\n"
                    "M 2\n"
                    "M 3\n"
            ),
        "M_DET 1 !0 2 3\n"
    );

    ASSERT_EQ(
            convert_program(
                    "H 0\n"
                    "CNOT 0 1\n"
                    "M 0\n"
                    "M 1\n"
            ),
            "H 0\n"
            "CNOT 0 1\n"
            "RANDOM_KICKBACK X0*X1\n"
            "M_DET 0 1\n"
    );

    ASSERT_EQ(
            convert_program(
                    "H 0\n"
                    "CNOT 0 1\n"
                    "X 0\n"
                    "M 0\n"
                    "M 1\n"
            ),
            "H 0\n"
            "CNOT 0 1\n"
            "RANDOM_KICKBACK X0*X1\n"
            "M_DET !0 1\n"
    );

    ASSERT_EQ(
            convert_program(
                    "H 0\n"
                    "CNOT 0 1\n"
                    "X 1\n"
                    "M 0\n"
                    "M 1\n"
            ),
            "H 0\n"
            "CNOT 0 1\n"
            "RANDOM_KICKBACK X0*X1\n"
            "M_DET 0 !1\n"
    );

    ASSERT_EQ(
            convert_program(
                    "H 0\n"
                    "CNOT 0 1\n"
                    "X 1\n"
                    "M 0\n"
                    "M 1\n"
            ),
            "H 0\n"
            "CNOT 0 1\n"
            "RANDOM_KICKBACK X0*X1\n"
            "M_DET 0 !1\n"
    );

    ASSERT_EQ(
            convert_program(
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
            "RANDOM_KICKBACK X0*X2\n"
            "RANDOM_KICKBACK X1*X3\n"
            "M_DET 0 2 3 !1\n"
    );
}
