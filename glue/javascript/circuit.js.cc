#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "circuit.js.h"

ExposedCircuit::ExposedCircuit() : circuit() {
}

ExposedCircuit::ExposedCircuit(Circuit circuit) : circuit(circuit) {
}

ExposedCircuit::ExposedCircuit(const std::string &text) : circuit(Circuit::from_text(text.data())) {
}

void ExposedCircuit::append_operation(const std::string &name, const emscripten::val &targets, double arg) {
    circuit.append_op(name.data(), emscripten::convertJSArrayToNumberVector<uint32_t>(targets), arg);
}

void ExposedCircuit::append_from_stim_program_text(const std::string &text) {
    circuit.append_from_text(text.data());
}

ExposedCircuit ExposedCircuit::repeated(size_t repetitions) const {
    return ExposedCircuit(circuit * repetitions);
}

std::string ExposedCircuit::toString() const {
    return "new stim.Circuit(`\n" + circuit.str() + "\n`)";
}

void emscripten_bind_circuit() {
    auto &&c = emscripten::class_<ExposedCircuit>("Circuit");
    c.constructor();
    c.constructor<const std::string &>();
    c.function("toString", &ExposedCircuit::toString);
    c.function("append_operation", &ExposedCircuit::append_operation);
    c.function("append_from_stim_program_text", &ExposedCircuit::append_from_stim_program_text);
}
