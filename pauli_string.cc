#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include "simd_util.h"
#include "pauli_string.h"
#include <cstring>
#include <cstdlib>

PauliStringPtr::PauliStringPtr(size_t init_size, bool *init_sign, uint64_t *init_x, uint64_t *init_z) :
        size(init_size),
        ptr_sign(init_sign),
        _x(init_x),
        _z(init_z) {
}

PauliStringVal::PauliStringVal(size_t init_size) :
        val_sign(false),
        x_data(init_size),
        z_data(init_size) {
}

PauliStringVal::PauliStringVal(const PauliStringPtr &other) :
        val_sign(*other.ptr_sign),
        x_data(other.size, other._x),
        z_data(other.size, other._z) {
}

PauliStringPtr PauliStringVal::ptr() const {
    return PauliStringPtr(*this);
}

std::string PauliStringVal::str() const {
    return ptr().str();
}

void PauliStringPtr::overwrite_with(const PauliStringPtr &other) {
    assert(size == other.size);
    size_t words = (size + 255) / 256;
    *ptr_sign = *other.ptr_sign;
    memcpy(_x, other._x, sizeof(__m256i) * words);
    memcpy(_z, other._z, sizeof(__m256i) * words);
}

PauliStringVal& PauliStringVal::operator=(const PauliStringPtr &other) noexcept {
    (*this).~PauliStringVal();
    new(this) PauliStringVal(other);
    return *this;
}

PauliStringPtr::PauliStringPtr(const PauliStringVal &other) :
        size(other.x_data.num_bits),
        ptr_sign((bool *)&other.val_sign),
        _x(other.x_data.data),
        _z(other.z_data.data) {
}

uint8_t PauliStringPtr::log_i_scalar_byproduct(const PauliStringPtr &other) const {
    auto x256 = (__m256i *)_x;
    auto z256 = (__m256i *)_z;
    auto ox256 = (__m256i *)other._x;
    auto oz256 = (__m256i *)other._z;

    assert(size == other.size);
    __m256i cnt = _mm256_set1_epi16(0);
    for (size_t i = 0; i < (size + 0xFF) >> 8; i++) {
        auto x0 = _mm256_andnot_si256(z256[i], x256[i]);
        auto y0 = _mm256_and_si256(x256[i], z256[i]);
        auto z0 = _mm256_andnot_si256(x256[i], z256[i]);

        auto x1 = _mm256_andnot_si256(oz256[i], ox256[i]);
        auto y1 = _mm256_and_si256(ox256[i], oz256[i]);
        auto z1 = _mm256_andnot_si256(ox256[i], oz256[i]);

        auto f1 = _mm256_and_si256(x0, y1);
        auto f2 = _mm256_and_si256(y0, z1);
        auto f3 = _mm256_and_si256(z0, x1);
        auto f = _mm256_or_si256(f1, _mm256_or_si256(f2, f3));

        auto b1 = _mm256_and_si256(x0, z1);
        auto b2 = _mm256_and_si256(y0, x1);
        auto b3 = _mm256_and_si256(z0, y1);
        auto b = _mm256_or_si256(b1, _mm256_or_si256(b2, b3));

        cnt = _mm256_add_epi16(cnt, popcnt16(f));
        cnt = _mm256_sub_epi16(cnt, popcnt16(b));
    }

    uint8_t s = 0;
    auto cnt16 = (uint16_t *)&cnt;
    for (size_t k = 0; k < 16; k++) {
        s += (uint8_t)cnt16[k];
    }
    return s & 3;
}

std::string PauliStringPtr::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool PauliStringPtr::operator==(const PauliStringPtr &other) const {
    if (size != other.size || *ptr_sign != *other.ptr_sign) {
        return false;
    }
    __m256i acc;
    auto acc64 = (uint64_t *)&acc;
    for (size_t k = 0; k < 4; k++) {
        acc64[k] = UINT64_MAX;
    }
    auto x256 = (__m256i *)_x;
    auto z256 = (__m256i *)_z;
    auto ox256 = (__m256i *)other._x;
    auto oz256 = (__m256i *)other._z;
    for (size_t i = 0; i*256 < size; i++) {
        auto dx = _mm256_xor_si256(x256[i], ox256[i]);
        acc = _mm256_andnot_si256(dx, acc);
        auto dz = _mm256_xor_si256(z256[i], oz256[i]);
        acc = _mm256_andnot_si256(dz, acc);
    }
    for (size_t k = 0; k < 4; k++) {
        if (acc64[k] != UINT64_MAX) {
            return false;
        }
    }
    return true;
}

