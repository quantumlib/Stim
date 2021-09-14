#include "pauli_string.js.h"

#include <emscripten/bind.h>

#include "common.js.h"

using namespace stim;

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

ExposedPauliString ExposedPauliString::times(const ExposedPauliString &other) const {
    PauliString result = pauli_string;
    uint8_t r = result.ref().inplace_right_mul_returning_log_i_scalar(other.pauli_string);
    if (r & 1) {
        throw std::invalid_argument("Multiplied non-commuting.");
    }
    if (r & 2) {
        result.sign ^= 1;
    }
    return ExposedPauliString(result);
}

ExposedPauliString ExposedPauliString::neg() const {
    PauliString result = pauli_string;
    result.sign ^= 1;
    return ExposedPauliString(result);
}

bool ExposedPauliString::isEqualTo(const ExposedPauliString &other) const {
    return pauli_string == other.pauli_string;
}

bool ExposedPauliString::commutes(const ExposedPauliString &other) const {
    return pauli_string.ref().commutes(other.pauli_string);
}

std::string ExposedPauliString::toString() const {
    return pauli_string.str();
}

uint8_t ExposedPauliString::pauli(size_t index) const {
    if (index >= pauli_string.num_qubits) {
        return 0;
    }
    uint8_t x = pauli_string.xs[index];
    uint8_t z = pauli_string.zs[index];
    return pauli_xz_to_xyz(x, z);
}

size_t ExposedPauliString::getLength() const {
    return pauli_string.num_qubits;
}

int8_t ExposedPauliString::getSign() const {
    return pauli_string.sign ? -1 : +1;
}

void emscripten_bind_pauli_string() {
    auto c = emscripten::class_<ExposedPauliString>("PauliString");
    c.constructor<const emscripten::val &>();
    c.class_function("random", &ExposedPauliString::random);
    c.function("neg", &ExposedPauliString::neg);
    c.function("times", &ExposedPauliString::times);
    c.function("commutes", &ExposedPauliString::commutes);
    c.function("isEqualTo", &ExposedPauliString::isEqualTo);
    c.function("toString", &ExposedPauliString::toString);
    c.function("pauli", &ExposedPauliString::pauli);
    c.property("length", &ExposedPauliString::getLength);
    c.property("sign", &ExposedPauliString::getSign);
}
