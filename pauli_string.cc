#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include "simd_util.h"
#include "pauli_string.h"
#include <cstring>
#include <cstdlib>

PauliString::~PauliString() {
    if (_x != nullptr) {
        _mm_free(_x);
        _x = nullptr;
    }
    if (_z != nullptr) {
        _mm_free(_z);
        _z = nullptr;
    }
}

PauliString::PauliString(const PauliString &other) {
    _sign = other._sign;
    size = other.size;
    size_t words = (size + 255) / 256;
    _x = (uint64_t *)_mm_malloc(sizeof(__m256i) * words, 32);
    _z = (uint64_t *)_mm_malloc(sizeof(__m256i) * words, 32);
    memcpy(_x, other._x, sizeof(__m256i) * words);
    memcpy(_z, other._z, sizeof(__m256i) * words);
}

PauliString::PauliString(size_t n_size) {
    size = n_size;
    _sign = false;
    size_t words = (size + 255) / 256;
    _x = (uint64_t *)_mm_malloc(sizeof(__m256i) * words, 32);
    _z = (uint64_t *)_mm_malloc(sizeof(__m256i) * words, 32);
    memset(_x, 0, sizeof(__m256i) * words);
    memset(_z, 0, sizeof(__m256i) * words);
}

PauliString::PauliString(PauliString &&other) noexcept {
    _sign = other._sign;
    size = other.size;
    _x = other._x;
    _z = other._z;
    other._x = nullptr;
    other._z = nullptr;
}

PauliString& PauliString::operator=(const PauliString &other) noexcept {
    size_t words = (size + 255) / 256;
    size_t other_words = (other.size + 255) / 256;
    if (words != other_words) {
        (*this).~PauliString();
        new(this) PauliString(other);
    } else {
        _sign = other._sign;
        size = other.size;
        memcpy(_x, other._x, sizeof(__m256i) * words);
        memcpy(_z, other._z, sizeof(__m256i) * words);
    }
    return *this;
}

PauliString& PauliString::operator=(PauliString &&other) noexcept {
    (*this).~PauliString();
    new(this) PauliString(other);
    return *this;
}

uint8_t PauliString::log_i_scalar_byproduct(const PauliString &other) const {
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

std::string PauliString::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool PauliString::operator==(const PauliString &other) const {
    if (size != other.size || _sign != other._sign) {
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

bool PauliString::operator!=(const PauliString &other) const {
    return !(*this == other);
}

std::ostream &operator<<(std::ostream &out, const PauliString &ps) {
    out << (ps._sign ? '-' : '+');
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

PauliString PauliString::from_pattern(bool sign, size_t size, const std::function<char(size_t)> &func) {
    PauliString result(size);
    result._sign = sign;
    std::vector<bool> xs;
    std::vector<bool> zs;
    auto xx = (__m256i *)result._x;
    auto zz = (__m256i *)result._z;
    for (size_t i = 0; i < result.size; i++) {
        char c = func(i);
        if (c == 'X') {
            xs.push_back(true);
            zs.push_back(false);
        } else if (c == 'Y') {
            xs.push_back(true);
            zs.push_back(true);
        } else if (c == 'Z') {
            xs.push_back(false);
            zs.push_back(true);
        } else if (c == '_' || c == 'I') {
            xs.push_back(false);
            zs.push_back(false);
        } else {
            throw std::runtime_error("Unrecognized pauli character. " + std::to_string(c));
        }
        if (xs.size() == 256) {
            *xx++ = bits_to_m256i(xs);
            *zz++ = bits_to_m256i(zs);
            xs.clear();
            zs.clear();
        }
    }
    if (!xs.empty()) {
        *xx++ = bits_to_m256i(xs);
        *zz++ = bits_to_m256i(zs);
    }
    return result;
}

PauliString PauliString::from_str(const char *text) {
    auto sign = text[0] == '-';
    if (text[0] == '+' || text[0] == '-') {
        text++;
    }
    return PauliString::from_pattern(sign, strlen(text), [&](size_t i){ return text[i]; });
}

PauliString PauliString::identity(size_t size) {
    return PauliString(size);
}

PauliString& PauliString::operator*=(const PauliString& rhs) {
    uint8_t log_i = inplace_right_mul_with_scalar_output(rhs);
    assert((log_i & 1) == 0);
    _sign ^= (log_i & 2) == 2;
    return *this;
}

uint8_t PauliString::inplace_right_mul_with_scalar_output(const PauliString& rhs) {
    uint8_t result = log_i_scalar_byproduct(rhs);
    _sign ^= rhs._sign;
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

bool PauliString::get_x_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    return ((_x[i0] >> i1) & 1) != 0;
}

bool PauliString::get_z_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    return ((_z[i0] >> i1) & 1) != 0;
}

void PauliString::toggle_x_bit(size_t k) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _x[i0] ^= 1ull << i1;
}

void PauliString::toggle_z_bit(size_t k) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _z[i0] ^= 1ull << i1;
}

void PauliString::set_x_bit(size_t k, bool val) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _x[i0] &= ~(1ull << i1);
    _x[i0] ^= (uint64_t)val << i1;
}

void PauliString::set_z_bit(size_t k, bool val) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _z[i0] &= ~(1ull << i1);
    _z[i0] ^= (uint64_t)val << i1;
}

void PauliString::gather_into(PauliString &out, const std::vector<size_t> &in_indices) const {
    assert(in_indices.size() == out.size);
    for (size_t k_out = 0; k_out < out.size; k_out++) {
        size_t k_in = in_indices[k_out];
        out.set_x_bit(k_out, get_x_bit(k_in));
        out.set_z_bit(k_out, get_z_bit(k_in));
    }
}

void PauliString::scatter_into(PauliString &out, const std::vector<size_t> &out_indices) const {
    assert(size == out_indices.size());
    for (size_t k_in = 0; k_in < size; k_in++) {
        size_t k_out = out_indices[k_in];
        out.set_x_bit(k_out, get_x_bit(k_in));
        out.set_z_bit(k_out, get_z_bit(k_in));
    }
    out._sign ^= _sign;
}
