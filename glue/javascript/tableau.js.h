#ifndef STIM_TABLEAU_JS_H
#define STIM_TABLEAU_JS_H

#include <cstddef>
#include <cstdint>

#include "pauli_string.js.h"
#include "stim/stabilizers/tableau.h"

struct ExposedTableau {
    stim::Tableau tableau;
    explicit ExposedTableau(stim::Tableau tableau);
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
