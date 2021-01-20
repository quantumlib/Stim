#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include "simd_util.h"
#include "pauli_string.h"
#include <cstring>
#include <cstdlib>
#include <bit>

PauliStringPtr::PauliStringPtr(
            size_t init_size,
            BitPtr init_sign,
            uint64_t *init_x,
            uint64_t *init_z) :
        size(init_size),
        bit_ptr_sign(init_sign),
        _x(init_x),
        _z(init_z) {
}

PauliStringVal::PauliStringVal(size_t init_size) :
        val_sign(false),
        x_data(init_size),
        z_data(init_size) {
}

PauliStringVal::PauliStringVal(const PauliStringPtr &other) :
        val_sign(other.bit_ptr_sign.get()),
        x_data(other.size, other._x),
        z_data(other.size, other._z) {
}

PauliStringPtr PauliStringVal::ptr() const {
    return PauliStringPtr(*this);
}

std::string PauliStringVal::str() const {
    return ptr().str();
}

void PauliStringPtr::swap_with(PauliStringPtr &other) {
    assert(size == other.size);
    bit_ptr_sign.swap(other.bit_ptr_sign);
    x_rng().swap_with((__m256i *)other._x);
    z_rng().swap_with((__m256i *)other._z);
}

SimdRange PauliStringPtr::x_rng() {
    return SimdRange {(__m256i *)_x, num_words256()};
}
SimdRange PauliStringPtr::z_rng() {
    return SimdRange {(__m256i *)_z, num_words256()};
}

void PauliStringPtr::overwrite_with(const PauliStringPtr &other) {
    assert(size == other.size);
    bit_ptr_sign.set(other.bit_ptr_sign.get());
    x_rng().overwrite_with((__m256i *)other._x);
    z_rng().overwrite_with((__m256i *)other._z);
}

PauliStringVal& PauliStringVal::operator=(const PauliStringPtr &other) noexcept {
    (*this).~PauliStringVal();
    new(this) PauliStringVal(other);
    return *this;
}

PauliStringPtr::PauliStringPtr(const PauliStringVal &other) :
        size(other.x_data.num_bits),
        bit_ptr_sign(BitPtr((void *)&other.val_sign, 0)),
        _x(other.x_data.u64),
        _z(other.z_data.u64) {
}

SparsePauliString PauliStringPtr::sparse() const {
    SparsePauliString result {
        bit_ptr_sign.get(),
        {}
    };
    auto n = (size + 63) >> 6;
    for (size_t k = 0; k < n; k++) {
        auto wx = _x[k];
        auto wz = _z[k];
        if (wx | wz) {
            result.indexed_words.push_back({k, wx, wz});
        }
    }
    return result;
}

std::string PauliStringPtr::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::string SparsePauliString::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool PauliStringPtr::operator==(const PauliStringPtr &other) const {
    if (size != other.size || bit_ptr_sign.get() != other.bit_ptr_sign.get()) {
        return false;
    }
    auto n = num_words256() << 5;
    return memcmp(_x, other._x, n) == 0 && memcmp(_z, other._z, n) == 0;
}

bool PauliStringPtr::operator!=(const PauliStringPtr &other) const {
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
    return out << ps.ptr();
}

size_t PauliStringPtr::num_words256() const {
    return ceil256(size) >> 8;
}

