#include <iostream>
#include <map>
#include <random>
#include "pauli_string.h"
#include "tableau.h"
#include "bit_mat.h"
#include <cmath>
#include <cstring>
#include <thread>

void do_transpose(Tableau &tableau) {
    size_t n = ceil256(tableau.num_qubits);
    if (n >= 1024) {
        std::thread t1([&]() { transpose_bit_matrix(tableau.xs.xt.data.u64, n); });
        std::thread t2([&]() { transpose_bit_matrix(tableau.xs.zt.data.u64, n); });
        std::thread t3([&]() { transpose_bit_matrix(tableau.zs.xt.data.u64, n); });
        transpose_bit_matrix(tableau.zs.zt.data.u64, n);
        t1.join();
        t2.join();
        t3.join();
    } else {
        transpose_bit_matrix(tableau.xs.xt.data.u64, n);
        transpose_bit_matrix(tableau.xs.zt.data.u64, n);
        transpose_bit_matrix(tableau.zs.xt.data.u64, n);
        transpose_bit_matrix(tableau.zs.zt.data.u64, n);
    }
}

TransposedTableauXZ TempTransposedTableauRaii::transposed_xz_ptr(size_t qubit) const {
    PauliStringRef x(tableau.xs[qubit]);
    PauliStringRef z(tableau.zs[qubit]);
    return {{
        {x.x_ref.u256, x.z_ref.u256, tableau.xs.signs.u256},
        {z.x_ref.u256, z.z_ref.u256, tableau.zs.signs.u256}
    }};
}

TempTransposedTableauRaii::TempTransposedTableauRaii(Tableau &tableau) : tableau(tableau) {
    do_transpose(tableau);
}

TempTransposedTableauRaii::~TempTransposedTableauRaii() {
    do_transpose(tableau);
}

void TempTransposedTableauRaii::append_CX(size_t control, size_t target) {
    auto pcs = transposed_xz_ptr(control);
    auto pts = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.xz[k];
        auto pt = pts.xz[k];
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
    size_t old_num_qubits = num_qubits;
    size_t old_num_simd_words = ceil256(old_num_qubits) >> 8;
    size_t new_num_simd_words = ceil256(new_num_qubits) >> 8;
    if (old_num_simd_words == new_num_simd_words) {
        for (size_t k = num_qubits; k < new_num_qubits; k++) {
            xs[k].x_ref[k] = true;
            zs[k].z_ref[k] = true;
        }
        num_qubits = new_num_qubits;
        return;
    }

    Tableau old_state = std::move(*this);
    this->~Tableau();
    new(this) Tableau(new_num_qubits);

    xs.signs.word_range_ref(0, old_num_simd_words) = old_state.xs.signs;
    zs.signs.word_range_ref(0, old_num_simd_words) = old_state.zs.signs;
    for (size_t k = 0; k < old_num_qubits; k++) {
        xs[k].x_ref.word_range_ref(0, old_num_simd_words) = old_state.xs[k].x_ref;
        xs[k].z_ref.word_range_ref(0, old_num_simd_words) = old_state.xs[k].z_ref;
        zs[k].x_ref.word_range_ref(0, old_num_simd_words) = old_state.zs[k].x_ref;
        zs[k].z_ref.word_range_ref(0, old_num_simd_words) = old_state.zs[k].z_ref;
    }
}

