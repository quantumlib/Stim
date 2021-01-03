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
    if (_y != nullptr) {
        _mm_free(_y);
        _y = nullptr;
    }
}

PauliString::PauliString(const PauliString &other) {
    _sign = other._sign;
    size = other.size;
    size_t words = (size + 255) / 256;
    _x = (__m256i*)_mm_malloc(sizeof(__m256i) * words, 32);
    _y = (__m256i*)_mm_malloc(sizeof(__m256i) * words, 32);
    memcpy(_x, other._x, sizeof(__m256i) * words);
    memcpy(_y, other._y, sizeof(__m256i) * words);
}

PauliString::PauliString(size_t n_size) {
    size = n_size;
    _sign = false;
    size_t words = (size + 255) / 256;
    _x = (__m256i*)_mm_malloc(sizeof(__m256i) * words, 32);
    _y = (__m256i*)_mm_malloc(sizeof(__m256i) * words, 32);
    memset(_x, 0, sizeof(__m256i) * words);
    memset(_y, 0, sizeof(__m256i) * words);
}

PauliString::PauliString(PauliString &&other) noexcept {
    _sign = other._sign;
    size = other.size;
    size_t words = (size + 255) / 256;
    _x = other._x;
    _y = other._y;
    other._x = nullptr;
    other._y = nullptr;
}

uint8_t PauliString::log_i_scalar_byproduct(const PauliString &other) const {
    assert(size == other.size);
    __m256i cnt = _mm256_set1_epi16(0);
    for (size_t i = 0; i < (size + 0xFF) >> 8; i++) {
        auto x0 = _mm256_andnot_si256(_y[i], _x[i]);
        auto y0 = _mm256_andnot_si256(_x[i], _y[i]);
        auto z0 = _mm256_and_si256(_x[i], _y[i]);

        auto x1 = _mm256_andnot_si256(other._y[i], other._x[i]);
        auto y1 = _mm256_andnot_si256(other._x[i], other._y[i]);
        auto z1 = _mm256_and_si256(other._x[i], other._y[i]);

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
    for (size_t i = 0; i*256 < size; i++) {
        auto dx = _mm256_xor_si256(_x[i], other._x[i]);
        acc = _mm256_andnot_si256(dx, acc);
        auto dy = _mm256_xor_si256(_y[i], other._y[i]);
        acc = _mm256_andnot_si256(dy, acc);
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
    for (size_t i = 0; i < ps.size; i += 256) {
        auto xs = m256i_to_bits(ps._x[i / 256]);
        auto ys = m256i_to_bits(ps._y[i / 256]);
        for (int j = 0; j < 256 && i + j < ps.size; j++) {
            out << "_XYZ"[xs[j] + 2 * ys[j]];
        }
    }
    return out;
}

PauliString PauliString::from_pattern(bool sign, size_t size, const std::function<char(size_t)> &func) {
    PauliString result(size);
    result._sign = sign;
    std::vector<bool> xs;
    std::vector<bool> ys;
    auto xx = result._x;
    auto yy = result._y;
    for (size_t i = 0; i < result.size; i++) {
        char c = func(i);
        if (c == 'X') {
            xs.push_back(true);
            ys.push_back(false);
        } else if (c == 'Y') {
            xs.push_back(false);
            ys.push_back(true);
        } else if (c == 'Z') {
            xs.push_back(true);
            ys.push_back(true);
        } else if (c == '_' || c == 'I') {
            xs.push_back(false);
            ys.push_back(false);
        } else {
            throw std::runtime_error("Unrecognized pauli character. " + std::to_string(c));
        }
        if (xs.size() == 256) {
            *xx++ = bits_to_m256i(xs);
            *yy++ = bits_to_m256i(ys);
            xs.clear();
            ys.clear();
        }
    }
    if (!xs.empty()) {
        *xx++ = bits_to_m256i(xs);
        *yy++ = bits_to_m256i(ys);
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
    uint8_t log_i;
    inplace_right_mul_with_scalar_output(rhs, &log_i);
    assert((log_i & 1) == 0);
    _sign ^= (log_i & 2) == 2;
    return *this;
}

void PauliString::inplace_right_mul_with_scalar_output(const PauliString& rhs, uint8_t *out_log_i) {
    *out_log_i = log_i_scalar_byproduct(rhs);
    _sign ^= rhs._sign;
    for (size_t i = 0; i < (size + 0xFF) >> 8; i++) {
        _x[i] = _mm256_xor_si256(_x[i], rhs._x[i]);
        _y[i] = _mm256_xor_si256(_y[i], rhs._y[i]);
    }
}

bool PauliString::get_x_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    auto x64 = (uint64_t *)&_x;
    return ((x64[i0] >> i1) & 1) != 0;
}

bool PauliString::get_y_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    auto y64 = (uint64_t *)&_y;
    return ((y64[i0] >> i1) & 1) != 0;
}

void PauliString::toggle_x_bit(size_t k) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    auto x64 = (uint64_t *)&_x;
    x64[i0] ^= 1ull << i1;
}

void PauliString::toggle_y_bit(size_t k) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    auto y64 = (uint64_t *)&_y;
    y64[i0] ^= 1ull << i1;
}

void PauliString::gather_into(PauliString &out, const size_t *in_indices) const {
    for (size_t k_out = 0; k_out < out.size; k_out++) {
        size_t k_in = in_indices[k_out];
        if (get_x_bit(k_in)) {
            out.toggle_x_bit(k_out);
        }
        if (get_y_bit(k_in)) {
            out.toggle_y_bit(k_out);
        }
    }
}

void PauliString::scatter_into(PauliString &out, const size_t *out_indices) const {
    for (size_t k_in = 0; k_in < size; k_in++) {
        size_t k_out = out_indices[k_in];
        if (get_x_bit(k_in)) {
            out.toggle_x_bit(k_out);
        }
        if (get_y_bit(k_in)) {
            out.toggle_y_bit(k_out);
        }
    }
}