std::ostream &operator<<(std::ostream &out, const PauliStringPtr &ps) {
    out << (ps.bit_ptr_sign.get() ? '-' : '+');
    auto x256 = (__m256i *)ps._x;
    auto z256 = (__m256i *)ps._z;
    auto end = &x256[ps.num_words256()];
    size_t remaining = ps.size;
    while (x256 != end) {
        auto xs = m256i_to_bits(*x256);
        auto zs = m256i_to_bits(*z256);
        for (int j = 0; j < 256 && remaining; j++) {
            out << "_XZY"[xs[j] + 2 * zs[j]];
            remaining--;
        }
        x256++;
        z256++;
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

PauliStringVal PauliStringVal::identity(size_t size) {
    return PauliStringVal(size);
}

PauliStringPtr& PauliStringPtr::operator*=(const PauliStringPtr& rhs) {
    uint8_t log_i = inplace_right_mul_returning_log_i_scalar(rhs);
    assert((log_i & 1) == 0);
    bit_ptr_sign.toggle_if(log_i & 2);
    return *this;
}

uint8_t PauliStringPtr::inplace_right_mul_returning_log_i_scalar(const PauliStringPtr& rhs) noexcept {
    assert(size == rhs.size);

    // Accumulator registers for counting mod 4 in parallel across each bit position.
    __m256i cnt1 {};
    __m256i cnt2 {};

    simd_for_each_4(
            (__m256i *)_x,
            (__m256i *)_z,
            (__m256i *)rhs._x,
            (__m256i *)rhs._z,
            num_words256(),
            [&cnt1, &cnt2](auto px1, auto pz1, auto px2, auto pz2) {
                // Load into registers.
                auto x1 = *px1;
                auto z2 = *pz2;
                auto z1 = *pz1;
                auto x2 = *px2;

                // Update the left hand side Paulis.
                *px1 = x1 ^ x2;
                *pz1 = z1 ^ z2;

                // At each bit position: accumulate anti-commutation (+i or -i) counts.
                auto x1z2 = x1 & z2;
                auto anti_commutes = (x2 & z1) ^ x1z2;
                cnt2 ^= (cnt1 ^ *px1 ^ *pz1 ^ x1z2) & anti_commutes;
                cnt1 ^= anti_commutes;
            });

    // Combine final anti-commutation phase tally (mod 4).
    uint8_t s = pop_count(cnt1);
    s ^= pop_count(cnt2) << 1;
    s ^= (uint8_t)rhs.bit_ptr_sign.get() << 1;
    return s & 3;
}

PauliStringVal PauliStringVal::random(size_t num_qubits) {
    auto result = PauliStringVal(num_qubits);
    result.x_data = std::move(aligned_bits256::random(num_qubits));
    result.z_data = std::move(aligned_bits256::random(num_qubits));
    result.val_sign ^= aligned_bits256::random(1).get_bit(0);
    return result;
}

bool PauliStringPtr::commutes(const PauliStringPtr& other) const noexcept {
    assert(size == other.size);
    __m256i cnt1 {};
    simd_for_each_4(
            (__m256i *)_x,
            (__m256i *)_z,
            (__m256i *)other._x,
            (__m256i *)other._z,
            num_words256(),
            [&cnt1](auto x1, auto z1, auto x2, auto z2) {
        cnt1 ^= (*x1 & *z2) ^ (*x2 & *z1);
    });
    return (pop_count(cnt1) & 1) == 0;
}

bool PauliStringPtr::get_x_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    return (_x[i0] >> i1) & 1;
}

bool PauliStringPtr::get_z_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    return (_z[i0] >> i1) & 1;
}

bool PauliStringPtr::get_xz_bit(size_t k) const {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    return ((_x[i0] ^ _z[i0]) >> i1) & 1;
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

void PauliStringPtr::toggle_x_bit_if(size_t k, bool condition) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _x[i0] ^= (size_t)condition << i1;
}

void PauliStringPtr::toggle_xz_bit_if(size_t k, bool condition) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _x[i0] ^= (size_t)condition << i1;
    _z[i0] ^= (size_t)condition << i1;
}

void PauliStringPtr::toggle_z_bit_if(size_t k, bool condition) {
    size_t i0 = k >> 6;
    size_t i1 = k & 63;
    _z[i0] ^= (size_t)condition << i1;
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
    out.bit_ptr_sign.toggle_if(bit_ptr_sign.get());
}

bool PauliStringVal::operator==(const PauliStringPtr &other) const {
    return ptr() == other;
}
bool PauliStringVal::operator!=(const PauliStringPtr &other) const {
    return ptr() != other;
}
