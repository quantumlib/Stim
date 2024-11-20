#include "stim/util_top/simplified_circuit.h"

#include "gtest/gtest.h"

#include "stim/cmd/command_help.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

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
    original.safe_append(CircuitInstruction(gate.id, args, gate_decomposition_help_targets_for_gate_type(gate.id), ""));
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
