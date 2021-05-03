#ifndef CIRCUIT_JS_H
#define CIRCUIT_JS_H

#include <emscripten/val.h>
#include "../../src/circuit/circuit.h"

struct ExposedCircuit {
    Circuit circuit;
    ExposedCircuit();
    explicit ExposedCircuit(Circuit circuit);
    explicit ExposedCircuit(const std::string &text);
    void append_operation(const std::string &name, const emscripten::val &targets, double arg);
    void append_from_stim_program_text(const std::string &text);
    ExposedCircuit repeated(size_t repetitions) const;
    std::string toString() const;
};

void emscripten_bind_circuit();

#endif
