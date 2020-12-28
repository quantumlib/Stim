#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include "simd_util.h"
#include "pauli_string.h"

uint8_t PauliStringPtr::log_i_scalar_byproduct(const PauliStringPtr &other) const {
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
    for (auto w : cnt.m256i_u16) {
        s += (uint8_t)w;
    }
    return s & 3;
}

std::string PauliStringPtr::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool PauliStringPtr::operator==(const PauliStringPtr &other) const {
    if (size != other.size || *sign != *other.sign) {
        return false;
    }
    __m256i acc;
    for (uint64_t &i : acc.m256i_u64) {
        i = UINT64_MAX;
    }
    for (size_t i = 0; i*256 < size; i++) {
        auto dx = _mm256_xor_si256(_x[i], other._x[i]);
        acc = _mm256_andnot_si256(dx, acc);
        auto dy = _mm256_xor_si256(_y[i], other._y[i]);
        acc = _mm256_andnot_si256(dy, acc);
    }
    for (auto e : acc.m256i_u64) {
        if (e != UINT64_MAX) {
            return false;
        }
    }
    return true;
}

bool PauliStringPtr::operator!=(const PauliStringPtr &other) const {
    return !(*this == other);
}

std::ostream &operator<<(std::ostream &out, const PauliStringPtr &ps) {
    out << (*ps.sign ? '-' : '+');
    for (size_t i = 0; i < ps.size; i += 256) {
        auto xs = m256i_to_bits(ps._x[i / 256]);
        auto ys = m256i_to_bits(ps._y[i / 256]);
        for (int j = 0; j < 256 && i + j < ps.size; j++) {
            out << "_XYZ"[xs[j] + 2 * ys[j]];
        }
    }
    return out;
}

PauliStringStorage PauliStringStorage::from_pattern(bool sign, size_t size, const std::function<char(size_t)> &func) {
    PauliStringStorage result {};
    result.sign = sign;
    result.size = size;
    std::vector<bool> xs;
    std::vector<bool> ys;
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
            result.x.push_back(bits_to_m256i(xs));
            result.y.push_back(bits_to_m256i(ys));
            xs.clear();
            ys.clear();
        }
    }
    if (!xs.empty()) {
        result.x.push_back(bits_to_m256i(xs));
        result.y.push_back(bits_to_m256i(ys));
    }
    return result;
}

PauliStringStorage PauliStringStorage::from_str(const char *text) {
    PauliStringStorage result {};
    auto sign = text[0] == '-';
    if (text[0] == '+' || text[0] == '-') {
        text++;
    }
    return PauliStringStorage::from_pattern(sign, strlen(text), [&](size_t i){ return text[i]; });
}

PauliStringPtr PauliStringStorage::ptr() {
    return PauliStringPtr {size, &sign, x.data(), y.data()};
}

PauliStringPtr& PauliStringPtr::operator*=(const PauliStringPtr& rhs) {
    uint8_t log_i;
    inplace_times_with_scalar_output(rhs, &log_i);
    assert((log_i & 1) == 0);
    *sign ^= (log_i & 2) >> 1;
    return *this;
}

void PauliStringPtr::inplace_times_with_scalar_output(const PauliStringPtr& rhs, uint8_t *out_log_i) const {
    *out_log_i = log_i_scalar_byproduct(rhs);
    *sign ^= *rhs.sign;
    for (size_t i = 0; i < (size + 0xFF) >> 8; i++) {
        _x[i] = _mm256_xor_si256(_x[i], rhs._x[i]);
        _y[i] = _mm256_xor_si256(_y[i], rhs._y[i]);
    }
}
