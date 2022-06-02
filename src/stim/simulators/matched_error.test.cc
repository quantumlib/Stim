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

#include "gtest/gtest.h"

#include "stim/simulators/error_matcher.h"

using namespace stim;

TEST(matched_error, FlippedMeasurement_basics) {
}
TEST(matched_error, DemTargetWithCoords_basics) {
    DemTargetWithCoords c{DemTarget::relative_detector_id(5), {1, 2, 3}};
    DemTargetWithCoords c2{DemTarget::relative_detector_id(5)};
    ASSERT_EQ(c.str(), "D5[coords 1,2,3]");
    ASSERT_EQ(c2.str(), "D5");

    ASSERT_TRUE(c2 == (DemTargetWithCoords{DemTarget::relative_detector_id(5), {}}));
    ASSERT_FALSE(c2 != (DemTargetWithCoords{DemTarget::relative_detector_id(5), {}}));
    ASSERT_TRUE(c != c2);
    ASSERT_FALSE(c == c2);

    ASSERT_NE(c, c2);
    ASSERT_EQ(c, (DemTargetWithCoords{DemTarget::relative_detector_id(5), {1, 2, 3}}));
    ASSERT_NE(c, (DemTargetWithCoords{DemTarget::relative_detector_id(5), {1, 2, 4}}));
    ASSERT_NE(c, (DemTargetWithCoords{DemTarget::relative_detector_id(6), {1, 2, 3}}));
}
TEST(matched_error, GateTargetWithCoords_basics) {
    GateTargetWithCoords c{GateTarget::qubit(5), {1, 2, 3}};
    GateTargetWithCoords c2{GateTarget::qubit(5)};
    GateTargetWithCoords c3{GateTarget::x(5), {1, 2, 3}};
    ASSERT_EQ(c.str(), "5[coords 1,2,3]");
    ASSERT_EQ(c2.str(), "5");
    ASSERT_EQ(c3.str(), "X5[coords 1,2,3]");

    ASSERT_TRUE(c2 == (GateTargetWithCoords{GateTarget::qubit(5), {}}));
    ASSERT_FALSE(c2 != (GateTargetWithCoords{GateTarget::qubit(5), {}}));
    ASSERT_TRUE(c != c2);
    ASSERT_FALSE(c == c2);

    ASSERT_NE(c, c2);
    ASSERT_EQ(c, (GateTargetWithCoords{GateTarget::qubit(5), {1, 2, 3}}));
    ASSERT_NE(c, (GateTargetWithCoords{GateTarget::qubit(5), {1, 2, 4}}));
    ASSERT_NE(c, (GateTargetWithCoords{GateTarget::qubit(6), {1, 2, 3}}));
}
TEST(matched_error, CircuitErrorLocationStackFrame_basics) {
    CircuitErrorLocationStackFrame c{1, 2, 3};
    ASSERT_EQ(
        c.str(),
        "CircuitErrorLocationStackFrame{"
        "instruction_offset=1, "
        "iteration_index=2, "
        "instruction_repetitions_arg=3}");

    ASSERT_TRUE(c == (CircuitErrorLocationStackFrame{1, 2, 3}));
    ASSERT_FALSE(c != (CircuitErrorLocationStackFrame{1, 2, 3}));
    ASSERT_TRUE(c != (CircuitErrorLocationStackFrame{2, 2, 3}));
    ASSERT_FALSE(c == (CircuitErrorLocationStackFrame{2, 2, 3}));

    ASSERT_EQ(c, (CircuitErrorLocationStackFrame{1, 2, 3}));
    ASSERT_NE(c, (CircuitErrorLocationStackFrame{9, 2, 3}));
    ASSERT_NE(c, (CircuitErrorLocationStackFrame{1, 9, 3}));
    ASSERT_NE(c, (CircuitErrorLocationStackFrame{1, 2, 9}));
}

TEST(matched_error, CircuitTargetsInsideInstruction_basics) {
    CircuitTargetsInsideInstruction targets{
        &GATE_DATA.at("X_ERROR"),
        {0.125},
        11,
        17,
        {
            {GateTarget::qubit(5), {1, 2, 3}},
            {GateTarget::x(6), {}},
            {GateTarget::combiner(), {}},
            {GateTarget::y(9), {3, 4}},
            {GateTarget::rec(-5), {}},
        },
    };
    ASSERT_EQ(targets.str(), "X_ERROR(0.125) 5[coords 1,2,3] X6*Y9[coords 3,4] rec[-5]");
    ASSERT_TRUE(targets == targets);
    ASSERT_FALSE(targets != targets);
    auto targets2 = targets;
    targets2.target_range_start++;
    ASSERT_TRUE(targets != targets2);
    ASSERT_FALSE(targets == targets2);
}

TEST(matched_error, CircuitTargetsInsideInstruction_fill) {
    CircuitTargetsInsideInstruction not_filled{
        &GATE_DATA.at("X_ERROR"),
        {0.125},
        2,
        5,
        {},
    };
    not_filled.fill_args_and_targets_in_range(
        Circuit("X_ERROR(0.125) 0 1 2 3 4 5 6 7 8 9").operations[0].target_data, {{4, {11, 13}}});
    ASSERT_EQ(not_filled.str(), "X_ERROR(0.125) 2 3 4[coords 11,13]");
}

