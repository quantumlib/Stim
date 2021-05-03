#ifndef TABLEAU_JS_H
#define TABLEAU_JS_H

#include "../../src/stabilizers/tableau.h"

struct ExposedTableau {
    Tableau tableau;
    explicit ExposedTableau(Tableau tableau);
    explicit ExposedTableau(int n);
    static ExposedTableau random(int n);
    static ExposedTableau from_named_gate(const std::string &name);
    std::string str() const;
};

void emscripten_bind_tableau();

#endif