bool PauliStringPtr::operator!=(const PauliStringPtr &other) const {
    return !(*this == other);
}

std::ostream &operator<<(std::ostream &out, const PauliStringVal &ps) {
    return out << ps.ptr();
}
std::ostream &operator<<(std::ostream &out, const PauliStringPtr &ps) {
    out << (*ps.ptr_sign ? '-' : '+');
    auto x256 = (__m256i *)ps._x;
    auto z256 = (__m256i *)ps._z;
    for (size_t i = 0; i < ps.size; i += 256) {
        auto xs = m256i_to_bits(x256[i / 256]);
        auto zs = m256i_to_bits(z256[i / 256]);
        for (int j = 0; j < 256 && i + j < ps.size; j++) {
            out << "_XZY"[xs[j] + 2 * zs[j]];
        }
    }
    return out;
}

PauliStringVal PauliStringVal::from_pattern(bool sign, size_t size, const std::function<char(size_t)> &func) {
    PauliStringVal result(size);
    result.val_sign = sign;
    for (size_t i = 0; i < size; i++) {
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
        result.x_data.data[i / 64] ^= (uint64_t)x << (i & 63);
        result.z_data.data[i / 64] ^= (uint64_t)z << (i & 63);
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

PauliStringVal PauliStringVal::identity(size_t size) {
    return PauliStringVal(size);
}

PauliStringPtr& PauliStringPtr::operator*=(const PauliStringPtr& rhs) {
    uint8_t log_i = inplace_right_mul_with_scalar_output(rhs);
    assert((log_i & 1) == 0);
    *ptr_sign ^= (log_i & 2) == 2;
    return *this;
}

uint8_t PauliStringPtr::inplace_right_mul_with_scalar_output(const PauliStringPtr& rhs) {
    uint8_t result = log_i_scalar_byproduct(rhs);
    *ptr_sign ^= *rhs.ptr_sign;
    auto x256 = (__m256i *)_x;
    auto z256 = (__m256i *)_z;
    auto ox256 = (__m256i *)rhs._x;
    auto oz256 = (__m256i *)rhs._z;
    for (size_t i = 0; i < (size + 0xFF) >> 8; i++) {
        x256[i] = _mm256_xor_si256(x256[i], ox256[i]);
        z256[i] = _mm256_xor_si256(z256[i], oz256[i]);
    }
    return result;
}

bool PauliStringPtr::get_x_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    return ((_x[i0] >> i1) & 1) != 0;
}

bool PauliStringPtr::get_z_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    return ((_z[i0] >> i1) & 1) != 0;
}

void PauliStringPtr::toggle_x_bit(size_t k) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _x[i0] ^= 1ull << i1;
}

void PauliStringPtr::toggle_z_bit(size_t k) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _z[i0] ^= 1ull << i1;
}

void PauliStringPtr::set_x_bit(size_t k, bool val) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _x[i0] &= ~(1ull << i1);
    _x[i0] ^= (uint64_t)val << i1;
}

void PauliStringPtr::set_z_bit(size_t k, bool val) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _z[i0] &= ~(1ull << i1);
    _z[i0] ^= (uint64_t)val << i1;
}

void PauliStringPtr::gather_into(PauliStringPtr &out, const std::vector<size_t> &in_indices) const {
    assert(in_indices.size() == out.size);
    for (size_t k_out = 0; k_out < out.size; k_out++) {
        size_t k_in = in_indices[k_out];
        out.set_x_bit(k_out, get_x_bit(k_in));
        out.set_z_bit(k_out, get_z_bit(k_in));
    }
}

void PauliStringPtr::scatter_into(PauliStringPtr &out, const std::vector<size_t> &out_indices) const {
    assert(size == out_indices.size());
    for (size_t k_in = 0; k_in < size; k_in++) {
        size_t k_out = out_indices[k_in];
        out.set_x_bit(k_out, get_x_bit(k_in));
        out.set_z_bit(k_out, get_z_bit(k_in));
    }
    *out.ptr_sign ^= *ptr_sign;
}

bool PauliStringVal::operator==(const PauliStringPtr &other) const {
    return ptr() == other;
}
bool PauliStringVal::operator!=(const PauliStringPtr &other) const {
    return ptr() != other;
}
