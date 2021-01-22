#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include "../simd/simd_util.h"
#include "pauli_string.h"
#include <cstring>
#include <cstdlib>
#include <bit>

PauliStringVal::operator const PauliStringRef() const {
    return ref();
}

PauliStringVal::operator PauliStringRef() {
    return ref();
}

PauliStringVal::PauliStringVal(size_t num_qubits) :
        num_qubits(num_qubits),
        val_sign(false),
        x_data(num_qubits),
        z_data(num_qubits) {
}

PauliStringVal::PauliStringVal(const PauliStringRef &other) :
        num_qubits(other.num_qubits),
        val_sign((bool)other.sign_ref),
        x_data(other.x_ref),
        z_data(other.z_ref) {
}

const PauliStringRef PauliStringVal::ref() const {
    return PauliStringRef(
        num_qubits,
        // HACK: const correctness is temporarily removed, but immediately restored.
        bit_ref((bool *)&val_sign, 0),
        x_data,
        z_data);
}

PauliStringRef PauliStringVal::ref() {
    return PauliStringRef(
        num_qubits,
        bit_ref(&val_sign, 0),
        x_data,
        z_data);
}

std::string PauliStringVal::str() const {
    return ref().str();
}

PauliStringVal& PauliStringVal::operator=(const PauliStringRef &other) noexcept {
    (*this).~PauliStringVal();
    new(this) PauliStringVal(other);
    return *this;
}

std::ostream &operator<<(std::ostream &out, const PauliStringVal &ps) {
    return out << ps.ref();
}

PauliStringVal PauliStringVal::from_pattern(bool sign, size_t num_qubits, const std::function<char(size_t)> &func) {
    PauliStringVal result(num_qubits);
    result.val_sign = sign;
    for (size_t i = 0; i < num_qubits; i++) {
        char c = func(i);
        bool x;
        bool z;
        if (c == 'X') {
            x = true;
            z = false;
        } else if (c == 'Y') {
            x = true;
            z = true;
        } else if (c == 'Z') {
            x = false;
            z = true;
        } else if (c == '_' || c == 'I') {
            x = false;
            z = false;
        } else {
            throw std::runtime_error("Unrecognized pauli character. " + std::to_string(c));
        }
        result.x_data.u64[i / 64] ^= (uint64_t)x << (i & 63);
        result.z_data.u64[i / 64] ^= (uint64_t)z << (i & 63);
    }
    return result;
}

PauliStringVal PauliStringVal::from_str(const char *text) {
    auto sign = text[0] == '-';
    if (text[0] == '+' || text[0] == '-') {
        text++;
    }
    return PauliStringVal::from_pattern(sign, strlen(text), [&](size_t i){ return text[i]; });
}

PauliStringVal PauliStringVal::identity(size_t num_qubits) {
    return PauliStringVal(num_qubits);
}

PauliStringVal PauliStringVal::random(size_t num_qubits, std::mt19937_64 &rng) {
    auto result = PauliStringVal(num_qubits);
    result.x_data.randomize(num_qubits, rng);
    result.z_data.randomize(num_qubits, rng);
    result.val_sign ^= rng() & 1;
    return result;
}

bool PauliStringVal::operator==(const PauliStringRef &other) const {
    return ref() == other;
}

bool PauliStringVal::operator!=(const PauliStringRef &other) const {
    return ref() != other;
}
