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
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"

using namespace stim;

TEST(ErrorMatcher, X_ERROR) {
    auto actual = ErrorMatcher::match_errors_from_circuit(
        Circuit(R"CIRCUIT(
            QUBIT_COORDS(5, 6) 0
            X_ERROR(0.25) 0
            M 0
            DETECTOR(2, 3) rec[-1]
        )CIRCUIT"),
        DetectorErrorModel());
    std::vector<MatchedError> expected{
        MatchedError{
            {
                {DemTarget::relative_detector_id(0), {2, 3}},
            },
            {
                CircuitErrorLocation{
                    0,
                    {
                        {GateTarget::x(0), {5, 6}},
                    },
                    FlippedMeasurement{UINT64_MAX, {}},
                    CircuitTargetsInsideInstruction{
                        &GATE_DATA.at("X_ERROR"),
                        {0.25},
                        0,
                        1,
                        {
                            {GateTarget::qubit(0), {5, 6}},
                        },
                    },
                    {
                        CircuitErrorLocationStackFrame{1, 0, 0},
                    },
                },
            }
        },
    };
    ASSERT_EQ(actual, expected);
    ASSERT_EQ(actual.size(), 1);
    ASSERT_EQ(actual[0].str(), R"RESULT(MatchedError {
    dem_error_terms: D0[coords 2,3]
    CircuitErrorLocation {
        flipped_pauli_product: X0[coords 5,6]
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #2 (X_ERROR) in the circuit
            at target #1 of the instruction
            resolving to X_ERROR(0.25) 0[coords 5,6]
    }
})RESULT");
}

TEST(ErrorMatcher, repetition_code) {
    CircuitGenParameters params(2, 3, "rotated_memory_z");
    params.before_round_data_depolarization = 0.001;
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;

    auto actual = ErrorMatcher::match_errors_from_circuit(circuit, DetectorErrorModel());
    std::stringstream ss;
    for (const auto &match : actual) {
        ss << "\n" << match << "\n";
    }
    std::cerr << ss.str() << "\n";
    ASSERT_EQ(ss.str(), R"RESULT(
MatchedError {
    dem_error_terms: D0[coords 1,0]
    CircuitErrorLocation {
        flipped_pauli_product: X0
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.001) 0
    }
    CircuitErrorLocation {
        flipped_pauli_product: Y0
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.001) 0
    }
}

MatchedError {
    dem_error_terms: D0[coords 1,0] D1[coords 3,0]
    CircuitErrorLocation {
        flipped_pauli_product: X2
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #2 of the instruction
            resolving to DEPOLARIZE1(0.001) 2
    }
    CircuitErrorLocation {
        flipped_pauli_product: Y2
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #2 of the instruction
            resolving to DEPOLARIZE1(0.001) 2
    }
}

MatchedError {
    dem_error_terms: D1[coords 3,0] L0
    CircuitErrorLocation {
        flipped_pauli_product: X4
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #3 of the instruction
            resolving to DEPOLARIZE1(0.001) 4
    }
    CircuitErrorLocation {
        flipped_pauli_product: Y4
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #3 of the instruction
            resolving to DEPOLARIZE1(0.001) 4
    }
}

MatchedError {
    dem_error_terms: D2[coords 1,0]
    CircuitErrorLocation {
        flipped_pauli_product: X0
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.001) 0
    }
    CircuitErrorLocation {
        flipped_pauli_product: Y0
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.001) 0
    }
}

MatchedError {
    dem_error_terms: D2[coords 1,0] D3[coords 3,0]
    CircuitErrorLocation {
        flipped_pauli_product: X2
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #2 of the instruction
            resolving to DEPOLARIZE1(0.001) 2
    }
    CircuitErrorLocation {
        flipped_pauli_product: Y2
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #2 of the instruction
            resolving to DEPOLARIZE1(0.001) 2
    }
}

MatchedError {
    dem_error_terms: D3[coords 3,0] L0
    CircuitErrorLocation {
        flipped_pauli_product: X4
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #3 of the instruction
            resolving to DEPOLARIZE1(0.001) 4
    }
    CircuitErrorLocation {
        flipped_pauli_product: Y4
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #3 of the instruction
            resolving to DEPOLARIZE1(0.001) 4
    }
}
)RESULT");
}
