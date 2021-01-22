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

struct PauliStringVal {
    size_t num_qubits;
    bool val_sign;
    simd_bits x_data;
    simd_bits z_data;

    explicit PauliStringVal(size_t num_qubits);
    static PauliStringVal identity(size_t num_qubits);
    static PauliStringVal from_pattern(bool sign, size_t num_qubits, const std::function<char(size_t)> &func);
    static PauliStringVal from_str(const char *text);

    PauliStringVal(const PauliStringRef &other); // NOLINT(google-explicit-constructor)
    PauliStringVal& operator=(const PauliStringRef &other) noexcept;
    static PauliStringVal random(size_t num_qubits, std::mt19937_64 &rng);
    operator const PauliStringRef() const;
    operator PauliStringRef();

    bool operator==(const PauliStringRef &other) const;
    bool operator!=(const PauliStringRef &other) const;

    const PauliStringRef ref() const;
    PauliStringRef ref();

    std::string str() const;
};

std::ostream &operator<<(std::ostream &out, const PauliStringVal &ps);

#endif
