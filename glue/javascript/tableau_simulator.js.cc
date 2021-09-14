#include "tableau_simulator.js.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "common.js.h"

using namespace stim;

struct TempArgData {
    std::vector<GateTarget> targets;
    TempArgData(std::vector<GateTarget> targets) : targets(std::move(targets)) {
    }
    TempArgData(std::vector<uint32_t> init_targets) {
        for (auto e : init_targets) {
            targets.push_back(GateTarget{e});
        }
    }
    operator OperationData() {
        return {{}, targets};
    }
};

static TempArgData args_to_targets(TableauSimulator &self, const emscripten::val &args) {
    std::vector<uint32_t> result = emscripten::convertJSArrayToNumberVector<uint32_t>(args);
    uint32_t max_q = 0;
    for (uint32_t q : result) {
        max_q = std::max(max_q, q & TARGET_VALUE_MASK);
    }

    // Note: quadratic behavior.
    self.ensure_large_enough_for_qubits((size_t)max_q + 1);

    return TempArgData(result);
}

static TempArgData safe_targets(TableauSimulator &self, uint32_t target) {
    uint32_t max_q = target & TARGET_VALUE_MASK;
    self.ensure_large_enough_for_qubits((size_t)max_q + 1);
    return TempArgData({GateTarget{target}});
}

static TempArgData safe_targets(TableauSimulator &self, uint32_t target1, uint32_t target2) {
    uint32_t max_q = std::max(target1 & TARGET_VALUE_MASK, target2 & TARGET_VALUE_MASK);
    self.ensure_large_enough_for_qubits((size_t)max_q + 1);
    if (target1 == target2) {
        throw std::invalid_argument("target1 == target2");
    }
    return TempArgData({GateTarget{target1}, GateTarget{target2}});
}

static TempArgData args_to_target_pairs(TableauSimulator &self, const emscripten::val &args) {
    auto result = args_to_targets(self, args);
    if (result.targets.size() & 1) {
        throw std::out_of_range("Two qubit operation requires an even number of targets.");
    }
    return result;
}

ExposedTableauSimulator::ExposedTableauSimulator() : sim(JS_BIND_SHARED_RNG(), 0) {
}

bool ExposedTableauSimulator::measure(size_t target) {
    sim.ensure_large_enough_for_qubits(target + 1);
    sim.measure_z(TempArgData({GateTarget{target}}));
    return (bool)sim.measurement_record.storage.back();
}

emscripten::val ExposedTableauSimulator::measure_kickback(size_t target) {
    safe_targets(sim, target);
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
    circuit.circuit.for_each_operation([&](const Operation &op) {
        (sim.*op.gate->tableau_simulator_function)(op.target_data);
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
    sim.X(safe_targets(sim, target));
}
void ExposedTableauSimulator::Y(uint32_t target) {
    sim.Y(safe_targets(sim, target));
}
void ExposedTableauSimulator::Z(uint32_t target) {
    sim.Z(safe_targets(sim, target));
}
void ExposedTableauSimulator::H(uint32_t target) {
    sim.H_XZ(safe_targets(sim, target));
}
void ExposedTableauSimulator::CNOT(uint32_t control, uint32_t target) {
    sim.ZCX(safe_targets(sim, control, target));
}
void ExposedTableauSimulator::SWAP(uint32_t target1, uint32_t target2) {
    sim.SWAP(safe_targets(sim, target1, target2));
}
void ExposedTableauSimulator::CY(uint32_t control, uint32_t target) {
    sim.ZCY(safe_targets(sim, control, target));
}
void ExposedTableauSimulator::CZ(uint32_t control, uint32_t target) {
    sim.ZCZ(safe_targets(sim, control, target));
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
