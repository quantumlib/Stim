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

#include "stim/circuit/gate_decomposition.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/cmd/command_help.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST(gate_decomposition, decompose_mpp_operation) {
    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &inst) {
        out.safe_append(inst);
        out.append_from_text("TICK");
    };
    decompose_mpp_operation(
        Circuit("MPP(0.125) X0*X1*X2 Z3*Z4*Z5 X2*Y4 Z3 Z3 Z4*Z5").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        H 0 1 2
        TICK
        CX 1 0 2 0 4 3 5 3
        TICK
        M(0.125) 0 3
        TICK
        CX 1 0 2 0 4 3 5 3
        TICK
        H 0 1 2
        TICK

        H 2
        TICK
        H_YZ 4
        TICK
        CX 4 2
        TICK
        M(0.125) 2 3
        TICK
        CX 4 2
        TICK
        H_YZ 4
        TICK
        H 2
        TICK

        CX 5 4
        TICK
        M(0.125) 3 4
        TICK
        CX 5 4
        TICK
    )CIRCUIT"));

    out.clear();
    decompose_mpp_operation(Circuit("MPP X0*Z1*Y2 X3*X4 Y0*Y1*Y2*Y3*Y4").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        H 0 3 4
        TICK
        H_YZ 2
        TICK
        CX 1 0 2 0 4 3
        TICK
        M 0 3
        TICK
        CX 1 0 2 0 4 3
        TICK
        H_YZ 2
        TICK
        H 0 3 4
        TICK

        H_YZ 0 1 2 3 4
        TICK
        CX 1 0 2 0 3 0 4 0
        TICK
        M 0
        TICK
        CX 1 0 2 0 3 0 4 0
        TICK
        H_YZ 0 1 2 3 4
        TICK
    )CIRCUIT"));
}

TEST(gate_decomposition, decompose_mpp_to_mpad) {
    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &inst) {
        out.safe_append(inst);
        out.append_from_text("TICK");
    };
    decompose_mpp_operation(
        Circuit(R"CIRCUIT(
            MPP(0.125) X0*X0 X0*!X0 X0*Y0*Z0*X1*Y1*Z1
        )CIRCUIT")
            .operations[0],
        10,
        append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        MPAD(0.125) 0
        TICK
        MPAD(0.125) 1
        TICK
        MPAD(0.125) 1
        TICK
    )CIRCUIT"));

    ASSERT_THROW(
        { decompose_mpp_operation(Circuit("MPP(0.125) X0*Y0*Z0").operations[0], 10, append_into_circuit); },
        std::invalid_argument);
}

TEST(gate_decomposition, decompose_pair_instruction_into_segments_with_single_use_controls) {
    Circuit out;
    auto append_into_circuit = [&](const CircuitInstruction &segment) {
        std::vector<GateTarget> evens;
        for (size_t k = 0; k < segment.targets.size(); k += 2) {
            evens.push_back(segment.targets[k]);
        }
        out.safe_append(CircuitInstruction{GateType::CX, {}, segment.targets});
        out.safe_append(CircuitInstruction{GateType::MX, segment.args, evens});
        out.safe_append(CircuitInstruction{GateType::CX, {}, segment.targets});
        out.append_from_text("TICK");
    };
    decompose_pair_instruction_into_segments_with_single_use_controls(
        Circuit("MXX(0.125) 0 1 0 2 3 5 4 5 3 4").operations[0], 10, append_into_circuit);
    ASSERT_EQ(out, Circuit(R"CIRCUIT(
        CX 0 1
        MX(0.125) 0
        CX 0 1
        TICK

        CX 0 2 3 5 4 5
        MX(0.125) 0 3 4
        CX 0 2 3 5 4 5
        TICK

        CX 3 4
        MX(0.125) 3
        CX 3 4
        TICK
    )CIRCUIT"));
}

static std::pair<std::vector<PauliString<64>>, std::vector<PauliString<64>>> circuit_output_eq_val(
    const Circuit &circuit) {
    // CAUTION: this is not 100% reliable when measurement count is larger than 1.
    TableauSimulator<64> sim1(INDEPENDENT_TEST_RNG(), circuit.count_qubits(), -1);
    TableauSimulator<64> sim2(INDEPENDENT_TEST_RNG(), circuit.count_qubits(), +1);
    sim1.safe_do_circuit(circuit);
    sim2.safe_do_circuit(circuit);
    return {sim1.canonical_stabilizers(), sim2.canonical_stabilizers()};
}

bool is_simplification_correct(const Gate &gate) {
    std::vector<double> args;
    while (args.size() < gate.arg_count && gate.arg_count != ARG_COUNT_SYGIL_ANY &&
           gate.arg_count != ARG_COUNT_SYGIL_ZERO_OR_ONE) {
        args.push_back(args.empty() ? 1 : 0);
    }

    Circuit original;
    original.safe_append(gate.id, gate_decomposition_help_targets_for_gate_type(gate.id), args);
    Circuit simplified = simplified_circuit(original);

    if (gate.h_s_cx_m_r_decomposition == nullptr) {
        return simplified == original;
    }

    uint32_t n = original.count_qubits();

    Circuit epr;
    for (uint32_t q = 0; q < n; q++) {
        epr.safe_append_u("H", {q});
    }
    for (uint32_t q = 0; q < n; q++) {
        epr.safe_append_u("CNOT", {q, q + n});
    }

    Circuit circuit1 = epr + original;
    Circuit circuit2 = epr + simplified;

    // Reset gates make the ancillary qubits irrelevant because the final value is unrelated to the initial value.
    // So, for reset gates, discard the ancillary qubits.
    // CAUTION: this could give false positives if "partial reset" gates are added in the future.
    //          (E.g. a two qubit gate that resets only one of the qubits.)
    if ((gate.flags & GATE_IS_RESET) && !(gate.flags & GATE_PRODUCES_RESULTS)) {
        for (uint32_t q = 0; q < n; q++) {
            circuit1.safe_append_u("R", {q + n});
            circuit2.safe_append_u("R", {q + n});
        }
    }

    // Verify decomposed all the way to base gate set, if the gate has a decomposition.
    for (const auto &op : circuit2.operations) {
        if (op.gate_type != GateType::CX && op.gate_type != GateType::H && op.gate_type != GateType::S &&
            op.gate_type != GateType::M && op.gate_type != GateType::R) {
            return false;
        }
    }

    auto v1 = circuit_output_eq_val(circuit1);
    auto v2 = circuit_output_eq_val(circuit2);
    return v1 == v2;
}

TEST(gate_decomposition, simplifications_are_correct) {
    for (const auto &g : GATE_DATA.items) {
        if (g.id != GateType::NOT_A_GATE && g.id != GateType::REPEAT) {
            EXPECT_TRUE(is_simplification_correct(g)) << g.name;
        }
    }
}
