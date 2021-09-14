#ifndef STIM_CIRCUIT_JS_H
#define STIM_CIRCUIT_JS_H

#include <cstddef>
#include <cstdint>
#include <emscripten/val.h>

#include "stim/circuit/circuit.h"

struct ExposedCircuit {
    stim::Circuit circuit;
    ExposedCircuit();
    explicit ExposedCircuit(stim::Circuit circuit);
    explicit ExposedCircuit(const std::string &text);
    void append_operation(const std::string &name, const emscripten::val &targets, const emscripten::val &args);
    void append_from_stim_program_text(const std::string &text);
    ExposedCircuit repeated(size_t repetitions) const;
    ExposedCircuit copy() const;
    bool isEqualTo(const ExposedCircuit &other) const;
    std::string toString() const;
};

void emscripten_bind_circuit();

#endif
