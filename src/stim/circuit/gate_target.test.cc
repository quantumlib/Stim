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

#include "stim/test_util.test.h"

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
