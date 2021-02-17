#include "error_fuser.h"
#include "frame_simulator.h"
#include "../test_util.test.h"

#include <gtest/gtest.h>

#define ASSERT_APPROX_EQ(c1, c2, atol) ASSERT_TRUE(c1.approx_equals(c2, atol)) << c1

TEST(ErrorFuser, convert_circuit) {
    ASSERT_EQ(
        ErrorFuser::convert_circuit(Circuit::from_text(R"circuit(
        X_ERROR(0.25) 3
        M 3
        DETECTOR 3@-1
    )circuit")),
        Circuit::from_text(R"circuit(
        CORRELATED_ERROR(0.25) X0
    )circuit"));

    ASSERT_EQ(
        ErrorFuser::convert_circuit(Circuit::from_text(R"circuit(
        Y_ERROR(0.25) 3
        M 3
        DETECTOR 3@-1
    )circuit")),
        Circuit::from_text(R"circuit(
        CORRELATED_ERROR(0.25) X0
    )circuit"));

    ASSERT_EQ(
        ErrorFuser::convert_circuit(Circuit::from_text(R"circuit(
        Z_ERROR(0.25) 3
        M 3
        DETECTOR 3@-1
    )circuit")),
        Circuit::from_text(R"circuit(
    )circuit"));

    ASSERT_APPROX_EQ(
        ErrorFuser::convert_circuit(Circuit::from_text(R"circuit(
        DEPOLARIZE1(0.25) 3
        M 3
        DETECTOR 3@-1
    )circuit")),
        Circuit::from_text(R"circuit(
        CORRELATED_ERROR(0.16666666667) X0
    )circuit"),
        1e-8);

    ASSERT_APPROX_EQ(
        ErrorFuser::convert_circuit(Circuit::from_text(R"circuit(
        DEPOLARIZE2(0.25) 3 5
        M 3
        M 5
        DETECTOR 3@-1
        DETECTOR 5@-1
    )circuit")),
        Circuit::from_text(R"circuit(
        CORRELATED_ERROR(0.071825580711162351) X0
        CORRELATED_ERROR(0.071825580711162351) X0 X1
        CORRELATED_ERROR(0.071825580711162351) X1
    )circuit"),
        1e-8);

    ASSERT_APPROX_EQ(
        ErrorFuser::convert_circuit(Circuit::from_text(R"circuit(
        H 0 1
        CNOT 0 2 1 3
        DEPOLARIZE2(0.25) 0 1
        CNOT 0 2 1 3
        H 0 1
        M 0 1 2 3
        DETECTOR 0@-1
        DETECTOR 1@-1
        DETECTOR 2@-1
        DETECTOR 3@-1
    )circuit")),
        Circuit::from_text(R"circuit(
        CORRELATED_ERROR(0.019013726448203538) X0
        CORRELATED_ERROR(0.019013726448203538) X0 X1
        CORRELATED_ERROR(0.019013726448203538) X0 X1 X2
        CORRELATED_ERROR(0.019013726448203538) X0 X1 X2 X3
        CORRELATED_ERROR(0.019013726448203538) X0 X1 X3
        CORRELATED_ERROR(0.019013726448203538) X0 X2
        CORRELATED_ERROR(0.019013726448203538) X0 X2 X3
        CORRELATED_ERROR(0.019013726448203538) X0 X3
        CORRELATED_ERROR(0.019013726448203538) X1
        CORRELATED_ERROR(0.019013726448203538) X1 X2
        CORRELATED_ERROR(0.019013726448203538) X1 X2 X3
        CORRELATED_ERROR(0.019013726448203538) X1 X3
        CORRELATED_ERROR(0.019013726448203538) X2
        CORRELATED_ERROR(0.019013726448203538) X2 X3
        CORRELATED_ERROR(0.019013726448203538) X3
    )circuit"),
        1e-8);
}

TEST(ErrorFuser, unitary_gates_match_frame_simulator) {
    FrameSimulator f(16, 16, 0, SHARED_TEST_RNG());
    ErrorFuser e(16, 16, 0);
    for (size_t q = 0; q < 16; q++) {
        if (q & 1) {
            e.xs[q] ^= 0;
            f.x_table[q][0] = true;
        }
        if (q & 2) {
            e.xs[q] ^= 1;
            f.x_table[q][1] = true;
        }
        if (q & 4) {
            e.zs[q] ^= 0;
            f.z_table[q][0] = true;
        }
        if (q & 8) {
            e.zs[q] ^= 1;
            f.z_table[q][1] = true;
        }
    }


    std::vector<uint32_t> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    OperationData targets = {0, {&data, 0, data.size()}};
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            (e.*gate.hit_simulator_function)(targets);
            (f.*gate.frame_simulator_function)(targets);
            for (size_t q = 0; q < 16; q++) {
                bool xs[2]{};
                bool zs[2]{};
                for (auto x : e.xs[q]) {
                    ASSERT_TRUE(x < 2) << gate.name;
                    xs[x] = true;
                }
                for (auto z : e.zs[q]) {
                    ASSERT_TRUE(z < 2) << gate.name;
                    zs[z] = true;
                }
                ASSERT_EQ(f.x_table[q][0], xs[0]) << gate.name;
                ASSERT_EQ(f.x_table[q][1], xs[1]) << gate.name;
                ASSERT_EQ(f.z_table[q][0], zs[0]) << gate.name;
                ASSERT_EQ(f.z_table[q][1], zs[1]) << gate.name;
            }
        }
    }
}
