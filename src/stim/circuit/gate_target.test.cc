// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/circuit/gate_target.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(gate_target, xyz) {
    ASSERT_THROW({ GateTarget::x(UINT32_MAX); }, std::invalid_argument);
    ASSERT_THROW({ GateTarget::y(UINT32_MAX, true); }, std::invalid_argument);
    ASSERT_THROW({ GateTarget::z(UINT32_MAX, false); }, std::invalid_argument);

    auto t = GateTarget::x(5, false);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), false);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), true);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "stim.target_x(5)");
    ASSERT_EQ(t.value(), 5);
    ASSERT_TRUE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());

    t = GateTarget::x(7, true);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), true);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), true);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "stim.target_x(7, invert=True)");
    ASSERT_EQ(t.value(), 7);
    ASSERT_TRUE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());

    t = GateTarget::y(11, false);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), false);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), true);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "stim.target_y(11)");
    ASSERT_EQ(t.value(), 11);
    ASSERT_TRUE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());

    t = GateTarget::y(13, true);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), true);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), true);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "stim.target_y(13, invert=True)");
    ASSERT_EQ(t.value(), 13);
    ASSERT_FALSE(t.is_sweep_bit_target());

    t = GateTarget::z(17, false);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), false);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), true);
    ASSERT_EQ(t.str(), "stim.target_z(17)");
    ASSERT_EQ(t.value(), 17);
    ASSERT_TRUE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());

    t = GateTarget::z(19, true);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), true);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), true);
    ASSERT_EQ(t.str(), "stim.target_z(19, invert=True)");
    ASSERT_EQ(t.value(), 19);
    ASSERT_EQ(t.qubit_value(), 19);
    ASSERT_TRUE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());
}

TEST(gate_target, qubit) {
    ASSERT_THROW({ GateTarget::qubit(UINT32_MAX); }, std::invalid_argument);

    auto t = GateTarget::qubit(5, false);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), false);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), true);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "5");
    ASSERT_EQ(t.value(), 5);
    ASSERT_EQ(t.qubit_value(), 5);
    ASSERT_TRUE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());

    t = GateTarget::qubit(7, true);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), true);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), true);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "stim.target_inv(7)");
    ASSERT_EQ(t.value(), 7);
    ASSERT_TRUE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());
}

TEST(gate_target, record) {
    ASSERT_THROW({ GateTarget::rec(1); }, std::out_of_range);
    ASSERT_THROW({ GateTarget::rec(0); }, std::out_of_range);
    ASSERT_THROW({ GateTarget::rec(-(int32_t{1} << 30)); }, std::out_of_range);

    auto t = GateTarget::rec(-5);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), false);
    ASSERT_EQ(t.is_measurement_record_target(), true);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "stim.target_rec(-5)");
    ASSERT_EQ(t.value(), -5);
    ASSERT_EQ(t.qubit_value(), 5);
    ASSERT_FALSE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());
}

TEST(gate_target, sweep) {
    auto t = GateTarget::sweep_bit(5);
    ASSERT_EQ(t.is_combiner(), false);
    ASSERT_EQ(t.is_inverted_result_target(), false);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "stim.target_sweep_bit(5)");
    ASSERT_EQ(t.value(), 5);
    ASSERT_FALSE(t.has_qubit_value());
    ASSERT_TRUE(t.is_sweep_bit_target());
}

TEST(gate_target, combiner) {
    auto t = GateTarget::combiner();
    ASSERT_EQ(t.is_combiner(), true);
    ASSERT_EQ(t.is_inverted_result_target(), false);
    ASSERT_EQ(t.is_measurement_record_target(), false);
    ASSERT_EQ(t.is_qubit_target(), false);
    ASSERT_EQ(t.is_x_target(), false);
    ASSERT_EQ(t.is_y_target(), false);
    ASSERT_EQ(t.is_z_target(), false);
    ASSERT_EQ(t.str(), "stim.GateTarget.combiner()");
    ASSERT_EQ(t.qubit_value(), 0);
    ASSERT_FALSE(t.has_qubit_value());
    ASSERT_FALSE(t.is_sweep_bit_target());
}

TEST(gate_target, equality) {
    ASSERT_TRUE(GateTarget{0} == GateTarget{0});
    ASSERT_FALSE(GateTarget{0} == GateTarget{1});
    ASSERT_TRUE(GateTarget{0} != GateTarget{1});
    ASSERT_FALSE(GateTarget{0} != GateTarget{0});
    ASSERT_TRUE(GateTarget{0} < GateTarget{1});
    ASSERT_FALSE(GateTarget{1} < GateTarget{0});
    ASSERT_FALSE(GateTarget{0} < GateTarget{0});
}