void TempTransposedTableauRaii::append_CY(size_t control, size_t target) {
    auto pcs = transposed_xz_ptr(control);
    auto pts = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.xz[k];
        auto pt = pts.xz[k];
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

void TempTransposedTableauRaii::append_CZ(size_t control, size_t target) {
    auto pcs = transposed_xz_ptr(control);
    auto pts = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.xz[k];
        auto pt = pts.xz[k];
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

void TempTransposedTableauRaii::append_SWAP(size_t q1, size_t q2) {
    auto p1s = transposed_xz_ptr(q1);
    auto p2s = transposed_xz_ptr(q2);
    for (size_t k = 0; k < 2; k++) {
        auto p1 = p1s.xz[k];
        auto p2 = p2s.xz[k];
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

void TempTransposedTableauRaii::append_H_XY(size_t target) {
    auto ps = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.xz[k];
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

void TempTransposedTableauRaii::append_H_YZ(size_t target) {
    auto ps = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.xz[k];
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

void TempTransposedTableauRaii::append_H(size_t target) {
    auto ps = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.xz[k];
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

void TempTransposedTableauRaii::append_X(size_t target) {
    auto ps = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.xz[k];
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= *p.z;
            p.z++;
            s++;
        }
    }
}

PauliStringRef TableauHalf::operator[](size_t input_qubit) {
    return PauliStringRef(num_qubits, signs[input_qubit], xt[input_qubit], zt[input_qubit]);
}

const PauliStringRef TableauHalf::operator[](size_t input_qubit) const {
    return PauliStringRef(num_qubits, signs[input_qubit], xt[input_qubit], zt[input_qubit]);
}

bool Tableau::z_sign(size_t a) const {
    return zs.signs[a];
}

bool TempTransposedTableauRaii::z_sign(size_t a) const {
    return tableau.z_sign(a);
}

bool TempTransposedTableauRaii::z_obs_x_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.zs.xt[output_qubit][input_qubit];
}

bool TempTransposedTableauRaii::x_obs_z_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.xs.zt[output_qubit][input_qubit];
}

bool TempTransposedTableauRaii::z_obs_z_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.zs.zt[output_qubit][input_qubit];
}

PauliStringVal Tableau::eval_y_obs(size_t qubit) const {
    PauliStringVal result(xs[qubit]);
    uint8_t log_i = result.ref().inplace_right_mul_returning_log_i_scalar(zs[qubit]);
    log_i++;
    assert((log_i & 1) == 0);
    if (log_i & 2) {
        result.val_sign ^= true;
    }
    return result;
}

Tableau::Tableau(size_t num_qubits) :
        num_qubits(num_qubits),
        xs(num_qubits),
        zs(num_qubits) {
    for (size_t q = 0; q < num_qubits; q++) {
        xs.xt[q][q] = true;
        zs.zt[q][q] = true;
    }
}

TableauHalf::TableauHalf(size_t num_qubits) :
        num_qubits(num_qubits),
        xt(ceil256(num_qubits), ceil256(num_qubits)),
        zt(ceil256(num_qubits), ceil256(num_qubits)),
        signs(ceil256(num_qubits)) {
}

Tableau Tableau::identity(size_t num_qubits) {
    return Tableau(num_qubits);
}

Tableau Tableau::gate1(const char *x, const char *z) {
    Tableau result(1);
    result.xs[0] = PauliStringVal::from_str(x);
    result.zs[0] = PauliStringVal::from_str(z);
    assert((bool)result.zs[0].sign_ref == (z[0] == '-'));
    return result;
}

Tableau Tableau::gate2(const char *x1,
                       const char *z1,
                       const char *x2,
                       const char *z2) {
    Tableau result(2);
    result.xs[0] = PauliStringVal::from_str(x1);
    result.zs[0] = PauliStringVal::from_str(z1);
    result.xs[1] = PauliStringVal::from_str(x2);
    result.zs[1] = PauliStringVal::from_str(z2);
    return result;
}

std::ostream &operator<<(std::ostream &out, const Tableau &t) {
    out << "+-";
    for (size_t k = 0; k < t.num_qubits; k++) {
        out << 'x';
        out << 'z';
        out << '-';
    }
    out << "\n|";
    for (size_t k = 0; k < t.num_qubits; k++) {
        out << ' ';
        out << "+-"[t.xs[k].sign_ref];
        out << "+-"[t.zs[k].sign_ref];
    }
    for (size_t q = 0; q < t.num_qubits; q++) {
        out << "\n|";
        for (size_t k = 0; k < t.num_qubits; k++) {
            out << ' ';
            auto x = t.xs[k];
            auto z = t.zs[k];
            out << "_XZY"[x.x_ref[q] + 2 * x.z_ref[q]];
            out << "_XZY"[z.x_ref[q] + 2 * z.z_ref[q]];
        }
    }
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
        auto x = xs[q];
        auto z = zs[q];
        operation.apply_within(x, target_qubits);
        operation.apply_within(z, target_qubits);
    }
}

bool Tableau::operator==(const Tableau &other) const {
    return num_qubits == other.num_qubits
        && xs.xt == other.xs.xt
        && xs.zt == other.xs.zt
        && zs.xt == other.zs.xt
        && zs.zt == other.zs.zt
        && xs.signs == other.xs.signs
        && zs.signs == other.zs.signs;
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
        new_x.emplace_back(std::move(scatter_eval(operation.xs[q], target_qubits)));
        new_z.emplace_back(std::move(scatter_eval(operation.zs[q], target_qubits)));
    }
    for (size_t q = 0; q < operation.num_qubits; q++) {
        xs[target_qubits[q]] = new_x[q];
        zs[target_qubits[q]] = new_z[q];
    }
}

void Tableau::prepend_SQRT_X(size_t q) {
    auto z = zs[q];
    uint8_t m = 1 + z.inplace_right_mul_returning_log_i_scalar(xs[q]);
    z.sign_ref ^= m & 2;
}

void Tableau::prepend_SQRT_X_DAG(size_t q) {
    auto z = zs[q];
    uint8_t m = 3 + z.inplace_right_mul_returning_log_i_scalar(xs[q]);
    z.sign_ref ^= m & 2;
}

