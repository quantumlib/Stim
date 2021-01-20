#ifndef TABLEAU_H
#define TABLEAU_H

#include <iostream>
#include <unordered_map>
#include <immintrin.h>
#include "simd/simd_bit_table.h"
#include "simd/simd_util.h"
#include "pauli_string.h"

struct TableauHalf {
    size_t num_qubits;
    simd_bit_table xt;
    simd_bit_table zt;
    simd_bits signs;
    PauliStringRef operator[](size_t input_qubit);
    const PauliStringRef operator[](size_t input_qubit) const;
    TableauHalf(size_t num_qubits);
};

/// A Tableau is a stabilizer tableau representation of a Clifford operation.
/// It stores, for each X and Z observable for each qubit, what is produced when
/// conjugating that observable by the operation. In other words, it explains how
/// to transform "input side" Pauli products into "output side" Pauli products.
///
/// The memory layout used by this class is column major, meaning iterating over
/// the output observable is iterating along the grain of memory. This makes
/// prepending operations cheap. To append operations, use TempTransposedTableauRaii.
struct Tableau {
    size_t num_qubits;
    TableauHalf xs;
    TableauHalf zs;

    explicit Tableau(size_t num_qubits);
    bool operator==(const Tableau &other) const;
    bool operator!=(const Tableau &other) const;

    PauliStringVal eval_y_obs(size_t qubit) const;

    std::string str() const;
    void expand(size_t new_num_qubits);

    /// Creates a Tableau representing the identity operation.
    static Tableau identity(size_t num_qubits);
    static Tableau random(size_t num_qubits, std::mt19937 &rng);
    Tableau inverse() const;

    bool satisfies_invariants() const;

    /// Creates a Tableau representing a single qubit gate.
    ///
    /// All observables specified using the string format accepted by `PauliStringVal::from_str`.
    /// For example: "-X" or "+Y".
    ///
    /// Args:
    ///    x: The output-side observable that the input-side X observable gets mapped to.
    ///    z: The output-side observable that the input-side Y observable gets mapped to.
    static Tableau gate1(const char *x,
                         const char *z);

    /// Creates a Tableau representing a two qubit gate.
    ///
    /// All observables specified using the string format accepted by `PauliStringVal::from_str`.
    /// For example: "-IX" or "+YZ".
    ///
    /// Args:
    ///    x1: The output-side observable that the input-side XI observable gets mapped to.
    ///    z1: The output-side observable that the input-side YI observable gets mapped to.
    ///    x2: The output-side observable that the input-side IX observable gets mapped to.
    ///    z2: The output-side observable that the input-side IY observable gets mapped to.
    static Tableau gate2(const char *x1,
                         const char *z1,
                         const char *x2,
                         const char *z2);

    /// Returns the result of applying the tableau to the given Pauli string.
    ///
    /// Args:
    ///     p: The input-side Pauli string.
    ///
    /// Returns:
    ///     The output-side Pauli string.
    ///     Algebraically: $c p c^{-1}$ where $c$ is the tableau's Clifford operation.
    PauliStringVal operator()(const PauliStringRef &p) const;

    /// Returns the result of applying the tableau to `gathered_input.scatter(scattered_indices)`.
    PauliStringVal scatter_eval(const PauliStringRef &gathered_input, const std::vector<size_t> &scattered_indices) const;

    /// Applies the Tableau inplace to a subset of a Pauli string.
    void apply_within(PauliStringRef &target, const std::vector<size_t> &target_qubits) const;

    /// Appends a smaller operation into this tableau's operation.
    ///
    /// The new value T' of this tableau will equal the composition T o P = PT where T is the old
    /// value of this tableau and P is the operation to append.
    ///
    /// Args:
    ///     operation: The smaller operation to append into this tableau.
    ///     target_qubits: The qubits being acted on by `operation`.
    void inplace_scatter_append(const Tableau &operation, const std::vector<size_t> &target_qubits);

    /// Prepends a smaller operation into this tableau's operation.
    ///
    /// The new value T' of this tableau will equal the composition P o T = TP where T is the old
    /// value of this tableau and P is the operation to append.
    ///
    /// Args:
    ///     operation: The smaller operation to prepend into this tableau.
    ///     target_qubits: The qubits being acted on by `operation`.
    void inplace_scatter_prepend(const Tableau &operation, const std::vector<size_t> &target_qubits);

    void prepend_SWAP(size_t q1, size_t q2);
    void prepend_X(size_t q);
    void prepend_Y(size_t q);
    void prepend_Z(size_t q);
    void prepend_H(size_t q);
    void prepend_H_YZ(size_t q);
    void prepend_H_XY(size_t q);
    void prepend_SQRT_X(size_t q);
    void prepend_SQRT_X_DAG(size_t q);
    void prepend_SQRT_Y(size_t q);
    void prepend_SQRT_Y_DAG(size_t q);
    void prepend_SQRT_Z(size_t q);
    void prepend_SQRT_Z_DAG(size_t q);
    void prepend_CX(size_t control, size_t target);
    void prepend_CY(size_t control, size_t target);
    void prepend_CZ(size_t control, size_t target);
    void prepend_ISWAP(size_t q1, size_t q2);
    void prepend_ISWAP_DAG(size_t q1, size_t q2);
    void prepend_XCX(size_t control, size_t target);
    void prepend_XCY(size_t control, size_t target);
    void prepend_XCZ(size_t control, size_t target);
    void prepend_YCX(size_t control, size_t target);
    void prepend_YCY(size_t control, size_t target);
    void prepend_YCZ(size_t control, size_t target);
    void prepend(const PauliStringRef &op);
    void prepend(const SparsePauliString &op);

    bool z_sign(size_t a) const;
};

struct TransposedPauliStringPtr {
    __m256i *x;
    __m256i *z;
    __m256i *s;
};

struct TransposedTableauXZ {
    TransposedPauliStringPtr xz[2];
};

size_t bit_address(size_t input_qubit, size_t output_qubit, size_t num_qubits, bool transposed);

std::ostream &operator<<(std::ostream &out, const Tableau &ps);

/// Tableaus for common gates, keyed by name.
extern const std::unordered_map<std::string, const Tableau> GATE_TABLEAUS;
extern const std::unordered_map<std::string, const std::string> GATE_INVERSE_NAMES;

/// When this class is constructed, it transposes the tableau given to it.
/// The transpose is undone on deconstruction.
///
/// This is useful when appending operations to the tableau, since otherwise
/// the append would be working against the grain of memory.
struct TempTransposedTableauRaii {
    Tableau &tableau;

    explicit TempTransposedTableauRaii(Tableau &tableau);
    ~TempTransposedTableauRaii();

    TempTransposedTableauRaii() = delete;
    TempTransposedTableauRaii(const TempTransposedTableauRaii &) = delete;
    TempTransposedTableauRaii(TempTransposedTableauRaii &&) = delete;

    TransposedTableauXZ transposed_xz_ptr(size_t qubit) const;

    bool z_sign(size_t a) const;
    bool x_obs_z_bit(size_t input_qubit, size_t output_qubit) const;
    bool z_obs_x_bit(size_t input_qubit, size_t output_qubit) const;
    bool z_obs_z_bit(size_t input_qubit, size_t output_qubit) const;

    void append_H(size_t q);
    void append_H_XY(size_t q);
    void append_H_YZ(size_t q);
    void append_CX(size_t control, size_t target);
    void append_CY(size_t control, size_t target);
    void append_CZ(size_t control, size_t target);
    void append_X(size_t q);
    void append_SWAP(size_t q1, size_t q2);
};

#endif