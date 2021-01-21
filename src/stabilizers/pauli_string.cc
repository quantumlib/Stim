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

PauliStringRef::PauliStringRef(
        size_t init_num_qubits,
        bit_ref init_sign_ref,
        simd_bits_range_ref init_x_ref,
        simd_bits_range_ref init_z_ref) :
        num_qubits(init_num_qubits),
        sign_ref(init_sign_ref),
        x_ref(init_x_ref),
        z_ref(init_z_ref) {
}

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

void PauliStringRef::swap_with(PauliStringRef other) {
    assert(num_qubits == other.num_qubits);
    sign_ref.swap_with(other.sign_ref);
    x_ref.swap_with(other.x_ref);
    z_ref.swap_with(other.z_ref);
}

PauliStringRef &PauliStringRef::operator=(const PauliStringRef &other) {
    assert(num_qubits == other.num_qubits);
    sign_ref = other.sign_ref;
    assert((bool)sign_ref == (bool)other.sign_ref);
    x_ref = other.x_ref;
    z_ref = other.z_ref;
    return *this;
}

PauliStringVal& PauliStringVal::operator=(const PauliStringRef &other) noexcept {
    (*this).~PauliStringVal();
    new(this) PauliStringVal(other);
    return *this;
}

SparsePauliString PauliStringRef::sparse() const {
    SparsePauliString result {
        (bool)sign_ref,
        {}
    };
    auto n = x_ref.num_u64_padded();
    for (size_t k = 0; k < n; k++) {
        auto wx = x_ref.u64[k];
        auto wz = z_ref.u64[k];
        if (wx | wz) {
            result.indexed_words.push_back({k, wx, wz});
        }
    }
    return result;
}

std::string PauliStringRef::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::string SparsePauliString::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool PauliStringRef::operator==(const PauliStringRef &other) const {
    return num_qubits == other.num_qubits && sign_ref == other.sign_ref && x_ref == other.x_ref && z_ref == other.z_ref;
}

bool PauliStringRef::operator!=(const PauliStringRef &other) const {
    return !(*this == other);
}

std::ostream &operator<<(std::ostream &out, const SparsePauliString &ps) {
    out << "+-"[ps.sign];
    bool first = true;
    for (const auto &w : ps.indexed_words) {
        for (size_t k2 = 0; k2 < 64; k2++) {
            auto x = (w.wx >> k2) & 1;
            auto z = (w.wz >> k2) & 1;
            auto p = x + 2*z;
            if (p) {
                if (!first) {
                    out << '*';
                }
                first = false;
                out << "IXZY"[p] << ((w.index64 << 6) | k2);
            }
        }
    }
    if (first) {
        out << 'I';
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const PauliStringVal &ps) {
    return out << ps.ref();
}

std::ostream &operator<<(std::ostream &out, const PauliStringRef &ps) {
    out << "+-"[ps.sign_ref];
    for (size_t k = 0; k < ps.num_qubits; k++) {
        out << "_XZY"[ps.x_ref[k] + 2 * ps.z_ref[k]];
    }
    return out;
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

PauliStringRef& PauliStringRef::operator*=(const PauliStringRef& rhs) {
    uint8_t log_i = inplace_right_mul_returning_log_i_scalar(rhs);
    assert((log_i & 1) == 0);
    sign_ref ^= log_i & 2;
    return *this;
}

uint8_t PauliStringRef::inplace_right_mul_returning_log_i_scalar(const PauliStringRef& rhs) noexcept {
    assert(num_qubits == rhs.num_qubits);

    // Accumulator registers for counting mod 4 in parallel across each bit position.
    SIMD_WORD_TYPE cnt1 {};
    SIMD_WORD_TYPE cnt2 {};

    x_ref.for_each_word(z_ref, rhs.x_ref, rhs.z_ref,  [&cnt1, &cnt2](auto &x1, auto &z1, auto &x2, auto &z2) {
        // Update the left hand side Paulis.
        auto old_x1 = x1;
        auto old_z1 = z1;
        x1 ^= x2;
        z1 ^= z2;

        // At each bit position: accumulate anti-commutation (+i or -i) counts.
        auto x1z2 = old_x1 & z2;
        auto anti_commutes = (x2 & old_z1) ^ x1z2;
        cnt2 ^= (cnt1 ^ x1 ^ z1 ^ x1z2) & anti_commutes;
        cnt1 ^= anti_commutes;
    });

    // Combine final anti-commutation phase tally (mod 4).
    uint8_t s = cnt1.popcount();
    s ^= cnt2.popcount() << 1;
    s ^= (uint8_t)rhs.sign_ref << 1;
    return s & 3;
}

PauliStringVal PauliStringVal::random(size_t num_qubits, std::mt19937& rng) {
    auto result = PauliStringVal(num_qubits);
    result.x_data.randomize(num_qubits, rng);
    result.z_data.randomize(num_qubits, rng);
    result.val_sign ^= rng() & 1;
    return result;
}

bool PauliStringRef::commutes(const PauliStringRef& other) const noexcept {
    assert(num_qubits == other.num_qubits);
    SIMD_WORD_TYPE cnt1 {};
    x_ref.for_each_word(z_ref, other.x_ref, other.z_ref, [&cnt1](auto &x1, auto &z1, auto &x2, auto &z2) {
        cnt1 ^= (x1 & z2) ^ (x2 & z1);
    });
    return (cnt1.popcount() & 1) == 0;
}

void PauliStringRef::gather_into(PauliStringRef out, const std::vector<size_t> &in_indices) const {
    assert(in_indices.size() == out.num_qubits);
    for (size_t k_out = 0; k_out < out.num_qubits; k_out++) {
        size_t k_in = in_indices[k_out];
        out.x_ref[k_out] = x_ref[k_in];
        out.z_ref[k_out] = z_ref[k_in];
    }
}

void PauliStringRef::scatter_into(PauliStringRef out, const std::vector<size_t> &out_indices) const {
    assert(num_qubits == out_indices.size());
    for (size_t k_in = 0; k_in < num_qubits; k_in++) {
        size_t k_out = out_indices[k_in];
        out.x_ref[k_out] = x_ref[k_in];
        out.z_ref[k_out] = z_ref[k_in];
    }
    out.sign_ref ^= sign_ref;
}

bool PauliStringVal::operator==(const PauliStringRef &other) const {
    return ref() == other;
}
bool PauliStringVal::operator!=(const PauliStringRef &other) const {
    return ref() != other;
}
