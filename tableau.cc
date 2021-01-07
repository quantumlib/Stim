#include <iostream>
#include <map>
#include "pauli_string.h"
#include "tableau.h"

#define BLOCK_TRANSPOSED_PAULI_STRING_STRIDE 256

size_t table_row_length_bits(size_t num_qubits) {
    return ceil256(num_qubits);
}

size_t table_quadrant_bits(size_t num_qubits) {
    auto diam = table_row_length_bits(num_qubits);
    return diam * diam;
}


BlockTransposedTableau::BlockTransposedTableau(Tableau &tableau) : tableau(tableau) {
    blockwise_transpose();
}

BlockTransposedTableau::~BlockTransposedTableau() {
    blockwise_transpose();
}

void BlockTransposedTableau::blockwise_transpose() {
    size_t n = ceil256(tableau.num_qubits);
    transpose_bit_matrix_256x256blocks(tableau.data_x2x.u64, n);
    transpose_bit_matrix_256x256blocks(tableau.data_x2z.u64, n);
    transpose_bit_matrix_256x256blocks(tableau.data_z2x.u64, n);
    transpose_bit_matrix_256x256blocks(tableau.data_z2z.u64, n);
}

TransposedPauliStringPtr BlockTransposedTableau::transposed_col_x_obs_ptr(size_t qubit) const {
    size_t col_start = ((table_row_length_bits(tableau.num_qubits) >> 8) * (qubit & ~0xFF)) | (qubit & 0xFF);
    return TransposedPauliStringPtr {
        &tableau.data_x2x.u256[col_start],
        &tableau.data_x2z.u256[col_start],
    };
}

TransposedPauliStringPtr BlockTransposedTableau::transposed_col_z_obs_ptr(size_t qubit) const {
    size_t col_start = ((table_row_length_bits(tableau.num_qubits) >> 8) * (qubit & ~0xFF)) | (qubit & 0xFF);
    return TransposedPauliStringPtr {
        &tableau.data_z2x.u256[col_start],
        &tableau.data_z2z.u256[col_start],
    };
}

void BlockTransposedTableau::append_CX(size_t control, size_t target) {
    size_t words256 = ceil256(tableau.num_qubits) >> 8;
    for (size_t t = 0; t < 2; t++) {
        auto pc = t == 0 ? transposed_col_x_obs_ptr(control) : transposed_col_z_obs_ptr(control);
        auto pt = t == 0 ? transposed_col_x_obs_ptr(target) : transposed_col_z_obs_ptr(target);
        auto s256 = t == 0 ? tableau.data_sign_x.u256 : tableau.data_sign_z.u256;
        auto end = &pc._x[words256 * BLOCK_TRANSPOSED_PAULI_STRING_STRIDE];
        while (pc._x != end) {
            *s256 ^= _mm256_andnot_si256(*pc._z ^ *pt._x, *pc._x & *pt._z);
            *pc._z ^= *pt._z;
            *pt._x ^= *pc._x;
            pc._x += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            pc._z += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            pt._x += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            pt._z += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            s256 += 1;
        }
    }
}

void BlockTransposedTableau::append_H_YZ(size_t target) {
    size_t words256 = ceil256(tableau.num_qubits) >> 8;
    for (size_t t = 0; t < 2; t++) {
        auto p = t == 0 ? transposed_col_x_obs_ptr(target) : transposed_col_z_obs_ptr(target);
        auto s256 = t == 0 ? tableau.data_sign_x.u256 : tableau.data_sign_z.u256;
        auto end = &p._x[words256 * BLOCK_TRANSPOSED_PAULI_STRING_STRIDE];
        while (p._x != end) {
            *s256 ^= _mm256_andnot_si256(*p._z, *p._x);
            *p._x ^= *p._z;
            p._x += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            p._z += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            s256 += 1;
        }
    }
}

void BlockTransposedTableau::append_H(size_t target) {
    size_t words256 = ceil256(tableau.num_qubits) >> 8;
    for (size_t t = 0; t < 2; t++) {
        auto p = t == 0 ? transposed_col_x_obs_ptr(target) : transposed_col_z_obs_ptr(target);
        auto s256 = t == 0 ? tableau.data_sign_x.u256 : tableau.data_sign_z.u256;
        auto end = &p._x[words256 * BLOCK_TRANSPOSED_PAULI_STRING_STRIDE];
        while (p._x != end) {
            std::swap(*p._x, *p._z);
            *s256 ^= *p._x & *p._z;
            p._x += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            p._z += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            s256 += 1;
        }
    }
}