void Tableau::prepend_SQRT_Y(size_t q) {
    auto z = zs[q];
    z.sign_ref ^= 1;
    xs[q].swap_with(z);
}

void Tableau::prepend_SQRT_Y_DAG(size_t q) {
    auto z = zs[q];
    xs[q].swap_with(z);
    z.sign_ref ^= 1;
}

void Tableau::prepend_SQRT_Z(size_t q) {
    auto x = xs[q];
    uint8_t m = 1 + x.inplace_right_mul_returning_log_i_scalar(zs[q]);
    x.sign_ref ^= m & 2;
}

void Tableau::prepend_SQRT_Z_DAG(size_t q) {
    auto x = xs[q];
    uint8_t m = 3 + x.inplace_right_mul_returning_log_i_scalar(zs[q]);
    x.sign_ref ^= m & 2;
}

void Tableau::prepend_SWAP(size_t q1, size_t q2) {
    auto z2 = zs[q2];
    auto x2 = xs[q2];
    zs[q1].swap_with(z2);
    xs[q1].swap_with(x2);
}

void Tableau::prepend_CX(size_t control, size_t target) {
    zs[target] *= zs[control];
    xs[control] *= xs[target];
}

void Tableau::prepend_CY(size_t control, size_t target) {
    prepend_H_YZ(target);
    prepend_CZ(control, target);
    prepend_H_YZ(target);
}

void Tableau::prepend_CZ(size_t control, size_t target) {
    xs[target] *= zs[control];
    xs[control] *= zs[target];
}

void Tableau::prepend_ISWAP(size_t q1, size_t q2) {
    prepend_SWAP(q1, q2);
    prepend_CZ(q1, q2);
    prepend_SQRT_Z(q1);
    prepend_SQRT_Z(q2);
}

void Tableau::prepend_ISWAP_DAG(size_t q1, size_t q2) {
    prepend_SWAP(q1, q2);
    prepend_CZ(q1, q2);
    prepend_SQRT_Z_DAG(q1);
    prepend_SQRT_Z_DAG(q2);
}

void Tableau::prepend_H(const size_t q) {
    auto z = zs[q];
    xs[q].swap_with(z);
}

void Tableau::prepend_H_YZ(const size_t q) {
    auto x = xs[q];
    auto z = zs[q];
    uint8_t m = 3 + z.inplace_right_mul_returning_log_i_scalar(x);
    x.sign_ref ^= 1;
    z.sign_ref ^= m & 2;
}

void Tableau::prepend_H_XY(const size_t q) {
    auto x = xs[q];
    auto z = zs[q];
    uint8_t m = 1 + x.inplace_right_mul_returning_log_i_scalar(z);
    z.sign_ref ^= 1;
    x.sign_ref ^= m & 2;
}

void Tableau::prepend(const PauliStringRef &op) {
    assert(op.num_qubits == num_qubits);
    zs.signs ^= op.x_ref;
    xs.signs ^= op.z_ref;
}

void Tableau::prepend(const SparsePauliString &op) {
    for (const auto &p : op.indexed_words) {
        zs.signs.u64[p.index64] ^= p.wx;
        xs.signs.u64[p.index64] ^= p.wz;
    }
}

void Tableau::prepend_X(size_t q) {
    zs[q].sign_ref ^= 1;
}

void Tableau::prepend_Y(size_t q) {
    xs[q].sign_ref ^= 1;
    zs[q].sign_ref ^= 1;
}

void Tableau::prepend_Z(size_t q) {
    xs[q].sign_ref ^= 1;
}

void Tableau::prepend_XCX(size_t control, size_t target) {
    zs[target] *= xs[control];
    zs[control] *= xs[target];
}

void Tableau::prepend_XCY(size_t control, size_t target) {
    prepend_H_XY(target);
    prepend_XCX(control, target);
    prepend_H_XY(target);
}

void Tableau::prepend_XCZ(size_t control, size_t target) {
    prepend_CX(target, control);
}

void Tableau::prepend_YCX(size_t control, size_t target) {
    prepend_XCY(target, control);
}

void Tableau::prepend_YCY(size_t control, size_t target) {
    prepend_H_YZ(control);
    prepend_H_YZ(target);
    prepend_CZ(control, target);
    prepend_H_YZ(target);
    prepend_H_YZ(control);
}

void Tableau::prepend_YCZ(size_t control, size_t target) {
    prepend_CY(target, control);
}

