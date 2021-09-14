#include "tableau.js.h"

#include <emscripten/bind.h>

#include "common.js.h"
#include "stim/circuit/gate_data.h"

using namespace stim;

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

ExposedPauliString ExposedTableau::x_output(size_t index) const {
    if (index >= tableau.num_qubits) {
        throw std::invalid_argument("target >= tableau.length");
    }
    return ExposedPauliString(tableau.xs[index]);
}
ExposedPauliString ExposedTableau::y_output(size_t index) const {
    if (index >= tableau.num_qubits) {
        throw std::invalid_argument("target >= tableau.length");
    }

    // Compute Y using Y = i*X*Z.
    uint8_t log_i = 1;
    PauliString copy = tableau.xs[index];
    log_i += copy.ref().inplace_right_mul_returning_log_i_scalar(tableau.zs[index]);
    if (log_i & 1) {
        throw std::out_of_range("Malformed tableau. X_k commutes with Z_k.");
    }
    copy.sign ^= (log_i & 2) != 0;
    return ExposedPauliString(copy);
}
ExposedPauliString ExposedTableau::z_output(size_t index) const {
    if (index >= tableau.num_qubits) {
        throw std::invalid_argument("target >= tableau.length");
    }
    return ExposedPauliString(tableau.zs[index]);
}

ExposedTableau ExposedTableau::from_conjugated_generators_xs_zs(
    const emscripten::val &xs_val, const emscripten::val &zs_val) {
    auto xs = emscripten::vecFromJSArray<ExposedPauliString>(xs_val);
    auto zs = emscripten::vecFromJSArray<ExposedPauliString>(zs_val);
    size_t n = xs.size();
    if (zs.size() != n) {
        throw std::invalid_argument("xs.length != zs.length");
    }
    for (const auto &e : xs) {
        if (e.pauli_string.num_qubits != n) {
            throw std::invalid_argument("x.length != xs.length");
        }
    }
    for (const auto &e : zs) {
        if (e.pauli_string.num_qubits != n) {
            throw std::invalid_argument("z.length != zs.length");
        }
    }
    Tableau result(n);
    for (size_t q = 0; q < n; q++) {
        result.xs[q] = xs[q].pauli_string;
        result.zs[q] = zs[q].pauli_string;
    }
    if (!result.satisfies_invariants()) {
        throw std::invalid_argument(
            "The given generator outputs don't describe a valid Clifford operation.\n"
            "They don't preserve commutativity.\n"
            "Everything must commute, except for X_k anticommuting with Z_k for each k.");
    }
    return ExposedTableau(result);
}

size_t ExposedTableau::getLength() const {
    return tableau.num_qubits;
}

bool ExposedTableau::isEqualTo(const ExposedTableau &other) const {
    return tableau == other.tableau;
}

void emscripten_bind_tableau() {
    auto c = emscripten::class_<ExposedTableau>("Tableau");
    c.constructor<int>();
    c.class_function("random", &ExposedTableau::random);
    c.class_function("from_named_gate", &ExposedTableau::from_named_gate);
    c.class_function("from_conjugated_generators_xs_zs", &ExposedTableau::from_conjugated_generators_xs_zs);
    c.function("x_output", &ExposedTableau::x_output);
    c.function("y_output", &ExposedTableau::y_output);
    c.function("z_output", &ExposedTableau::z_output);
    c.function("toString", &ExposedTableau::str);
    c.function("isEqualTo", &ExposedTableau::isEqualTo);
    c.property("length", &ExposedTableau::getLength);
}
