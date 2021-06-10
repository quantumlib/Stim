#ifndef TABLEAU_JS_H
#define TABLEAU_JS_H

#include <cstddef>
#include <cstdint>

#include "../../src/stabilizers/tableau.h"
#include "pauli_string.js.h"

struct ExposedTableau {
    stim_internal::Tableau tableau;
    explicit ExposedTableau(stim_internal::Tableau tableau);
    explicit ExposedTableau(int n);
    static ExposedTableau random(int n);
    static ExposedTableau from_named_gate(const std::string &name);
    static ExposedTableau from_conjugated_generators_xs_zs(
        const emscripten::val &xs_val, const emscripten::val &zs_val);
    ExposedPauliString x_output(size_t index) const;
    ExposedPauliString y_output(size_t index) const;
    ExposedPauliString z_output(size_t index) const;
    size_t getLength() const;
    std::string str() const;
    bool isEqualTo(const ExposedTableau &other) const;
};

void emscripten_bind_tableau();

#endif
