#include "pauli_string.h"

#include <cstring>
#include <string>

#include "../simd/simd_util.h"

PauliString::operator const PauliStringRef() const {
    return ref();
}

PauliString::operator PauliStringRef() {
    return ref();
}

PauliString::PauliString(size_t num_qubits) : num_qubits(num_qubits), sign(false), xs(num_qubits), zs(num_qubits) {
}

PauliString::PauliString(const PauliStringRef &other)
    : num_qubits(other.num_qubits), sign((bool)other.sign), xs(other.xs), zs(other.zs) {
}

const PauliStringRef PauliString::ref() const {
    return PauliStringRef(
        num_qubits,
        // HACK: const correctness is temporarily removed, but immediately restored.
        bit_ref((bool *)&sign, 0), xs, zs);
}

PauliStringRef PauliString::ref() {
    return PauliStringRef(num_qubits, bit_ref(&sign, 0), xs, zs);
}

std::string PauliString::str() const {
    return ref().str();
}

PauliString &PauliString::operator=(const PauliStringRef &other) noexcept {
    (*this).~PauliString();
    new (this) PauliString(other);
    return *this;
}

std::ostream &operator<<(std::ostream &out, const PauliString &ps) {
    return out << ps.ref();
}

PauliString PauliString::from_func(bool sign, size_t num_qubits, const std::function<char(size_t)> &func) {
    PauliString result(num_qubits);
    result.sign = sign;
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
        result.xs.u64[i / 64] ^= (uint64_t)x << (i & 63);
        result.zs.u64[i / 64] ^= (uint64_t)z << (i & 63);
    }
    return result;
}

PauliString PauliString::from_str(const char *text) {
    auto sign = text[0] == '-';
    if (text[0] == '+' || text[0] == '-') {
        text++;
    }
    return PauliString::from_func(sign, strlen(text), [&](size_t i) {
        return text[i];
    });
}

PauliString PauliString::random(size_t num_qubits, std::mt19937_64 &rng) {
    auto result = PauliString(num_qubits);
    result.xs.randomize(num_qubits, rng);
    result.zs.randomize(num_qubits, rng);
    result.sign ^= rng() & 1;
    return result;
}

bool PauliString::operator==(const PauliStringRef &other) const {
    return ref() == other;
}

bool PauliString::operator!=(const PauliStringRef &other) const {
    return ref() != other;
}
