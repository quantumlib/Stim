#include <emscripten/bind.h>
#include "common.js.h"
#include "pauli_string.js.h"

ExposedPauliString::ExposedPauliString(PauliString pauli_string) : pauli_string(pauli_string) {
}

ExposedPauliString::ExposedPauliString(const emscripten::val &arg) : pauli_string(0) {
    std::string t = arg.typeOf().as<std::string>();
    if (arg.isNumber()) {
        pauli_string = PauliString(js_val_to_uint32_t(arg));
    } else if (arg.isString()) {
        pauli_string = PauliString::from_str(arg.as<std::string>().data());
    } else {
        throw std::invalid_argument("Expected an int or a string. Got " + t);
    }
}

ExposedPauliString ExposedPauliString::random(size_t n) {
    return ExposedPauliString(PauliString::random(n, JS_BIND_SHARED_RNG()));
}

std::string ExposedPauliString::toString() const {
    return pauli_string.str();
}

void emscripten_bind_pauli_string() {
    auto &&c = emscripten::class_<ExposedPauliString>("PauliString");
    c.constructor<const emscripten::val &>();
    c.class_function("random", &ExposedPauliString::random);
    c.function("toString", &ExposedPauliString::toString);
}
