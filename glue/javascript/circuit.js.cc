#include "circuit.js.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace stim;

ExposedCircuit::ExposedCircuit() : circuit() {
}

ExposedCircuit::ExposedCircuit(Circuit circuit) : circuit(circuit) {
}

ExposedCircuit::ExposedCircuit(const std::string &text) : circuit(Circuit(text.data())) {
}

void ExposedCircuit::append_operation(
    const std::string &name, const emscripten::val &targets, const emscripten::val &args) {
    circuit.append_op(
        name.data(),
        emscripten::convertJSArrayToNumberVector<uint32_t>(targets),
        emscripten::convertJSArrayToNumberVector<double>(args));
}

void ExposedCircuit::append_from_stim_program_text(const std::string &text) {
    circuit.append_from_text(text.data());
}

ExposedCircuit ExposedCircuit::repeated(size_t repetitions) const {
    return ExposedCircuit(circuit * repetitions);
}

ExposedCircuit ExposedCircuit::copy() const {
    return ExposedCircuit(circuit);
}

std::string ExposedCircuit::toString() const {
    return circuit.str();
}

bool ExposedCircuit::isEqualTo(const ExposedCircuit &other) const {
    return circuit == other.circuit;
}

void emscripten_bind_circuit() {
    auto c = emscripten::class_<ExposedCircuit>("Circuit");
    c.constructor();
    c.constructor<const std::string &>();
    c.function("toString", &ExposedCircuit::toString);
    c.function("repeated", &ExposedCircuit::repeated);
    c.function("copy", &ExposedCircuit::copy);
    c.function("append_operation", &ExposedCircuit::append_operation);
    c.function("append_from_stim_program_text", &ExposedCircuit::append_from_stim_program_text);
    c.function("isEqualTo", &ExposedCircuit::isEqualTo);
}
