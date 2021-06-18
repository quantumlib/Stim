#ifndef STIM_PAULI_STRING_JS_H
#define STIM_PAULI_STRING_JS_H

#include <cstddef>
#include <cstdint>
#include <emscripten/val.h>

#include "../../src/stabilizers/pauli_string.h"

struct ExposedPauliString {
    stim_internal::PauliString pauli_string;
    explicit ExposedPauliString(stim_internal::PauliString pauli_string);
    explicit ExposedPauliString(const emscripten::val &arg);
    static ExposedPauliString random(size_t n);
    ExposedPauliString times(const ExposedPauliString &other) const;
    bool commutes(const ExposedPauliString &other) const;
    size_t getLength() const;
    int8_t getSign() const;
    uint8_t pauli(size_t index) const;
    ExposedPauliString neg() const;
    bool isEqualTo(const ExposedPauliString &other) const;
    std::string toString() const;
};

void emscripten_bind_pauli_string();

#endif
