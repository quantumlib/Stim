#include <iostream>
#include <map>
#include <random>
#include "pauli_string.h"
#include "tableau.h"
#include "bit_mat.h"
#include <cmath>
#include <cstring>
#include <thread>

#define X2X_QUAD 0
#define Z2X_QUAD 1
#define X2Z_QUAD 2
#define Z2Z_QUAD 3

size_t bit_address(
        size_t input_qubit,
        size_t output_qubit,
        size_t num_qubits,
        size_t quadrant,
        bool transposed) {
    if (transposed) {
        std::swap(input_qubit, output_qubit);
    }
    size_t m = ceil256(num_qubits);
    size_t bit_offset = input_qubit * m + output_qubit;
    return bit_offset + quadrant * m * m;
}

TempBlockTransposedTableauRaii::TempBlockTransposedTableauRaii(Tableau &tableau) : tableau(tableau) {
    do_transpose();
}

TempBlockTransposedTableauRaii::~TempBlockTransposedTableauRaii() {
    do_transpose();
}

void TempBlockTransposedTableauRaii::do_transpose() {
    size_t n = ceil256(tableau.num_qubits);
    size_t m = (n * n) >> 6;
    if (n >= 1024) {
        std::thread t1([&]() { transpose_bit_matrix(tableau.data_x2x_z2x_x2z_z2z.u64, n); });
        std::thread t2([&]() { transpose_bit_matrix(tableau.data_x2x_z2x_x2z_z2z.u64 + m, n); });
        std::thread t3([&]() { transpose_bit_matrix(tableau.data_x2x_z2x_x2z_z2z.u64 + m * 2, n); });
        transpose_bit_matrix(tableau.data_x2x_z2x_x2z_z2z.u64 + m * 3, n);
        t1.join();
        t2.join();
        t3.join();
    } else {
        transpose_bit_matrix(tableau.data_x2x_z2x_x2z_z2z.u64, n);
        transpose_bit_matrix(tableau.data_x2x_z2x_x2z_z2z.u64 + m, n);
        transpose_bit_matrix(tableau.data_x2x_z2x_x2z_z2z.u64 + m * 2, n);
        transpose_bit_matrix(tableau.data_x2x_z2x_x2z_z2z.u64 + m * 3, n);
    }
}

TransposedPauliStringPtr TempBlockTransposedTableauRaii::transposed_double_col_obs_ptr(size_t qubit) const {
    return TransposedPauliStringPtr {
        &tableau.data_x2x_z2x_x2z_z2z.u256[bit_address(0, qubit, tableau.num_qubits, X2X_QUAD, true) >> 8],
        &tableau.data_x2x_z2x_x2z_z2z.u256[bit_address(0, qubit, tableau.num_qubits, X2Z_QUAD, true) >> 8],
        tableau.data_sign_x_z.u256,
        &tableau.data_x2x_z2x_x2z_z2z.u256[bit_address(0, qubit, tableau.num_qubits, Z2X_QUAD, true) >> 8],
        &tableau.data_x2x_z2x_x2z_z2z.u256[bit_address(0, qubit, tableau.num_qubits, Z2Z_QUAD, true) >> 8],
        tableau.data_sign_x_z.u256 + (ceil256(tableau.num_qubits) >> 8),
    };
}

void TempBlockTransposedTableauRaii::append_CX(size_t control, size_t target) {
    auto pcs = transposed_double_col_obs_ptr(control);
    auto pts = transposed_double_col_obs_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.get(k);
        auto pt = pts.get(k);
        auto s = pc.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= _mm256_andnot_si256(*pc.z ^ *pt.x, *pc.x & *pt.z);
            *pc.z ^= *pt.z;
            *pt.x ^= *pc.x;
            pc.x++;
            pc.z++;
            pt.x++;
            pt.z++;
            s++;
        }
    }
}

