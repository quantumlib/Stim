#include "stim/stabilizers/flex_pauli_string.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(flex_pauli_string, mul) {
    FlexPauliString p1 = FlexPauliString::from_text("iXYZ");
    FlexPauliString p2 = FlexPauliString::from_text("i__Z");
    ASSERT_EQ(p1 * p2, FlexPauliString::from_text("-XY_"));
}

TEST(flex_pauli_string, from_text) {
    auto f = FlexPauliString::from_text("-iIXYZ_xyz");
    ASSERT_EQ(f.value.num_qubits, 8);
    ASSERT_EQ(f.imag, true);
    ASSERT_EQ(f.value.sign, true);
    ASSERT_EQ(f.value.xs.as_u64(), 0b01100110);
    ASSERT_EQ(f.value.zs.as_u64(), 0b11001100);

    f = FlexPauliString::from_text("iX");
    ASSERT_EQ(f.value.num_qubits, 1);
    ASSERT_EQ(f.imag, true);
    ASSERT_EQ(f.value.sign, false);
    ASSERT_EQ(f.value.xs.as_u64(), 0b1);
    ASSERT_EQ(f.value.zs.as_u64(), 0b0);

    f = FlexPauliString::from_text("Y");
    ASSERT_EQ(f.value.num_qubits, 1);
    ASSERT_EQ(f.imag, false);
    ASSERT_EQ(f.value.sign, false);
    ASSERT_EQ(f.value.xs.as_u64(), 0b1);
    ASSERT_EQ(f.value.zs.as_u64(), 0b1);

    f = FlexPauliString::from_text("+Z");
    ASSERT_EQ(f.value.num_qubits, 1);
    ASSERT_EQ(f.imag, false);
    ASSERT_EQ(f.value.sign, false);
    ASSERT_EQ(f.value.xs.as_u64(), 0b0);
    ASSERT_EQ(f.value.zs.as_u64(), 0b1);

    f = FlexPauliString::from_text("X8");
    ASSERT_EQ(f.value.num_qubits, 9);
    ASSERT_EQ(f.imag, false);
    ASSERT_EQ(f.value.sign, false);
    ASSERT_EQ(f.value.xs.as_u64(), 0b100000000);
    ASSERT_EQ(f.value.zs.as_u64(), 0b000000000);

    f = FlexPauliString::from_text("X8*Y2");
    ASSERT_EQ(f.value.num_qubits, 9);
    ASSERT_EQ(f.imag, false);
    ASSERT_EQ(f.value.sign, false);
    ASSERT_EQ(f.value.xs.as_u64(), 0b100000100);
    ASSERT_EQ(f.value.zs.as_u64(), 0b000000100);

    f = FlexPauliString::from_text("X8*Y2*X8");
    ASSERT_EQ(f.value.num_qubits, 9);
    ASSERT_EQ(f.imag, false);
    ASSERT_EQ(f.value.sign, false);
    ASSERT_EQ(f.value.xs.as_u64(), 0b000000100);
    ASSERT_EQ(f.value.zs.as_u64(), 0b000000100);

    f = FlexPauliString::from_text("X8*Y2*Y8");
    ASSERT_EQ(f.value.num_qubits, 9);
    ASSERT_EQ(f.imag, true);
    ASSERT_EQ(f.value.sign, false);
    ASSERT_EQ(f.value.xs.as_u64(), 0b000000100);
    ASSERT_EQ(f.value.zs.as_u64(), 0b100000100);

    f = FlexPauliString::from_text("Y8*Y2*X8");
    ASSERT_EQ(f.value.num_qubits, 9);
    ASSERT_EQ(f.imag, true);
    ASSERT_EQ(f.value.sign, true);
    ASSERT_EQ(f.value.xs.as_u64(), 0b000000100);
    ASSERT_EQ(f.value.zs.as_u64(), 0b100000100);

    ASSERT_EQ(FlexPauliString::from_text("X1"), FlexPauliString::from_text("_X"));

    ASSERT_EQ(FlexPauliString::from_text("X20*I21"), FlexPauliString::from_text("____________________X_"));
}
