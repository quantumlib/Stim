#ifndef TABLEAU_H
#define TABLEAU_H

#include <iostream>
#include <unordered_map>
#include <immintrin.h>
#include "simd_util.h"
#include "pauli_string.h"

/// Layout while not transposed.
/// Let M = ceil256(num_qubits)
///
/// A row starts every bit with a 256*M bit gap every 256'th bit.
/// Row word-to-word stride is 256*256 bits long.
/// A column starts every 256 bits.
/// Column word-to-word stride is M*256*256 bits long.
///
/// X-to-X quadrant (size M x M where M = ceil256(qubits))
///
///         256*256 bit row stride
///  *-------------*
///  ___________   ___________       ___________
/// |____00_____| |__1_00_____|     |__M-1+00___| *
/// |____01_____| |__1_01_____|     |__M-1+01___| |
/// |____02_____| |__1_02_____|     |__M-1+02___| |
/// |____03_____| |__1_03_____| ... |__M-1+03___| | M*256 bit col stride
/// |____.______| |____.______|     |____.______| |
/// |____.______| |____.______|     |____.______| |
/// |____FF_____| |__1_FF_____|     |__M-1+FF___| |
///  ___________   ___________       ___________  |
/// |__M+00_____| |__M+100____|     |_2M-1+00___| *
/// |__M+01_____| |__M+101____|     |_2M-1+01___|
/// |__M+02_____| |__M+102____|     |_2M-1+02___|
/// |__M+03_____| |__M+103____| ... |_2M-1+03___|
/// |____.______| |____.______|     |____.______|
/// |____.______| |____.______|     |____.______|
/// |__M+FF_____| |__M+1FF____|     |_2M-1+FF___|
/// .
/// .
/// .
/// Z-to-X quadrant
/// X-to-Z quadrant
/// Z-to-Z quadrant
///
///
/// Layout while transposed.
/// Let M = ceil256(num_qubits)
///
/// A row starts every bit with a 256*M bit gap every 256'th bit.
/// Row word-to-word stride is 256*256 bits long.
/// A column starts every 256 bits.
/// Column word-to-word stride is M*256*256 bits long.
///
/// X-to-X quadrant (size M x M where M = ceil256(qubits))
///
///         256*256 bit row stride
///  *-------------*
///  __ __ __ __   __ __ __ __       __ __ __ __
/// |  |  |  |  | |  |  |  |  |     |  |  |  |  | *
/// |  |  |  |  | |  |  |  |  |     |  |  |  |  | |
/// |  |  |  |  | |1 |1 |  |1 |     |M-|M-|  |M-| |
/// |00|01|..|FF| |00|01|..|FF| ... |00|01|..|FF| | M*256 bit col stride
/// |  |  |  |  | |  |  |  |  |     |  |  |  |  | |
/// |  |  |  |  | |  |  |  |  |     |  |  |  |  | |
/// |__|__|__|__| |__|__|__|__|     |__|__|__|__| |
///  __ __ __ __   __ __ __ __       __ __ __ __  |
/// |  |  |  |  | |  |  |  |  |     |  |  |  |  | *
/// |  |  |  |  | |  |  |  |  |     |  |  |  |  |
/// |M |M |  |M | |M+|M+|M+|M+|     |2M|2M|  |2M|
/// |00|01|..|FF| |00|01|..|FF| ... |00|01|..|FF|
/// |  |  |  |  | |  |  |  |  |     |  |  |  |  |
/// |  |  |  |  | |  |  |  |  |     |  |  |  |  |
/// |__|__|__|__| |__|__|__|__|     |__|__|__|__|
/// .
/// .
/// .
/// Z-to-X quadrant
/// X-to-Z quadrant
/// Z-to-Z quadrant
struct Tableau {
    size_t num_qubits;
    aligned_bits256 data_x2x_z2x_x2z_z2z;
    aligned_bits256 data_sign_x_z;

    explicit Tableau(size_t num_qubits);
    bool operator==(const Tableau &other) const;
    bool operator!=(const Tableau &other) const;

    PauliStringVal eval_y_obs(size_t qubit) const;
    PauliStringPtr x_obs_ptr(size_t qubit) const;
    PauliStringPtr z_obs_ptr(size_t qubit) const;

    std::string str() const;

    /// Creates a Tableau representing the identity operation.
    static Tableau identity(size_t num_qubits);
    static Tableau random(size_t num_qubits);

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
    PauliStringVal operator()(const PauliStringPtr &p) const;

    /// Returns the result of applying the tableau to `gathered_input.scatter(scattered_indices)`.
    PauliStringVal scatter_eval(const PauliStringPtr &gathered_input, const std::vector<size_t> &scattered_indices) const;

    /// Applies the Tableau inplace to a subset of a Pauli string.
    void apply_within(PauliStringPtr &target, const std::vector<size_t> &target_qubits) const;

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

    bool z_sign(size_t a) const;
};

struct TransposedPauliStringPtr {
    __m256i *x;
    __m256i *z;
};

size_t bit_address(size_t input_qubit, size_t output_qubit, size_t num_qubits, size_t quadrant, bool transposed);

std::ostream &operator<<(std::ostream &out, const Tableau &ps);

/// Tableaus for common gates, keyed by name.
extern const std::unordered_map<std::string, const Tableau> GATE_TABLEAUS;

struct BlockTransposedTableau {
    Tableau &tableau;

    explicit BlockTransposedTableau(Tableau &tableau);
    ~BlockTransposedTableau();

    BlockTransposedTableau() = delete;
    BlockTransposedTableau(const BlockTransposedTableau &) = delete;
    BlockTransposedTableau(BlockTransposedTableau &&) = delete;

    TransposedPauliStringPtr transposed_double_col_obs_ptr(size_t qubit) const;

    bool z_sign(size_t a) const;
    bool z_obs_x_bit(size_t input_qubit, size_t output_qubit) const;
    bool z_obs_z_bit(size_t input_qubit, size_t output_qubit) const;

    void append_H(size_t q);
    void append_H_YZ(size_t q);
    void append_CX(size_t control, size_t target);
    void append_X(size_t q);
private:
    void blockwise_transpose();
};

#endif