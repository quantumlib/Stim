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

#include "stim/simulators/error_matcher.h"

#include "gtest/gtest.h"

#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"

using namespace stim;

TEST(ErrorMatcher, X_ERROR) {
    auto actual = ErrorMatcher::explain_errors_from_circuit(
        Circuit(R"CIRCUIT(
            QUBIT_COORDS(5, 6) 0
            X_ERROR(0.25) 0
            M 0
            DETECTOR(2, 3) rec[-1]
        )CIRCUIT"),
        nullptr,
        false);
    std::vector<ExplainedError> expected{
        ExplainedError{
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
            }},
    };
    ASSERT_EQ(actual, expected);
    ASSERT_EQ(actual.size(), 1);
    ASSERT_EQ(actual[0].str(), R"RESULT(ExplainedError {
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

TEST(ErrorMatcher, CORRELATED_ERROR) {
    auto actual = ErrorMatcher::explain_errors_from_circuit(
        Circuit(R"CIRCUIT(
            SHIFT_COORDS(10, 20)
            QUBIT_COORDS(5, 6) 0
            SHIFT_COORDS(100, 200)
            CORRELATED_ERROR(0.25) X0 Y1
            M 0
            DETECTOR(2, 3) rec[-1]
        )CIRCUIT"),
        nullptr,
        false);
    ASSERT_EQ(actual.size(), 1);
    ASSERT_EQ(actual[0].str(), R"RESULT(ExplainedError {
    dem_error_terms: D0[coords 112,223]
    CircuitErrorLocation {
        flipped_pauli_product: X0[coords 15,26]*Y1
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #4 (E) in the circuit
            at targets #1 to #2 of the instruction
            resolving to E(0.25) X0[coords 15,26] Y1
    }
})RESULT");
}

TEST(ErrorMatcher, MX_ERROR) {
    auto actual = ErrorMatcher::explain_errors_from_circuit(
        Circuit(R"CIRCUIT(
            QUBIT_COORDS(5, 6) 0
            RX 0
            REPEAT 10 {
                TICK
            }
            MX(0.125) 1 2 3 0 4
            DETECTOR(2, 3) rec[-2]
        )CIRCUIT"),
        nullptr,
        false);
    ASSERT_EQ(actual.size(), 1);
    ASSERT_EQ(actual[0].str(), R"RESULT(ExplainedError {
    dem_error_terms: D0[coords 2,3]
    CircuitErrorLocation {
        flipped_measurement.measurement_record_index: 3
        flipped_measurement.measured_observable: X0[coords 5,6]
        Circuit location stack trace:
            (after 10 TICKs)
            at instruction #4 (MX) in the circuit
            at target #4 of the instruction
            resolving to MX(0.125) 0[coords 5,6]
    }
})RESULT");
}

TEST(ErrorMatcher, MPP_ERROR) {
    auto actual = ErrorMatcher::explain_errors_from_circuit(
        Circuit(R"CIRCUIT(
            QUBIT_COORDS(5, 6) 0
            RY 0
            MPP(0.125) Z1 Y0*Z3*Z4 Y5
            DETECTOR(2, 3) rec[-2]
            DETECTOR(5, 6) rec[-2]
        )CIRCUIT"),
        nullptr,
        false);
    ASSERT_EQ(actual.size(), 1);
    ASSERT_EQ(actual[0].str(), R"RESULT(ExplainedError {
    dem_error_terms: D0[coords 2,3] D1[coords 5,6]
    CircuitErrorLocation {
        flipped_measurement.measurement_record_index: 1
        flipped_measurement.measured_observable: Y0[coords 5,6]*Z3*Z4
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #3 (MPP) in the circuit
            at targets #2 to #6 of the instruction
            resolving to MPP(0.125) Y0[coords 5,6]*Z3*Z4
    }
})RESULT");
}

