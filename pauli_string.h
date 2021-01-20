#ifndef PAULI_STRING_H
#define PAULI_STRING_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "aligned_bits256.h"
#include "bit_ptr.h"
#include "simd_util.h"

struct PauliStringVal;

struct SparsePauliWord {
    // Bit index divided by 64, giving the uint64_t index.
    size_t index64;
    // X flip bits.
    uint64_t wx;
    // Z flip bits.
    uint64_t wz;
};

struct SparsePauliString {
    bool sign = false;
    std::vector<SparsePauliWord> indexed_words;
    std::string str() const;
};

struct PauliStringPtr {
    size_t size;
    BitPtr bit_ptr_sign;
    uint64_t *_x;
    uint64_t *_z;

    SimdRange x_rng();
    SimdRange z_rng();

    PauliStringPtr(size_t size, BitPtr bit_ptr_sign, uint64_t *x, uint64_t *z);
    PauliStringPtr(const PauliStringVal &other); // NOLINT(google-explicit-constructor)

    bool operator==(const PauliStringPtr &other) const;
    bool operator!=(const PauliStringPtr &other) const;

    void overwrite_with(const PauliStringPtr &other);
    void swap_with(PauliStringPtr &other);

    void gather_into(PauliStringPtr &out, const std::vector<size_t> &in_indices) const;
    void scatter_into(PauliStringPtr &out, const std::vector<size_t> &out_indices) const;

    SparsePauliString sparse() const;
    std::string str() const;

    // Multiplies a commuting Pauli string into this one.
    //
    // ASSERTS:
    //     The given Pauli strings have the same size.
    //     The given Pauli strings commute.
    PauliStringPtr& operator*=(const PauliStringPtr &commuting_rhs);

    // A more general version  of `*this *= rhs` which works for anti-commuting Paulis.
    //
    // Instead of updating the sign of `*this`, the base i logarithm of a scalar factor that still needs to be included
    // into the result is returned. For example, when multiplying XZ to get iY, the left hand side would become `Y`
    // and the returned value would be `1` (meaning a factor of `i**1 = i` is missing from the `Y`).
    //
    // Returns:
    //     The logarithm, base i, of a scalar byproduct from the multiplication.
    //     0 if the scalar byproduct is 1.
    //     1 if the scalar byproduct is i.
    //     2 if the scalar byproduct is -1.
    //     3 if the scalar byproduct is -i.
    //
    // ASSERTS:
    //     The given Pauli strings have the same size.
    uint8_t inplace_right_mul_returning_log_i_scalar(const PauliStringPtr& rhs) noexcept;

    size_t num_words256() const;

    bool commutes(const PauliStringPtr& other) const noexcept;
    bool get_x_bit(size_t k) const;
    bool get_z_bit(size_t k) const;
    bool get_xz_bit(size_t k) const;
    void set_x_bit(size_t k, bool b);
    void set_z_bit(size_t k, bool b);
    void toggle_x_bit(size_t k);
    void toggle_z_bit(size_t k);
    void toggle_x_bit_if(size_t k, bool condition);
    void toggle_xz_bit_if(size_t k, bool condition);
    void toggle_z_bit_if(size_t k, bool condition);
};

struct PauliStringVal {
    bool val_sign;
    aligned_bits256 x_data;
    aligned_bits256 z_data;

    explicit PauliStringVal(size_t size);
    PauliStringVal(const PauliStringPtr &other); // NOLINT(google-explicit-constructor)
    PauliStringVal& operator=(const PauliStringPtr &other) noexcept;
    static PauliStringVal random(size_t num_qubits);

    bool operator==(const PauliStringPtr &other) const;
    bool operator!=(const PauliStringPtr &other) const;

    static PauliStringVal from_pattern(bool sign, size_t size, const std::function<char(size_t)> &func);
    static PauliStringVal from_str(const char *text);
    static PauliStringVal identity(size_t size);
    PauliStringPtr ptr() const;

    std::string str() const;
};

std::ostream &operator<<(std::ostream &out, const SparsePauliString &ps);
std::ostream &operator<<(std::ostream &out, const PauliStringPtr &ps);
std::ostream &operator<<(std::ostream &out, const PauliStringVal &ps);

extern const std::unordered_map<std::string, std::function<void(PauliStringPtr &, size_t)>> SINGLE_QUBIT_GATE_UNSIGNED_CONJ_FUNCS;
extern const std::unordered_map<std::string, std::function<void(PauliStringPtr &, size_t, size_t)>> TWO_QUBIT_GATE_UNSIGNED_CONJ_FUNCS;
extern const std::unordered_map<std::string, std::function<void(PauliStringPtr &, const std::vector<size_t> &)>> BROADCAST_GATE_UNSIGNED_CONJ_FUNCS;

#endif
