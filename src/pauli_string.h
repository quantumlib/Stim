#ifndef PAULI_STRING_H
#define PAULI_STRING_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "simd/simd_bits.h"
#include "simd/simd_range.h"
#include "simd/bit_ptr.h"
#include "simd/simd_util.h"

struct PauliStringVal;
struct PauliStringRef;
struct SparsePauliWord;
struct SparsePauliString;

struct PauliStringRef {
    size_t num_qubits;
    bit_ref sign_ref;
    simd_range_ref x_ref;
    simd_range_ref z_ref;

    PauliStringRef(size_t num_qubits, bit_ref sign_ref, simd_range_ref x_ref, simd_range_ref z_ref);
    PauliStringRef &operator=(const PauliStringRef &other);

    bool operator==(const PauliStringRef &other) const;
    bool operator!=(const PauliStringRef &other) const;

    void swap_with(PauliStringRef other);

    void gather_into(PauliStringRef out, const std::vector<size_t> &in_indices) const;
    void scatter_into(PauliStringRef out, const std::vector<size_t> &out_indices) const;

    SparsePauliString sparse() const;
    std::string str() const;

    // Multiplies a commuting Pauli string into this one.
    //
    // ASSERTS:
    //     The given Pauli strings have the same size.
    //     The given Pauli strings commute.
    PauliStringRef& operator*=(const PauliStringRef &commuting_rhs);

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
    uint8_t inplace_right_mul_returning_log_i_scalar(const PauliStringRef& rhs) noexcept;

    size_t num_words256() const;

    bool commutes(const PauliStringRef& other) const noexcept;
};

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

struct PauliStringVal {
    bool val_sign;
    simd_bits x_data;
    simd_bits z_data;

    explicit PauliStringVal(size_t num_qubits);
    PauliStringVal(const PauliStringRef &other); // NOLINT(google-explicit-constructor)
    PauliStringVal& operator=(const PauliStringRef &other) noexcept;
    static PauliStringVal random(size_t num_qubits);
    operator const PauliStringRef() const;
    operator PauliStringRef();

    bool operator==(const PauliStringRef &other) const;
    bool operator!=(const PauliStringRef &other) const;

    static PauliStringVal from_pattern(bool sign, size_t num_qubits, const std::function<char(size_t)> &func);
    static PauliStringVal from_str(const char *text);
    static PauliStringVal identity(size_t num_qubits);
    const PauliStringRef ref() const;
    PauliStringRef ref();

    std::string str() const;
};

std::ostream &operator<<(std::ostream &out, const SparsePauliString &ps);
std::ostream &operator<<(std::ostream &out, const PauliStringRef &ps);
std::ostream &operator<<(std::ostream &out, const PauliStringVal &ps);

extern const std::unordered_map<std::string, std::function<void(PauliStringRef &, size_t)>> SINGLE_QUBIT_GATE_UNSIGNED_CONJ_FUNCS;
extern const std::unordered_map<std::string, std::function<void(PauliStringRef &, size_t, size_t)>> TWO_QUBIT_GATE_UNSIGNED_CONJ_FUNCS;
extern const std::unordered_map<std::string, std::function<void(PauliStringRef &, const std::vector<size_t> &)>> BROADCAST_GATE_UNSIGNED_CONJ_FUNCS;

#endif
