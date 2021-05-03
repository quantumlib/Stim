#include <emscripten/bind.h>
#include "../../src/circuit/gate_data.h"
#include "common.js.h"
#include "tableau.js.h"

ExposedTableau::ExposedTableau(Tableau tableau) : tableau(tableau) {
}

ExposedTableau::ExposedTableau(int n) : tableau(n) {
}

ExposedTableau ExposedTableau::random(int n) {
    return ExposedTableau(Tableau::random(n, JS_BIND_SHARED_RNG()));
}

ExposedTableau ExposedTableau::from_named_gate(const std::string &name) {
    const Gate &gate = GATE_DATA.at(name.data());
    if (!(gate.flags & GATE_IS_UNITARY)) {
        throw std::out_of_range("Recognized name, but not unitary: " + name);
    }
    return ExposedTableau(gate.tableau());
}

std::string ExposedTableau::str() const {
    return tableau.str();
}

void emscripten_bind_tableau() {
    auto &&c = emscripten::class_<ExposedTableau>("Tableau");
    c.constructor<int>();
    c.class_function("random", &ExposedTableau::random);
    c.class_function("from_named_gate", &ExposedTableau::from_named_gate);
    c.function("toString", &ExposedTableau::str);
}
