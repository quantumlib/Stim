#ifndef PAULI_STRING_JS_H
#define PAULI_STRING_JS_H

#include <emscripten/val.h>
#include "../../src/stabilizers/pauli_string.h"

struct ExposedPauliString {
    PauliString pauli_string;
    explicit ExposedPauliString(PauliString pauli_string);
    explicit ExposedPauliString(const emscripten::val &arg);
    static ExposedPauliString random(size_t n);
    std::string toString() const;
};

void emscripten_bind_pauli_string();

#endif
