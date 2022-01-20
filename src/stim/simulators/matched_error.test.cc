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

#include <gtest/gtest.h>

#include "stim/simulators/error_matcher.h"

using namespace stim;

TEST(CircuitErrorLocation, str) {
    CircuitErrorLocation loc{
        6,
        {GateTarget::z(3), GateTarget::y(4)},
        FlippedMeasurement{
            5,
            {GateTarget::x(3), GateTarget::y(4)},
        },
        CircuitTargetsInsideInstruction{
            &GATE_DATA.at("X_ERROR"),
            11,
            17,
            {GateTarget::qubit(5), GateTarget::x(6), GateTarget::combiner(), GateTarget::y(9), GateTarget::rec(-5)},
            {{1, 2, 3}, {}, {}, {3, 4}},
            {0.125},
        },
        {
            CircuitErrorLocationStackFrame{9, 0, 100},
            CircuitErrorLocationStackFrame{13, 15, 0},
        },
    };
    ASSERT_EQ(loc.str(), R"RESULT(CircuitErrorLocation {
    flipped_pauli_product: Z3*Y4
    flipped_measurement.measurement_record_index: 5
    flipped_measurement.measured_observable: X3*Y4
    Circuit location stack trace:
        (after 6 TICKs)
        at instruction #10 (a REPEAT 100 block) in the circuit
        after 15 completed iterations
        at instruction #14 (X_ERROR) in the REPEAT block
        at targets #12 to #17 of the instruction
        resolving to X_ERROR(0.125) 5[coords 1,2,3] X6*Y9[coords 3,4] rec[-5]
})RESULT");
}

/*
struct CircuitErrorLocation {
    /// The number of ticks that have been executed by this point.
    uint64_t ticks_beforehand;


    // Stack trace within the circuit and nested loop blocks.
    std::vector<uint64_t> instruction_indices;
    std::vector<uint64_t> iteration_indices;

    std::string str() const;
    bool operator==(const CircuitErrorLocation &other) const;
    bool operator!=(const CircuitErrorLocation &other) const;
};

/// Explains how an error from a detector error model matches error(s) from a circuit.
struct MatchedError {
    /// A sorted list of detector and observable targets flipped by the error.
    /// This list should never contain target separators.
    std::vector<DemTarget> dem_error_terms;

    /// Locations of matching errors in the circuit.
    std::vector<CircuitErrorLocation> circuit_error_locations;

    std::string str() const;
    bool operator==(const MatchedError &other) const;
    bool operator!=(const MatchedError &other) const;
};

std::ostream &operator<<(std::ostream &out, const CircuitErrorLocation &e);
std::ostream &operator<<(std::ostream &out, const MatchedError &e);
 */