void Tableau::expand(size_t new_num_qubits) {
    assert(new_num_qubits >= num_qubits);
    size_t n1 = num_qubits;
    size_t m1 = ceil256(n1);
    size_t m2 = ceil256(new_num_qubits);
    if (m1 == m2) {
        for (size_t k = n1; k < new_num_qubits; k++) {
            x_obs_ptr(k).set_x_bit(k, true);
            z_obs_ptr(k).set_z_bit(k, true);
        }
        num_qubits = new_num_qubits;
        return;
    }

    Tableau old_state = std::move(*this);
    this->~Tableau();
    new(this) Tableau(new_num_qubits);

    memcpy(data_sign_x_z.u256, old_state.data_sign_x_z.u256, m1 >> 3);
    memcpy(data_sign_x_z.u256 + (m2 >> 8), old_state.data_sign_x_z.u256 + (m1 >> 8), m1 >> 3);
    for (size_t k = 0; k < n1; k++) {
        memcpy(x_obs_ptr(k)._x, old_state.x_obs_ptr(k)._x, m1 >> 3);
        memcpy(x_obs_ptr(k)._z, old_state.x_obs_ptr(k)._z, m1 >> 3);
        memcpy(z_obs_ptr(k)._x, old_state.z_obs_ptr(k)._x, m1 >> 3);
        memcpy(z_obs_ptr(k)._z, old_state.z_obs_ptr(k)._z, m1 >> 3);
    }
}

void TempBlockTransposedTableauRaii::append_CY(size_t control, size_t target) {
    auto pcs = transposed_double_col_obs_ptr(control);
    auto pts = transposed_double_col_obs_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.get(k);
        auto pt = pts.get(k);
        auto s = pc.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= *pc.x & (*pc.z ^ *pt.x) & (*pt.z ^ *pt.x);
            *pc.z ^= *pt.x;
            *pc.z ^= *pt.z;
            *pt.x ^= *pc.x;
            *pt.z ^= *pc.x;
            pc.x++;
            pc.z++;
            pt.x++;
            pt.z++;
            s++;
        }
    }
}

void TempBlockTransposedTableauRaii::append_CZ(size_t control, size_t target) {
    auto pcs = transposed_double_col_obs_ptr(control);
    auto pts = transposed_double_col_obs_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.get(k);
        auto pt = pts.get(k);
        auto s = pc.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= *pc.x & *pt.x & (*pc.z ^ *pt.z);
            *pc.z ^= *pt.x;
            *pt.z ^= *pc.x;
            pc.x++;
            pc.z++;
            pt.x++;
            pt.z++;
            s++;
        }
    }
}

void TempBlockTransposedTableauRaii::append_SWAP(size_t q1, size_t q2) {
    auto p1s = transposed_double_col_obs_ptr(q1);
    auto p2s = transposed_double_col_obs_ptr(q2);
    for (size_t k = 0; k < 2; k++) {
        auto p1 = p1s.get(k);
        auto p2 = p2s.get(k);
        auto end = p1.x + (ceil256(tableau.num_qubits) >> 8);
        while (p1.x != end) {
            std::swap(*p1.x, *p2.x);
            std::swap(*p1.z, *p2.z);
            p1.x++;
            p1.z++;
            p2.x++;
            p2.z++;
        }
    }
}

void TempBlockTransposedTableauRaii::append_H_XY(size_t target) {
    auto ps = transposed_double_col_obs_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.get(k);
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= _mm256_andnot_si256(*p.x, *p.z);
            *p.z ^= *p.x;
            p.x++;
            p.z++;
            s++;
        }
    }
}

void TempBlockTransposedTableauRaii::append_H_YZ(size_t target) {
    auto ps = transposed_double_col_obs_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.get(k);
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= _mm256_andnot_si256(*p.z, *p.x);
            *p.x ^= *p.z;
            p.x++;
            p.z++;
            s++;
        }
    }
}

