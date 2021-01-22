#ifndef PAULI_STRING_H
#define PAULI_STRING_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "../simd/simd_bits.h"
#include "../simd/simd_bits_range_ref.h"
#include "../simd/bit_ref.h"
#include "../simd/simd_util.h"
#include "pauli_string_ref.h"

struct PauliString {
    size_t num_qubits;
    bool val_sign;
    simd_bits x_data;
    simd_bits z_data;

    explicit PauliString(size_t num_qubits);
    static PauliString identity(size_t num_qubits);
    static PauliString from_pattern(bool sign, size_t num_qubits, const std::function<char(size_t)> &func);
    static PauliString from_str(const char *text);

    PauliString(const PauliStringRef &other); // NOLINT(google-explicit-constructor)
    PauliString& operator=(const PauliStringRef &other) noexcept;
    static PauliString random(size_t num_qubits, std::mt19937_64 &rng);
    operator const PauliStringRef() const;
    operator PauliStringRef();

    bool operator==(const PauliStringRef &other) const;
    bool operator!=(const PauliStringRef &other) const;

    const PauliStringRef ref() const;
    PauliStringRef ref();

    std::string str() const;
};

std::ostream &operator<<(std::ostream &out, const PauliString &ps);

#endif