TEST(ErrorMatcher, repetition_code_data_depolarization) {
    CircuitGenParameters params(2, 3, "memory");
    params.before_round_data_depolarization = 0.001;
    auto circuit = generate_rep_code_circuit(params).circuit;

    auto actual = ErrorMatcher::explain_errors_from_circuit(circuit, nullptr, false);
    std::stringstream ss;
    for (const auto &match : actual) {
        ss << "\n" << match << "\n";
    }
    ASSERT_EQ(ss.str(), R"RESULT(
ExplainedError {
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

ExplainedError {
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

ExplainedError {
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

ExplainedError {
    dem_error_terms: D2[coords 1,1]
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

ExplainedError {
    dem_error_terms: D2[coords 1,1] D3[coords 3,1]
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

ExplainedError {
    dem_error_terms: D3[coords 3,1] L0
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

TEST(ErrorMatcher, repetition_code_data_depolarization_single_results) {
    CircuitGenParameters params(2, 3, "memory");
    params.before_round_data_depolarization = 0.001;
    auto circuit = generate_rep_code_circuit(params).circuit;

    auto actual = ErrorMatcher::explain_errors_from_circuit(circuit, nullptr, true);
    std::stringstream ss;
    for (const auto &match : actual) {
        ss << "\n" << match << "\n";
    }
    ASSERT_EQ(ss.str(), R"RESULT(
ExplainedError {
    dem_error_terms: D0[coords 1,0]
    CircuitErrorLocation {
        flipped_pauli_product: X0
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.001) 0
    }
}

ExplainedError {
    dem_error_terms: D0[coords 1,0] D1[coords 3,0]
    CircuitErrorLocation {
        flipped_pauli_product: X2
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #2 of the instruction
            resolving to DEPOLARIZE1(0.001) 2
    }
}

ExplainedError {
    dem_error_terms: D1[coords 3,0] L0
    CircuitErrorLocation {
        flipped_pauli_product: X4
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #3 of the instruction
            resolving to DEPOLARIZE1(0.001) 4
    }
}

ExplainedError {
    dem_error_terms: D2[coords 1,1]
    CircuitErrorLocation {
        flipped_pauli_product: X0
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.001) 0
    }
}

ExplainedError {
    dem_error_terms: D2[coords 1,1] D3[coords 3,1]
    CircuitErrorLocation {
        flipped_pauli_product: X2
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #2 of the instruction
            resolving to DEPOLARIZE1(0.001) 2
    }
}

ExplainedError {
    dem_error_terms: D3[coords 3,1] L0
    CircuitErrorLocation {
        flipped_pauli_product: X4
        Circuit location stack trace:
            (after 4 TICKs)
            at instruction #12 (DEPOLARIZE1) in the circuit
            at target #3 of the instruction
            resolving to DEPOLARIZE1(0.001) 4
    }
}
)RESULT");
}

TEST(ErrorMatcher, surface_code_filter) {
    CircuitGenParameters params(5, 5, "rotated_memory_z");
    params.before_round_data_depolarization = 0.001;
    params.after_clifford_depolarization = 0.001;
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    DetectorErrorModel filter(R"MODEL(
        error(1) D97 D98 D102 D103
)MODEL");

    auto actual = ErrorMatcher::explain_errors_from_circuit(circuit, &filter, false);
    std::stringstream ss;
    for (const auto &match : actual) {
        ss << "\n" << match << "\n";
    }
    ASSERT_EQ(ss.str(), R"RESULT(
ExplainedError {
    dem_error_terms: D97[coords 4,6,4] D98[coords 6,6,4] D102[coords 2,8,4] D103[coords 4,8,4]
    CircuitErrorLocation {
        flipped_pauli_product: Y37[coords 4,6]*Y36[coords 3,7]
        Circuit location stack trace:
            (after 31 TICKs)
            at instruction #89 (a REPEAT 4 block) in the circuit
            after 3 completed iterations
            at instruction #10 (DEPOLARIZE2) in the REPEAT block
            at targets #9 to #10 of the instruction
            resolving to DEPOLARIZE2(0.001) 37[coords 4,6] 36[coords 3,7]
    }
}
)RESULT");
}