void TempBlockTransposedTableauRaii::append_H(size_t target) {
    auto ps = transposed_double_col_obs_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.get(k);
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            std::swap(*p.x, *p.z);
            *s ^= *p.x & *p.z;
            p.x++;
            p.z++;
            s++;
        }
    }
}

void TempBlockTransposedTableauRaii::append_X(size_t target) {
    auto ps = transposed_double_col_obs_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.get(k);
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= *p.z;
            p.z++;
            s++;
        }
    }
}

PauliStringPtr Tableau::x_obs_ptr(size_t qubit) const {
    return PauliStringPtr(
            num_qubits,
            BitPtr(data_sign_x_z.u64, qubit),
            &data_x2x_z2x_x2z_z2z.u64[bit_address(qubit, 0, num_qubits, X2X_QUAD, false) >> 6],
            &data_x2x_z2x_x2z_z2z.u64[bit_address(qubit, 0, num_qubits, X2Z_QUAD, false) >> 6]);
}

PauliStringPtr Tableau::z_obs_ptr(size_t qubit) const {
    return PauliStringPtr(
            num_qubits,
            BitPtr(data_sign_x_z.u64, ceil256(num_qubits) + qubit),
            &data_x2x_z2x_x2z_z2z.u64[bit_address(qubit, 0, num_qubits, Z2X_QUAD, false) >> 6],
            &data_x2x_z2x_x2z_z2z.u64[bit_address(qubit, 0, num_qubits, Z2Z_QUAD, false) >> 6]);
}

bool Tableau::z_sign(size_t a) const {
    return data_sign_x_z.get_bit(a + ceil256(num_qubits));
}

bool TempBlockTransposedTableauRaii::z_sign(size_t a) const {
    return tableau.z_sign(a);
}

bool TempBlockTransposedTableauRaii::z_obs_x_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.data_x2x_z2x_x2z_z2z.get_bit(
            bit_address(input_qubit, output_qubit, tableau.num_qubits, Z2X_QUAD, true)
    );
}

bool TempBlockTransposedTableauRaii::x_obs_z_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.data_x2x_z2x_x2z_z2z.get_bit(
            bit_address(input_qubit, output_qubit, tableau.num_qubits, X2Z_QUAD, true)
    );
}