PauliStringVal Tableau::scatter_eval(const PauliStringRef &gathered_input, const std::vector<size_t> &scattered_indices) const {
    assert(gathered_input.num_qubits == scattered_indices.size());
    auto result = PauliStringVal::identity(num_qubits);
    result.val_sign = gathered_input.sign_ref;
    for (size_t k_gathered = 0; k_gathered < gathered_input.num_qubits; k_gathered++) {
        size_t k_scattered = scattered_indices[k_gathered];
        bool x = gathered_input.x_ref[k_gathered];
        bool z = gathered_input.z_ref[k_gathered];
        if (x) {
            if (z) {
                // Multiply by Y using Y = i*X*Z.
                uint8_t log_i = 1;
                log_i += result.ref().inplace_right_mul_returning_log_i_scalar(xs[k_scattered]);
                log_i += result.ref().inplace_right_mul_returning_log_i_scalar(zs[k_scattered]);
                assert((log_i & 1) == 0);
                result.val_sign ^= (log_i & 2) != 0;
            } else {
                result.ref() *= xs[k_scattered];
            }
        } else if (z) {
            result.ref() *= zs[k_scattered];
        }
    }
    return result;
}

PauliStringVal Tableau::operator()(const PauliStringRef &p) const {
    assert(p.num_qubits == num_qubits);
    std::vector<size_t> indices;
    for (size_t k = 0; k < p.num_qubits; k++) {
        indices.push_back(k);
    }
    return scatter_eval(p, indices);
}

void Tableau::apply_within(PauliStringRef &target, const std::vector<size_t> &target_qubits) const {
    assert(num_qubits == target_qubits.size());
    auto inp = PauliStringVal::identity(num_qubits);
    target.gather_into(inp, target_qubits);
    auto out = (*this)(inp);
    out.ref().scatter_into(target, target_qubits);
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

Tableau Tableau::random(size_t num_qubits, std::mt19937 &rng) {
    auto raw = random_stabilizer_tableau_raw(num_qubits, rng);
    Tableau result(num_qubits);
    for (size_t row = 0; row < num_qubits; row++) {
        for (size_t col = 0; col < num_qubits; col++) {
            result.xs[row].x_ref[col] = raw.get(row, col);
            result.xs[row].z_ref[col] = raw.get(row, col + num_qubits);
            result.zs[row].x_ref[col] = raw.get(row + num_qubits, col);
            result.zs[row].z_ref[col] = raw.get(row + num_qubits, col + num_qubits);
        }
        uint32_t u = rng();
        result.xs.signs[row] = u & 1;
        result.zs.signs[row] = u & 2;
    }
    return result;
}

bool Tableau::satisfies_invariants() const {
    for (size_t q1 = 0; q1 < num_qubits; q1++) {
        auto x1 = xs[q1];
        auto z1 = zs[q1];
        if (x1.commutes(z1)) {
            return false;
        }
        for (size_t q2 = q1 + 1; q2 < num_qubits; q2++) {
            auto x2 = xs[q2];
            auto z2 = zs[q2];
            if (!x1.commutes(x2) || !x1.commutes(z2) || !z1.commutes(x2) || !z1.commutes(z2)) {
                return false;
            }
        }
    }
    return true;
}

Tableau Tableau::inverse() const {
    Tableau result(num_qubits);

    // Transpose data with xx zz swap tweak.
    result.xs.xt.data = zs.zt.data;
    result.xs.zt.data = xs.zt.data;
    result.zs.xt.data = zs.xt.data;
    result.zs.zt.data = xs.xt.data;
    do_transpose(result);

    // Fix signs by checking for consistent round trips.
    PauliStringVal singleton(num_qubits);
    for (size_t k = 0; k < num_qubits; k++) {
        singleton.x_data[k] = true;
        bool x_round_trip_sign = (*this)(result(singleton)).val_sign;
        singleton.x_data[k] = false;
        singleton.z_data[k] = true;
        bool z_round_trip_sign = (*this)(result(singleton)).val_sign;
        singleton.z_data[k] = false;

        result.xs[k].sign_ref ^= x_round_trip_sign;
        result.zs[k].sign_ref ^= z_round_trip_sign;
    }

    return result;
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
    {"ISWAP", "ISWAP_DAG"},
    {"ISWAP_DAG", "ISWAP"},
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
    {"ISWAP", Tableau::gate2("+ZY", "+IZ", "+YZ", "+ZI")},
    {"ISWAP_DAG", Tableau::gate2("-ZY", "+IZ", "-YZ", "+ZI")},
    // Controlled interactions in other bases.
    {"XCX", Tableau::gate2("+XI", "+ZX", "+IX", "+XZ")},
    {"XCY", Tableau::gate2("+XI", "+ZY", "+XX", "+XZ")},
    {"XCZ", Tableau::gate2("+XI", "+ZZ", "+XX", "+IZ")},
    {"YCX", Tableau::gate2("+XX", "+ZX", "+IX", "+YZ")},
    {"YCY", Tableau::gate2("+XY", "+ZY", "+YX", "+YZ")},
    {"YCZ", Tableau::gate2("+XZ", "+ZZ", "+YX", "+IZ")},
};
