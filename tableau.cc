#include <iostream>
#include <map>
#include "pauli_string.h"
#include "tableau.h"

size_t table_row_length_bits(size_t num_qubits) {
    return ceil256(num_qubits);
}

size_t table_quadrant_bits(size_t num_qubits) {
    auto diam = table_row_length_bits(num_qubits);
    return diam * diam;
}

size_t row_stride256(size_t num_qubits) {
    return 256;
}

size_t column_stride256(size_t num_qubits) {
    return table_row_length_bits(num_qubits);
}

BlockTransposedTableau::BlockTransposedTableau(Tableau &tableau) : tableau(tableau) {
    blockwise_transpose();
}

BlockTransposedTableau::~BlockTransposedTableau() {
    blockwise_transpose();
}

void BlockTransposedTableau::blockwise_transpose() {
    transpose_bit_matrix_256x256blocks(tableau.data_x2x_z2x_x2z_z2z.u64, tableau.data_x2x_z2x_x2z_z2z.num_bits);
}

TransposedPauliStringPtr BlockTransposedTableau::transposed_double_col_obs_ptr(size_t qubit) const {
    size_t col_start = ((table_row_length_bits(tableau.num_qubits) >> 8) * (qubit & ~0xFF)) | (qubit & 0xFF);
    return TransposedPauliStringPtr {
        &tableau.data_x2x_z2x_x2z_z2z.u256[col_start],
        &tableau.data_x2x_z2x_x2z_z2z.u256[col_start + (table_quadrant_bits(tableau.num_qubits) >> 7)],
    };
}

void BlockTransposedTableau::append_CX(size_t control, size_t target) {
    auto pc = transposed_double_col_obs_ptr(control);
    auto pt = transposed_double_col_obs_ptr(target);
    auto s = tableau.data_sign_x_z.u256;
    auto end = s + (tableau.data_sign_x_z.num_bits >> 8);
    auto stride256 = column_stride256(tableau.num_qubits);
    while (s != end) {
        *s ^= _mm256_andnot_si256(*pc.z ^ *pt.x, *pc.x & *pt.z);
        *pc.z ^= *pt.z;
        *pt.x ^= *pc.x;
        pc.x += stride256;
        pc.z += stride256;
        pt.x += stride256;
        pt.z += stride256;
        s++;
    }
}

void BlockTransposedTableau::append_H_YZ(size_t target) {
    auto p = transposed_double_col_obs_ptr(target);
    auto s = tableau.data_sign_x_z.u256;
    auto end = s + (tableau.data_sign_x_z.num_bits >> 8);
    auto stride256 = column_stride256(tableau.num_qubits);
    while (s != end) {
        *s ^= _mm256_andnot_si256(*p.z, *p.x);
        *p.x ^= *p.z;
        p.x += stride256;
        p.z += stride256;
        s++;
    }
}

void BlockTransposedTableau::append_H(size_t target) {
    auto p = transposed_double_col_obs_ptr(target);
    auto s = tableau.data_sign_x_z.u256;
    auto end = s + (tableau.data_sign_x_z.num_bits >> 8);
    auto stride256 = column_stride256(tableau.num_qubits);
    while (s != end) {
        std::swap(*p.x, *p.z);
        *s ^= *p.x & *p.z;
        p.x += stride256;
        p.z += stride256;
        s++;
    }
}

void BlockTransposedTableau::append_X(size_t target) {
    auto p = transposed_double_col_obs_ptr(target);
    auto s = tableau.data_sign_x_z.u256;
    auto end = s + (tableau.data_sign_x_z.num_bits >> 8);
    auto stride256 = column_stride256(tableau.num_qubits);
    while (s != end) {
        *s ^= *p.z;
        p.z += stride256;
        s++;
    }
}

PauliStringPtr Tableau::x_obs_ptr(size_t qubit) const {
    size_t quadrant = table_quadrant_bits(num_qubits) >> 6;
    return PauliStringPtr(
            num_qubits,
            BitPtr(data_sign_x_z.u64, qubit),
            &data_x2x_z2x_x2z_z2z.u64[4*qubit],
            &data_x2x_z2x_x2z_z2z.u64[4*qubit + quadrant*2],
            row_stride256(num_qubits));
}

PauliStringPtr Tableau::z_obs_ptr(size_t qubit) const {
    size_t quadrant = table_quadrant_bits(num_qubits) >> 6;
    return PauliStringPtr(
            num_qubits,
            BitPtr(data_sign_x_z.u64, ceil256(num_qubits) + qubit),
            &data_x2x_z2x_x2z_z2z.u64[4*qubit + quadrant],
            &data_x2x_z2x_x2z_z2z.u64[4*qubit + 3*quadrant],
            row_stride256(num_qubits));
}

bool Tableau::z_sign(size_t a) const {
    return data_sign_x_z.get_bit(a + table_row_length_bits(num_qubits));
}

bool BlockTransposedTableau::z_sign(size_t a) const {
    return tableau.z_sign(a);
}

bool BlockTransposedTableau::z_obs_x_bit(size_t a, size_t b) const {
    size_t col_start = ((table_row_length_bits(tableau.num_qubits) >> 8) * (b & ~0xFF)) | (b & 0xFF);
    a = ((a & ~0xFF) * column_stride256(tableau.num_qubits)) | (a & 0xFF);
    a += col_start << 8;
    auto quadrant = table_quadrant_bits(tableau.num_qubits);
    return tableau.data_x2x_z2x_x2z_z2z.get_bit(a + quadrant);
}

bool BlockTransposedTableau::z_obs_z_bit(size_t a, size_t b) const {
    size_t col_start = ((table_row_length_bits(tableau.num_qubits) >> 8) * (b & ~0xFF)) | (b & 0xFF);
    a = ((a & ~0xFF) * column_stride256(tableau.num_qubits)) | (a & 0xFF);
    a += col_start << 8;
    auto quadrant = table_quadrant_bits(tableau.num_qubits);
    return tableau.data_x2x_z2x_x2z_z2z.get_bit(a + 3 * quadrant);
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
        data_x2x_z2x_x2z_z2z(table_quadrant_bits(num_qubits) << 2),
        data_sign_x_z(table_row_length_bits(num_qubits) << 1) {
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
        && data_x2x_z2x_x2z_z2z == other.data_x2x_z2x_x2z_z2z
        && data_sign_x_z == other.data_sign_x_z;
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