bool TempBlockTransposedTableauRaii::z_obs_z_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.data_x2x_z2x_x2z_z2z.get_bit(
            bit_address(input_qubit, output_qubit, tableau.num_qubits, Z2Z_QUAD, true)
    );
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
        data_x2x_z2x_x2z_z2z(ceil256(num_qubits) * ceil256(num_qubits) * 4),
        data_sign_x_z(ceil256(num_qubits) * 2) {
    for (size_t q = 0; q < num_qubits; q++) {
        data_x2x_z2x_x2z_z2z.set_bit(bit_address(q, q, num_qubits, X2X_QUAD, false), true);
        data_x2x_z2x_x2z_z2z.set_bit(bit_address(q, q, num_qubits, Z2Z_QUAD, false), true);
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

void Tableau::prepend_SWAP(size_t q1, size_t q2) {
    auto z2 = z_obs_ptr(q2);
    auto x2 = x_obs_ptr(q2);
    z_obs_ptr(q1).swap_with(z2);
    x_obs_ptr(q1).swap_with(x2);
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

/// Samples a vector of bits and a permutation from a skewed distribution.
///
/// Reference:
///     "Hadamard-free circuits expose the structure of the Clifford group"
///     Sergey Bravyi, Dmitri Maslov
///     https://arxiv.org/abs/2003.09412
std::pair<std::vector<bool>, std::vector<size_t>> sample_qmallows(size_t n, std::mt19937 &gen) {
    auto uni = std::uniform_real_distribution<double>(0, 1);

    std::vector<bool> hada;
    std::vector<size_t> permutation;
    std::vector<size_t> remaining_indices;
    for (size_t k = 0; k < n; k++) {
        remaining_indices.push_back(k);
    }
    for (size_t i = 0; i < n; i++) {
		auto m = remaining_indices.size();
		auto u = uni(gen);
		auto eps = pow(4, -(int)m);
		auto k = (size_t)-ceil(log2(u + (1 - u) * eps));
		hada.push_back(k < m);
		if (k >= m) {
            k = 2 * m - k - 1;
        }
		permutation.push_back(remaining_indices[k]);
		remaining_indices.erase(remaining_indices.begin() + k);
    }
    return {hada, permutation};
}

/// Samples a random valid stabilizer tableau.
///
/// Reference:
///     "Hadamard-free circuits expose the structure of the Clifford group"
///     Sergey Bravyi, Dmitri Maslov
///     https://arxiv.org/abs/2003.09412
BitMat random_stabilizer_tableau_raw(size_t n, std::mt19937 &gen) {
    auto rand_bit = std::bernoulli_distribution(0.5);
    auto hs_pair = sample_qmallows(n, gen);
    const auto &hada = hs_pair.first;
    const auto &perm = hs_pair.second;

    BitMat symmetric(n);
    for (size_t col = 0; col < n; col++) {
        symmetric.set(col, col, rand_bit(gen));
        for (size_t row = col + 1; row < n; row++) {
            bool b = rand_bit(gen);
            symmetric.set(row, col, b);
            symmetric.set(col, row, b);
        }
    }

    BitMat symmetric_m(n);
    for (size_t col = 0; col < n; col++) {
        symmetric_m.set(col, col, rand_bit(gen) && hada[col]);
        for (size_t row = col + 1; row < n; row++) {
            bool b = hada[row] && hada[col];
            b |= hada[row] > hada[col] && perm[row] < perm[col];
            b |= hada[row] < hada[col] && perm[row] > perm[col];
            b &= rand_bit(gen);
            symmetric_m.set(row, col, b);
            symmetric_m.set(col, row, b);
        }
    }

    auto lower = BitMat::identity(n);
    for (size_t col = 0; col < n; col++) {
        for (size_t row = col + 1; row < n; row++) {
            lower.set(row, col, rand_bit(gen));
        }
    }

    auto lower_m = BitMat::identity(n);
    for (size_t col = 0; col < n; col++) {
        for (size_t row = col + 1; row < n; row++) {
            bool b = hada[row] < hada[col];
            b |= hada[row] && hada[col] && perm[row] > perm[col];
            b |= !hada[row] && !hada[col] && perm[row] < perm[col];
            b &= rand_bit(gen);
            lower_m.set(row, col, b);
        }
    }

    auto prod = symmetric * lower;
    auto prod_m = symmetric_m * lower_m;

    auto inv = lower.inv_lower_triangular().transposed();
    auto inv_m = lower_m.inv_lower_triangular().transposed();

    auto fused = BitMat::from_quadrants(
        lower, BitMat(n),
        prod, inv);
    auto fused_m = BitMat::from_quadrants(
        lower_m, BitMat(n),
        prod_m, inv_m);

    BitMat u(2*n);

    // Apply permutation.
    for (size_t row = 0; row < n; row++) {
        for (size_t col = 0; col < 2 * n; col++) {
            u.set(row, col, fused.get(perm[row], col));
            u.set(row + n, col, fused.get(perm[row] + n, col));
        }
    }
    // Apply Hadamards.
    for (size_t row = 0; row < n; row++) {
        if (hada[row]) {
            for (size_t col = 0; col < 2*n; col++) {
                bool t = u.get(row, col);
                u.set(row, col, u.get(row + n, col));
                u.set(row + n, col, t);
            }
        }
    }

    return fused_m * u;
}

Tableau Tableau::random(size_t num_qubits) {
    std::random_device rd;
    std::mt19937 gen(rd());
    auto rand_bit = std::bernoulli_distribution(0.5);

    auto raw = random_stabilizer_tableau_raw(num_qubits, gen);
    Tableau result(num_qubits);
    for (size_t row = 0; row < num_qubits; row++) {
        for (size_t col = 0; col < num_qubits; col++) {
            result.x_obs_ptr(row).set_x_bit(col, raw.get(row, col));
            result.x_obs_ptr(row).set_z_bit(col, raw.get(row, col + num_qubits));
            result.z_obs_ptr(row).set_x_bit(col, raw.get(row + num_qubits, col));
            result.z_obs_ptr(row).set_z_bit(col, raw.get(row + num_qubits, col + num_qubits));
        }
        result.data_sign_x_z.set_bit(row, rand_bit(gen));
        result.data_sign_x_z.set_bit(ceil256(num_qubits) + row, rand_bit(gen));
    }
    return result;
}

bool Tableau::satisfies_invariants() const {
    for (size_t q1 = 0; q1 < num_qubits; q1++) {
        auto x1 = x_obs_ptr(q1);
        auto z1 = z_obs_ptr(q1);
        if (x1.commutes(z1)) {
            return false;
        }
        for (size_t q2 = q1 + 1; q2 < num_qubits; q2++) {
            auto x2 = x_obs_ptr(q2);
            auto z2 = z_obs_ptr(q2);
            if (!x1.commutes(x2) || !x1.commutes(z2) || !z1.commutes(x2) || !z1.commutes(z2)) {
                return false;
            }
        }
    }
    return true;
}

const std::unordered_map<std::string, const std::string> GATE_INVERSE_NAMES {
    {"I", "I"},
    {"X", "X"},
    {"Y", "Y"},
    {"Z", "Z"},
    {"H", "H"},
    {"H_XY", "H_XY"},
    {"H_XZ", "H_XZ"},
    {"H_YZ", "H_YZ"},
    {"SQRT_X", "SQRT_X_DAG"},
    {"SQRT_X_DAG", "SQRT_X"},
    {"SQRT_Y", "SQRT_Y_DAG"},
    {"SQRT_Y_DAG", "SQRT_Y"},
    {"SQRT_Z", "SQRT_Z_DAG"},
    {"SQRT_Z_DAG", "SQRT_Z"},
    {"S", "S_DAG"},
    {"S_DAG", "S"},
    {"SWAP", "SWAP"},
    {"CNOT", "CNOT"},
    {"CX", "CX"},
    {"CY", "CY"},
    {"CZ", "CZ"},
    {"XCX", "XCX"},
    {"XCY", "XCY"},
    {"XCZ", "XCZ"},
    {"YCX", "YCX"},
    {"YCY", "YCY"},
    {"YCZ", "YCZ"},
};

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
    {"SWAP", Tableau::gate2("+IX", "+IZ", "+XI", "+ZI")},
    {"CNOT", Tableau::gate2("+XX", "+ZI", "+IX", "+ZZ")},
    {"CX", Tableau::gate2("+XX", "+ZI", "+IX", "+ZZ")},
    {"CY", Tableau::gate2("+XY", "+ZI", "+ZX", "+ZZ")},
    {"CZ", Tableau::gate2("+XZ", "+ZI", "+ZX", "+IZ")},
    // Controlled interactions in other bases.
    {"XCX", Tableau::gate2("+XI", "+ZX", "+IX", "+XZ")},
    {"XCY", Tableau::gate2("+XI", "+ZY", "+XX", "+XZ")},
    {"XCZ", Tableau::gate2("+XI", "+ZZ", "+XX", "+IZ")},
    {"YCX", Tableau::gate2("+XX", "+ZX", "+IX", "+YZ")},
    {"YCY", Tableau::gate2("+XY", "+ZY", "+YX", "+YZ")},
    {"YCZ", Tableau::gate2("+XZ", "+ZZ", "+YX", "+IZ")},
};
