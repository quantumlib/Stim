#include "tableau_simulator.js.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "common.js.h"

using namespace stim;

struct JsCircuitInstruction {
    GateType gate_type;
    std::vector<GateTarget> targets;
    JsCircuitInstruction(GateType gate_type, std::vector<GateTarget> targets) : gate_type(gate_type), targets(std::move(targets)) {
    }
    JsCircuitInstruction(GateType gate_type, std::vector<uint32_t> init_targets) : gate_type(gate_type) {
        for (auto e : init_targets) {
            targets.push_back(GateTarget{e});
        }
    }
    operator CircuitInstruction() const {
        return {gate_type, {}, targets};
    }
};

static JsCircuitInstruction args_to_targets(TableauSimulator<MAX_BITWORD_WIDTH> &self, GateType gate_type, const emscripten::val &args) {
    std::vector<uint32_t> result = emscripten::convertJSArrayToNumberVector<uint32_t>(args);
    uint32_t max_q = 0;
    for (uint32_t q : result) {
        max_q = std::max(max_q, q & TARGET_VALUE_MASK);
    }

    // Note: quadratic behavior.
    self.ensure_large_enough_for_qubits((size_t)max_q + 1);

    return JsCircuitInstruction(gate_type, result);
}

static JsCircuitInstruction safe_targets(TableauSimulator<MAX_BITWORD_WIDTH> &self, GateType gate_type, uint32_t target) {
    uint32_t max_q = target & TARGET_VALUE_MASK;
    self.ensure_large_enough_for_qubits((size_t)max_q + 1);
    return JsCircuitInstruction(gate_type, {GateTarget{target}});
}

static JsCircuitInstruction safe_targets(TableauSimulator<MAX_BITWORD_WIDTH> &self, GateType gate_type, uint32_t target1, uint32_t target2) {
    uint32_t max_q = std::max(target1 & TARGET_VALUE_MASK, target2 & TARGET_VALUE_MASK);
    self.ensure_large_enough_for_qubits((size_t)max_q + 1);
    if (target1 == target2) {
        throw std::invalid_argument("target1 == target2");
    }
    return JsCircuitInstruction(gate_type, {GateTarget{target1}, GateTarget{target2}});
}

static JsCircuitInstruction args_to_target_pairs(TableauSimulator<MAX_BITWORD_WIDTH> &self, GateType gate_type, const emscripten::val &args) {
    auto result = args_to_targets(self, gate_type, args);
    if (result.targets.size() & 1) {
        throw std::out_of_range("Two qubit operation requires an even number of targets.");
    }
    return result;
}

ExposedTableauSimulator::ExposedTableauSimulator() : sim(externally_seeded_rng(), 0) {
}

bool ExposedTableauSimulator::measure(size_t target) {
    sim.ensure_large_enough_for_qubits(target + 1);
    sim.do_MZ(JsCircuitInstruction(GateType::M, {GateTarget{target}}));
    return (bool)sim.measurement_record.storage.back();
}

emscripten::val ExposedTableauSimulator::measure_kickback(size_t target) {
    sim.ensure_large_enough_for_qubits(target + 1);
    auto result = sim.measure_kickback_z(GateTarget{(uint32_t)target});
    emscripten::val returned = emscripten::val::object();
    returned.set("result", result.first);
    if (result.second.num_qubits) {
        returned.set("kickback", ExposedPauliString(result.second));
    } else {
        returned.set("kickback", emscripten::val::undefined());
    }
    return returned;
}

void ExposedTableauSimulator::do_circuit(const ExposedCircuit &circuit) {
    sim.ensure_large_enough_for_qubits(circuit.circuit.count_qubits());
    circuit.circuit.for_each_operation([&](const CircuitInstruction &op) {
        sim.do_gate(op);
    });
}
void ExposedTableauSimulator::do_pauli_string(const ExposedPauliString &pauli_string) {
    sim.ensure_large_enough_for_qubits(pauli_string.pauli_string.num_qubits);
    sim.paulis(pauli_string.pauli_string);
}
void ExposedTableauSimulator::do_tableau(const ExposedTableau &tableau, const emscripten::val &targets) {
    auto conv = emscripten::convertJSArrayToNumberVector<size_t>(targets);
    uint32_t max_q = 0;
    for (uint32_t q : conv) {
        max_q = std::max(max_q, q & TARGET_VALUE_MASK);
    }
    sim.ensure_large_enough_for_qubits((size_t)max_q + 1);
    sim.inv_state.inplace_scatter_prepend(tableau.tableau.inverse(), conv);
}

void ExposedTableauSimulator::X(uint32_t target) {
    sim.do_X(safe_targets(sim, GateType::X, target));
}
void ExposedTableauSimulator::Y(uint32_t target) {
    sim.do_Y(safe_targets(sim, GateType::Y, target));
}
void ExposedTableauSimulator::Z(uint32_t target) {
    sim.do_Z(safe_targets(sim, GateType::Z, target));
}
void ExposedTableauSimulator::H(uint32_t target) {
    sim.do_H_XZ(safe_targets(sim, GateType::H, target));
}
void ExposedTableauSimulator::CNOT(uint32_t control, uint32_t target) {
    sim.do_ZCX(safe_targets(sim, GateType::CX, control, target));
}
void ExposedTableauSimulator::SWAP(uint32_t target1, uint32_t target2) {
    sim.do_SWAP(safe_targets(sim, GateType::SWAP, target1, target2));
}
void ExposedTableauSimulator::CY(uint32_t control, uint32_t target) {
    sim.do_ZCY(safe_targets(sim, GateType::CY, control, target));
}
void ExposedTableauSimulator::CZ(uint32_t control, uint32_t target) {
    sim.do_ZCZ(safe_targets(sim, GateType::CZ, control, target));
}

ExposedTableau ExposedTableauSimulator::current_inverse_tableau() const {
    return ExposedTableau(sim.inv_state);
}
void ExposedTableauSimulator::set_inverse_tableau(const ExposedTableau &tableau) {
    sim.inv_state = tableau.tableau;
}

ExposedTableauSimulator ExposedTableauSimulator::copy() const {
    return *this;
}

void emscripten_bind_tableau_simulator() {
    auto c = emscripten::class_<ExposedTableauSimulator>("TableauSimulator");
    c.constructor();
    c.function("current_inverse_tableau", &ExposedTableauSimulator::current_inverse_tableau);
    c.function("set_inverse_tableau", &ExposedTableauSimulator::set_inverse_tableau);
    c.function("measure", &ExposedTableauSimulator::measure);
    c.function("measure_kickback", &ExposedTableauSimulator::measure_kickback);
    c.function("do_circuit", &ExposedTableauSimulator::do_circuit);
    c.function("do_tableau", &ExposedTableauSimulator::do_tableau);
    c.function("do_pauli_string", &ExposedTableauSimulator::do_pauli_string);
    c.function("X", &ExposedTableauSimulator::X);
    c.function("Y", &ExposedTableauSimulator::Y);
    c.function("Z", &ExposedTableauSimulator::Z);
    c.function("H", &ExposedTableauSimulator::H);
    c.function("CNOT", &ExposedTableauSimulator::CNOT);
    c.function("CY", &ExposedTableauSimulator::CY);
    c.function("CZ", &ExposedTableauSimulator::CZ);
    c.function("SWAP", &ExposedTableauSimulator::SWAP);
    c.function("copy", &ExposedTableauSimulator::copy);
}