void BlockTransposedTableau::append_X(size_t target) {
    size_t words256 = ceil256(tableau.num_qubits) >> 8;
    for (size_t t = 0; t < 2; t++) {
        auto p = t == 0 ? transposed_col_x_obs_ptr(target) : transposed_col_z_obs_ptr(target);
        auto s256 = t == 0 ? tableau.data_sign_x.u256 : tableau.data_sign_z.u256;
        auto end = &p._z[words256 * BLOCK_TRANSPOSED_PAULI_STRING_STRIDE];
        while (p._z != end) {
            *s256 ^= *p._z;
            p._z += BLOCK_TRANSPOSED_PAULI_STRING_STRIDE;
            s256 += 1;
        }
    }
}

PauliStringPtr Tableau::x_obs_ptr(size_t qubit) const {
    auto stride256 = table_row_length_bits(num_qubits);
    return PauliStringPtr(
            num_qubits,
            BitPtr(data_sign_x.u64, qubit),
            &data_x2x.u64[4*qubit],
            &data_x2z.u64[4*qubit],
            stride256);
}

PauliStringPtr Tableau::z_obs_ptr(size_t qubit) const {
    auto stride256 = table_row_length_bits(num_qubits);
    return PauliStringPtr(
            num_qubits,
            BitPtr(data_sign_z.u64, qubit),
            &data_z2x.u64[4*qubit],
            &data_z2z.u64[4*qubit],
            stride256);
}

bool Tableau::z_sign(size_t a) const {
    return data_sign_z.get_bit(a);
}

bool BlockTransposedTableau::z_sign(size_t a) const {
    return tableau.data_sign_z.get_bit(a);
}

bool BlockTransposedTableau::z_obs_x_bit(size_t a, size_t b) const {
    size_t col_start = ((table_row_length_bits(tableau.num_qubits) >> 8) * (b & ~0xFF)) | (b & 0xFF);
    a = ((a & ~0xFF)*BLOCK_TRANSPOSED_PAULI_STRING_STRIDE) | (a & 0xFF);
    a += col_start << 8;
    return tableau.data_z2x.get_bit(a);
}

bool BlockTransposedTableau::z_obs_z_bit(size_t a, size_t b) const {
    size_t col_start = ((table_row_length_bits(tableau.num_qubits) >> 8) * (b & ~0xFF)) | (b & 0xFF);
    a = ((a & ~0xFF)*BLOCK_TRANSPOSED_PAULI_STRING_STRIDE) | (a & 0xFF);
    a += col_start << 8;
    return tableau.data_z2z.get_bit(a);
}

PauliStringVal Tableau::eval_y_obs(size_t qubit) const {
    PauliStringVal result(x_obs_ptr(qubit));
    uint8_t log_i = result.ptr().inplace_right_mul_returning_log_i_scalar(z_obs_ptr(qubit));
    log_i++;
    assert((log_i & 1) == 0);
    if (log_i & 2) {
        result.val_sign ^= true;
    }
    return result;
}

Tableau::Tableau(size_t num_qubits) :
        num_qubits(num_qubits),
        data_x2x(table_quadrant_bits(num_qubits)),
        data_x2z(table_quadrant_bits(num_qubits)),
        data_z2x(table_quadrant_bits(num_qubits)),
        data_z2z(table_quadrant_bits(num_qubits)),
        data_sign_x(table_row_length_bits(num_qubits)),
        data_sign_z(table_row_length_bits(num_qubits)){
    for (size_t q = 0; q < num_qubits; q++) {
        x_obs_ptr(q).set_x_bit(q, true);
        z_obs_ptr(q).set_z_bit(q, true);
    }
}

Tableau Tableau::identity(size_t num_qubits) {
    return Tableau(num_qubits);
}

Tableau Tableau::gate1(const char *x, const char *z) {
    Tableau result(1);
    result.x_obs_ptr(0).overwrite_with(PauliStringVal::from_str(x));
    result.z_obs_ptr(0).overwrite_with(PauliStringVal::from_str(z));
    return result;
}

Tableau Tableau::gate2(const char *x1,
                       const char *z1,
                       const char *x2,
                       const char *z2) {
    Tableau result(2);
    result.x_obs_ptr(0).overwrite_with(PauliStringVal::from_str(x1));
    result.z_obs_ptr(0).overwrite_with(PauliStringVal::from_str(z1));
    result.x_obs_ptr(1).overwrite_with(PauliStringVal::from_str(x2));
    result.z_obs_ptr(1).overwrite_with(PauliStringVal::from_str(z2));
    return result;
}

std::ostream &operator<<(std::ostream &out, const Tableau &t) {
    out << "Tableau {\n";
    for (size_t i = 0; i < t.num_qubits; i++) {
        out << "  qubit " << i << "_x: " << t.x_obs_ptr(i) << "\n";
        out << "  qubit " << i << "_z: " << t.z_obs_ptr(i) << "\n";
    }
    out << "}";
    return out;
}