TEST(gate_target, inverse) {
    ASSERT_EQ(!GateTarget::qubit(5), GateTarget::qubit(5, true));
    ASSERT_EQ(GateTarget::qubit(5), !GateTarget::qubit(5, true));
    ASSERT_EQ(!GateTarget::x(5), GateTarget::x(5, true));
    ASSERT_EQ(GateTarget::x(5), !GateTarget::x(5, true));
    ASSERT_EQ(!GateTarget::y(5), GateTarget::y(5, true));
    ASSERT_EQ(GateTarget::y(5), !GateTarget::y(5, true));
    ASSERT_EQ(!GateTarget::z(9), GateTarget::z(9, true));
    ASSERT_EQ(GateTarget::z(7), !GateTarget::z(7, true));
    ASSERT_EQ(!!GateTarget::z(9), GateTarget::z(9));
    ASSERT_THROW({ !GateTarget::combiner(); }, std::invalid_argument);
    ASSERT_THROW({ !GateTarget::rec(-3); }, std::invalid_argument);
    ASSERT_THROW({ !GateTarget::sweep_bit(6); }, std::invalid_argument);
}

TEST(gate_target, pauli) {
    ASSERT_EQ(GateTarget::combiner().pauli_type(), 'I');
    ASSERT_EQ(GateTarget::sweep_bit(5).pauli_type(), 'I');
    ASSERT_EQ(GateTarget::rec(-5).pauli_type(), 'I');
    ASSERT_EQ(GateTarget::qubit(5).pauli_type(), 'I');
    ASSERT_EQ(GateTarget::qubit(6, true).pauli_type(), 'I');

    ASSERT_EQ(GateTarget::x(7).pauli_type(), 'X');
    ASSERT_EQ(GateTarget::x(7, true).pauli_type(), 'X');
    ASSERT_EQ(GateTarget::y(7).pauli_type(), 'Y');
    ASSERT_EQ(GateTarget::y(7, true).pauli_type(), 'Y');
    ASSERT_EQ(GateTarget::z(7).pauli_type(), 'Z');
    ASSERT_EQ(GateTarget::z(7, true).pauli_type(), 'Z');
}

TEST(gate_target, from_target_str) {
    ASSERT_EQ(GateTarget::from_target_str("5"), GateTarget::qubit(5));
    ASSERT_EQ(GateTarget::from_target_str("rec[-3]"), GateTarget::rec(-3));
}

TEST(gate_target, target_str_round_trip) {
    std::vector<GateTarget> targets{
        GateTarget::qubit(2),
        GateTarget::qubit(3, true),
        GateTarget::sweep_bit(5),
        GateTarget::rec(-7),
        GateTarget::x(11),
        GateTarget::x(13, true),
        GateTarget::y(17),
        GateTarget::y(19, true),
        GateTarget::z(23),
        GateTarget::z(29, true),
        GateTarget::combiner(),
    };
    for (const auto &t : targets) {
        ASSERT_EQ(GateTarget::from_target_str(t.target_str().c_str()), t) << t;
    }
}

TEST(gate_target, is_pauli_target) {
    ASSERT_FALSE(GateTarget::qubit(2).is_pauli_target());
    ASSERT_FALSE(GateTarget::qubit(3, true).is_pauli_target());
    ASSERT_FALSE(GateTarget::sweep_bit(5).is_pauli_target());
    ASSERT_FALSE(GateTarget::rec(-7).is_pauli_target());
    ASSERT_TRUE(GateTarget::x(11).is_pauli_target());
    ASSERT_TRUE(GateTarget::x(13, true).is_pauli_target());
    ASSERT_TRUE(GateTarget::y(17).is_pauli_target());
    ASSERT_TRUE(GateTarget::y(19, true).is_pauli_target());
    ASSERT_TRUE(GateTarget::z(23).is_pauli_target());
    ASSERT_TRUE(GateTarget::z(29, true).is_pauli_target());
    ASSERT_FALSE(GateTarget::combiner().is_pauli_target());
}

TEST(gate_target, is_classical_bit_target) {
    ASSERT_TRUE(GateTarget::sweep_bit(5).is_classical_bit_target());
    ASSERT_TRUE(GateTarget::rec(-7).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::qubit(2).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::qubit(3, true).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::x(11).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::x(13, true).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::y(17).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::y(19, true).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::z(23).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::z(29, true).is_classical_bit_target());
    ASSERT_FALSE(GateTarget::combiner().is_classical_bit_target());
}