TEST(matched_error, CircuitErrorLocation_basics) {
    CircuitErrorLocation loc{
        6,
        {
            {GateTarget::x(3), {11, 12}},
            {GateTarget::z(5), {}},
        },
        FlippedMeasurement{
            5,
            {
                {GateTarget::x(3), {}},
                {GateTarget::y(4), {14, 15}},
            },
        },
        CircuitTargetsInsideInstruction{
            &GATE_DATA.at("X_ERROR"),
            {0.125},
            11,
            17,
            {
                {GateTarget::qubit(5), {1, 2, 3}},
                {GateTarget::x(6), {}},
                {GateTarget::combiner(), {}},
                {GateTarget::y(9), {3, 4}},
                {GateTarget::rec(-5), {}},
            },
        },
        {
            CircuitErrorLocationStackFrame{9, 0, 100},
            CircuitErrorLocationStackFrame{13, 15, 0},
        },
    };
    ASSERT_EQ(loc.str(), R"RESULT(CircuitErrorLocation {
    flipped_pauli_product: X3[coords 11,12]*Z5
    flipped_measurement.measurement_record_index: 5
    flipped_measurement.measured_observable: X3*Y4[coords 14,15]
    Circuit location stack trace:
        (after 6 TICKs)
        at instruction #10 (a REPEAT 100 block) in the circuit
        after 15 completed iterations
        at instruction #14 (X_ERROR) in the REPEAT block
        at targets #12 to #17 of the instruction
        resolving to X_ERROR(0.125) 5[coords 1,2,3] X6*Y9[coords 3,4] rec[-5]
})RESULT");
    ASSERT_TRUE(loc == loc);
    ASSERT_FALSE(loc != loc);
    auto loc2 = loc;
    loc2.tick_offset++;
    ASSERT_TRUE(loc != loc2);
    ASSERT_FALSE(loc == loc2);
}

TEST(matched_error, MatchedError_basics) {
    CircuitErrorLocation loc{
        6,
        {
            {GateTarget::x(3), {11, 12}},
            {GateTarget::z(5), {}},
        },
        FlippedMeasurement{
            5,
            {
                {GateTarget::x(3), {}},
                {GateTarget::y(4), {14, 15}},
            },
        },
        CircuitTargetsInsideInstruction{
            &GATE_DATA.at("X_ERROR"),
            {0.125},
            11,
            17,
            {
                {GateTarget::qubit(5), {1, 2, 3}},
                {GateTarget::x(6), {}},
                {GateTarget::combiner(), {}},
                {GateTarget::y(9), {3, 4}},
                {GateTarget::rec(-5), {}},
            },
        },
        {
            CircuitErrorLocationStackFrame{9, 0, 100},
            CircuitErrorLocationStackFrame{13, 15, 0},
        },
    };
    CircuitErrorLocation loc2 = loc;
    loc2.tick_offset++;

    ExplainedError err{
        {
            {DemTarget::relative_detector_id(5), {1, 2}},
            {DemTarget::observable_id(5), {}},
        },
        {loc, loc2},
    };
    ExplainedError err2{
        {
            {DemTarget::relative_detector_id(5), {1, 2}},
        },
        {loc2, loc},
    };
    ASSERT_TRUE(err == err);
    ASSERT_FALSE(err != err);
    ASSERT_TRUE(err != err2);
    ASSERT_FALSE(err == err2);
    ASSERT_EQ(err.str(), R"RESULT(ExplainedError {
    dem_error_terms: D5[coords 1,2] L5
    CircuitErrorLocation {
        flipped_pauli_product: X3[coords 11,12]*Z5
        flipped_measurement.measurement_record_index: 5
        flipped_measurement.measured_observable: X3*Y4[coords 14,15]
        Circuit location stack trace:
            (after 6 TICKs)
            at instruction #10 (a REPEAT 100 block) in the circuit
            after 15 completed iterations
            at instruction #14 (X_ERROR) in the REPEAT block
            at targets #12 to #17 of the instruction
            resolving to X_ERROR(0.125) 5[coords 1,2,3] X6*Y9[coords 3,4] rec[-5]
    }
    CircuitErrorLocation {
        flipped_pauli_product: X3[coords 11,12]*Z5
        flipped_measurement.measurement_record_index: 5
        flipped_measurement.measured_observable: X3*Y4[coords 14,15]
        Circuit location stack trace:
            (after 7 TICKs)
            at instruction #10 (a REPEAT 100 block) in the circuit
            after 15 completed iterations
            at instruction #14 (X_ERROR) in the REPEAT block
            at targets #12 to #17 of the instruction
            resolving to X_ERROR(0.125) 5[coords 1,2,3] X6*Y9[coords 3,4] rec[-5]
    }
})RESULT");
}

TEST(matched_error, MatchedError_fill) {
    ExplainedError err{{}, {}};
    err.fill_in_dem_targets(
        std::vector<DemTarget>{DemTarget::relative_detector_id(5), DemTarget::relative_detector_id(6)},
        {{5, {11, 13}}});
    ASSERT_EQ(err.str(), R"RESULT(ExplainedError {
    dem_error_terms: D5[coords 11,13] D6
    [no single circuit error had these exact symptoms]
})RESULT");
}