std::string Tableau::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

void Tableau::inplace_scatter_append(const Tableau &operation, const std::vector<size_t> &target_qubits) {
    assert(operation.num_qubits == target_qubits.size());
    for (size_t q = 0; q < num_qubits; q++) {
        auto x = x_obs_ptr(q);
        auto z = z_obs_ptr(q);
        operation.apply_within(x, target_qubits);
        operation.apply_within(z, target_qubits);
    }
}

bool Tableau::operator==(const Tableau &other) const {
    return num_qubits == other.num_qubits
        && data_x2x == other.data_x2x
        && data_x2z == other.data_x2z
        && data_z2x == other.data_z2x
        && data_z2z == other.data_z2z
        && data_sign_x == other.data_sign_x
        && data_sign_z == other.data_sign_z;
}

bool Tableau::operator!=(const Tableau &other) const {
    return !(*this == other);
}

void Tableau::inplace_scatter_prepend(const Tableau &operation, const std::vector<size_t> &target_qubits) {
    assert(operation.num_qubits == target_qubits.size());
    std::vector<PauliStringVal> new_x;
    std::vector<PauliStringVal> new_z;
    new_x.reserve(operation.num_qubits);
    new_z.reserve(operation.num_qubits);
    for (size_t q = 0; q < operation.num_qubits; q++) {
        new_x.emplace_back(std::move(scatter_eval(operation.x_obs_ptr(q), target_qubits)));
        new_z.emplace_back(std::move(scatter_eval(operation.z_obs_ptr(q), target_qubits)));
    }
    for (size_t q = 0; q < operation.num_qubits; q++) {
        x_obs_ptr(target_qubits[q]).overwrite_with(new_x[q]);
        z_obs_ptr(target_qubits[q]).overwrite_with(new_z[q]);
    }
}

void Tableau::prepend_SQRT_X(size_t q) {
    auto z = z_obs_ptr(q);
    uint8_t m = 1 + z.inplace_right_mul_returning_log_i_scalar(x_obs_ptr(q));
    if (m & 2) {
        z.bit_ptr_sign.toggle();
    }
}

void Tableau::prepend_SQRT_X_DAG(size_t q) {
    auto z = z_obs_ptr(q);
    uint8_t m = 3 + z.inplace_right_mul_returning_log_i_scalar(x_obs_ptr(q));
    if (m & 2) {
        z.bit_ptr_sign.toggle();
    }
}

void Tableau::prepend_SQRT_Y(size_t q) {
    auto z = z_obs_ptr(q);
    z.bit_ptr_sign.toggle();
    x_obs_ptr(q).swap_with(z);
}

void Tableau::prepend_SQRT_Y_DAG(size_t q) {
    auto z = z_obs_ptr(q);
    x_obs_ptr(q).swap_with(z);
    z.bit_ptr_sign.toggle();
}

void Tableau::prepend_SQRT_Z(size_t q) {
    auto x = x_obs_ptr(q);
    uint8_t m = 1 + x.inplace_right_mul_returning_log_i_scalar(z_obs_ptr(q));
    if (m & 2) {
        x.bit_ptr_sign.toggle();
    }
}

void Tableau::prepend_SQRT_Z_DAG(size_t q) {
    auto x = x_obs_ptr(q);
    uint8_t m = 3 + x.inplace_right_mul_returning_log_i_scalar(z_obs_ptr(q));
    if (m & 2) {
        x.bit_ptr_sign.toggle();
    }
}

void Tableau::prepend_CX(size_t control, size_t target) {
    z_obs_ptr(target) *= z_obs_ptr(control);
    x_obs_ptr(control) *= x_obs_ptr(target);
}

void Tableau::prepend_CY(size_t control, size_t target) {
    prepend_H_YZ(target);
    prepend_CZ(control, target);
    prepend_H_YZ(target);
}

void Tableau::prepend_CZ(size_t control, size_t target) {
    x_obs_ptr(target) *= z_obs_ptr(control);
    x_obs_ptr(control) *= z_obs_ptr(target);
}

void Tableau::prepend_H(const size_t q) {
    auto z = z_obs_ptr(q);
    x_obs_ptr(q).swap_with(z);
}

void Tableau::prepend_H_YZ(const size_t q) {
    auto x = x_obs_ptr(q);
    auto z = z_obs_ptr(q);
    uint8_t m = 3 + z.inplace_right_mul_returning_log_i_scalar(x);
    x.bit_ptr_sign.toggle();
    if (m & 2) {
        z.bit_ptr_sign.toggle();
    }
}

void Tableau::prepend_H_XY(const size_t q) {
    auto x = x_obs_ptr(q);
    auto z = z_obs_ptr(q);
    uint8_t m = 1 + x.inplace_right_mul_returning_log_i_scalar(z);
    z.bit_ptr_sign.toggle();
    if (m & 2) {
        x.bit_ptr_sign.toggle();
    }
}

void Tableau::prepend_X(size_t q) {
    z_obs_ptr(q).bit_ptr_sign.toggle();
}

void Tableau::prepend_Y(size_t q) {
    x_obs_ptr(q).bit_ptr_sign.toggle();
    z_obs_ptr(q).bit_ptr_sign.toggle();
}

void Tableau::prepend_Z(size_t q) {
    x_obs_ptr(q).bit_ptr_sign.toggle();
}

PauliStringVal Tableau::scatter_eval(const PauliStringPtr &gathered_input, const std::vector<size_t> &scattered_indices) const {
    assert(gathered_input.size == scattered_indices.size());
    auto result = PauliStringVal::identity(num_qubits);
    result.val_sign = gathered_input.bit_ptr_sign.get();
    for (size_t k_gathered = 0; k_gathered < gathered_input.size; k_gathered++) {
        size_t k_scattered = scattered_indices[k_gathered];
        auto x = gathered_input.get_x_bit(k_gathered);
        auto z = gathered_input.get_z_bit(k_gathered);
        if (x) {
            if (z) {
                // Multiply by Y using Y = i*X*Z.
                uint8_t log_i = 1;
                log_i += result.ptr().inplace_right_mul_returning_log_i_scalar(x_obs_ptr(k_scattered));
                log_i += result.ptr().inplace_right_mul_returning_log_i_scalar(z_obs_ptr(k_scattered));
                assert((log_i & 1) == 0);
                result.val_sign ^= (log_i & 2) != 0;
            } else {
                result.ptr() *= x_obs_ptr(k_scattered);
            }
        } else if (z) {
            result.ptr() *= z_obs_ptr(k_scattered);
        }
    }
    return result;
}

PauliStringVal Tableau::operator()(const PauliStringPtr &p) const {
    assert(p.size == num_qubits);
    std::vector<size_t> indices;
    for (size_t k = 0; k < p.size; k++) {
        indices.push_back(k);
    }
    return scatter_eval(p, indices);
}

void Tableau::apply_within(PauliStringPtr &target, const std::vector<size_t> &target_qubits) const {
    assert(num_qubits == target_qubits.size());
    auto inp = PauliStringVal::identity(num_qubits);
    PauliStringPtr inp_ptr = inp;
    target.gather_into(inp_ptr, target_qubits);
    auto out = (*this)(inp);
    out.ptr().scatter_into(target, target_qubits);
}

const std::unordered_map<std::string, const Tableau> GATE_TABLEAUS {
    {"I", Tableau::gate1("+X", "+Z")},
    // Pauli gates.
    {"X", Tableau::gate1("+X", "-Z")},
    {"Y", Tableau::gate1("-X", "-Z")},
    {"Z", Tableau::gate1("-X", "+Z")},
    // Axis exchange gates.
    {"H", Tableau::gate1("+Z", "+X")},
    {"H_XY", Tableau::gate1("+Y", "-Z")},
    {"H_XZ", Tableau::gate1("+Z", "+X")},
    {"H_YZ", Tableau::gate1("-X", "+Y")},
    // 90 degree rotation gates.
    {"SQRT_X", Tableau::gate1("+X", "-Y")},
    {"SQRT_X_DAG", Tableau::gate1("+X", "+Y")},
    {"SQRT_Y", Tableau::gate1("-Z", "+X")},
    {"SQRT_Y_DAG", Tableau::gate1("+Z", "-X")},
    {"SQRT_Z", Tableau::gate1("+Y", "+Z")},
    {"SQRT_Z_DAG", Tableau::gate1("-Y", "+Z")},
    {"S", Tableau::gate1("+Y", "+Z")},
    {"S_DAG", Tableau::gate1("-Y", "+Z")},
    // Two qubit gates.
    {"CNOT", Tableau::gate2("+XX", "+ZI", "+IX", "+ZZ")},
    {"CX", Tableau::gate2("+XX", "+ZI", "+IX", "+ZZ")},
    {"CY", Tableau::gate2("+XY", "+ZI", "+ZX", "+ZZ")},
    {"CZ", Tableau::gate2("+XZ", "+ZI", "+ZX", "+IZ")},
    {"SWAP", Tableau::gate2("+IX", "+IZ", "+XI", "+ZI")},
};
