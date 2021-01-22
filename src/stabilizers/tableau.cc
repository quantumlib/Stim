#include <iostream>
#include <map>
#include <random>
#include <cmath>
#include <cstring>
#include <thread>
#include "pauli_string.h"
#include "tableau.h"
#include "tableau_transposed_raii.h"

void Tableau::expand(size_t new_num_qubits) {
    // If the new qubits fit inside the padding, just extend into it.
    assert(new_num_qubits >= num_qubits);
    if (new_num_qubits <= xs.xt.num_major_bits_padded()) {
        for (size_t k = num_qubits; k < new_num_qubits; k++) {
            xs[k].x_ref[k] = true;
            zs[k].z_ref[k] = true;
        }
        num_qubits = new_num_qubits;
        return;
    }

    // Move state to temporary storage then re-allocate to make room for additional qubits.
    size_t old_num_simd_words = xs.xt.num_simd_words_major;
    size_t old_num_qubits = num_qubits;
    Tableau old_state = std::move(*this);
    this->~Tableau();
    new(this) Tableau(new_num_qubits);

    // Copy stored state back into new larger space.
    auto partial_copy = [=](simd_bits_range_ref dst, simd_bits_range_ref src) {
        dst.word_range_ref(0, old_num_simd_words) = src;
    };
    partial_copy(xs.signs, old_state.xs.signs);
    partial_copy(zs.signs, old_state.zs.signs);
    for (size_t k = 0; k < old_num_qubits; k++) {
        partial_copy(xs[k].x_ref, old_state.xs[k].x_ref);
        partial_copy(xs[k].z_ref, old_state.xs[k].z_ref);
        partial_copy(zs[k].x_ref, old_state.zs[k].x_ref);
        partial_copy(zs[k].z_ref, old_state.zs[k].z_ref);
    }
}

PauliStringRef TableauHalf::operator[](size_t input_qubit) {
    return PauliStringRef(num_qubits, signs[input_qubit], xt[input_qubit], zt[input_qubit]);
}

const PauliStringRef TableauHalf::operator[](size_t input_qubit) const {
    return PauliStringRef(num_qubits, signs[input_qubit], xt[input_qubit], zt[input_qubit]);
}

PauliString Tableau::eval_y_obs(size_t qubit) const {
    PauliString result(xs[qubit]);
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
        xt(num_qubits, num_qubits),
        zt(num_qubits, num_qubits),
        signs(num_qubits) {
}

Tableau Tableau::identity(size_t num_qubits) {
    return Tableau(num_qubits);
}

Tableau Tableau::gate1(const char *x, const char *z) {
    Tableau result(1);
    result.xs[0] = PauliString::from_str(x);
    result.zs[0] = PauliString::from_str(z);
    assert((bool)result.zs[0].sign_ref == (z[0] == '-'));
    return result;
}

Tableau Tableau::gate2(const char *x1,
                       const char *z1,
                       const char *x2,
                       const char *z2) {
    Tableau result(2);
    result.xs[0] = PauliString::from_str(x1);
    result.zs[0] = PauliString::from_str(z1);
    result.xs[1] = PauliString::from_str(x2);
    result.zs[1] = PauliString::from_str(z2);
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
    std::vector<PauliString> new_x;
    std::vector<PauliString> new_z;
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

PauliString Tableau::scatter_eval(const PauliStringRef &gathered_input, const std::vector<size_t> &scattered_indices) const {
    assert(gathered_input.num_qubits == scattered_indices.size());
    auto result = PauliString::PauliString(num_qubits);
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

PauliString Tableau::operator()(const PauliStringRef &p) const {
    assert(p.num_qubits == num_qubits);
    std::vector<size_t> indices;
    for (size_t k = 0; k < p.num_qubits; k++) {
        indices.push_back(k);
    }
    return scatter_eval(p, indices);
}

void Tableau::apply_within(PauliStringRef &target, const std::vector<size_t> &target_qubits) const {
    assert(num_qubits == target_qubits.size());
    auto inp = PauliString::PauliString(num_qubits);
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
std::pair<std::vector<bool>, std::vector<size_t>> sample_qmallows(size_t n, std::mt19937_64 &gen) {
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
simd_bit_table random_stabilizer_tableau_raw(size_t n, std::mt19937_64 &rng) {
    auto hs_pair = sample_qmallows(n, rng);
    const auto &hada = hs_pair.first;
    const auto &perm = hs_pair.second;

    simd_bit_table symmetric(n, n);
    for (size_t row = 0; row < n; row++) {
        symmetric[row].randomize(row + 1, rng);
        for (size_t col = 0; col < row; col++) {
            symmetric[col][row] = symmetric[row][col];
        }
    }

    simd_bit_table symmetric_m(n, n);
    for (size_t row = 0; row < n; row++) {
        symmetric_m[row].randomize(row + 1, rng);
        symmetric_m[row][row] &= hada[row];
        for (size_t col = 0; col < row; col++) {
            bool b = hada[row] && hada[col];
            b |= hada[row] > hada[col] && perm[row] < perm[col];
            b |= hada[row] < hada[col] && perm[row] > perm[col];
            symmetric_m[row][col] &= b;
            symmetric_m[col][row] = symmetric_m[row][col];
        }
    }

    auto lower = simd_bit_table::identity(n);
    for (size_t row = 0; row < n; row++) {
        lower[row].randomize(row, rng);
    }

    auto lower_m = simd_bit_table::identity(n);
    for (size_t row = 0; row < n; row++) {
        lower_m[row].randomize(row, rng);
        for (size_t col = 0; col < row; col++) {
            bool b = hada[row] < hada[col];
            b |= hada[row] && hada[col] && perm[row] > perm[col];
            b |= !hada[row] && !hada[col] && perm[row] < perm[col];
            lower_m[row][col] &= b;
        }
    }

    auto prod = symmetric.square_mat_mul(lower, n);
    auto prod_m = symmetric_m.square_mat_mul(lower_m, n);

    auto inv = lower.inverse_assuming_lower_triangular(n);
    auto inv_m = lower_m.inverse_assuming_lower_triangular(n);
    inv.do_square_transpose();
    inv_m.do_square_transpose();

    auto fused = simd_bit_table::from_quadrants(
        n,
        lower, simd_bit_table(n, n),
        prod, inv);
    auto fused_m = simd_bit_table::from_quadrants(
        n,
        lower_m, simd_bit_table(n, n),
        prod_m, inv_m);

    simd_bit_table u(2*n, 2*n);

    // Apply permutation.
    for (size_t row = 0; row < n; row++) {
        u[row] = fused[perm[row]];
        u[row + n] = fused[perm[row] + n];
    }
    // Apply Hadamards.
    for (size_t row = 0; row < n; row++) {
        if (hada[row]) {
            u[row].swap_with(u[row + n]);
        }
    }

    return fused_m.square_mat_mul(u, 2*n);
}

Tableau Tableau::random(size_t num_qubits, std::mt19937_64 &rng) {
    auto raw = random_stabilizer_tableau_raw(num_qubits, rng);
    Tableau result(num_qubits);
    for (size_t row = 0; row < num_qubits; row++) {
        for (size_t col = 0; col < num_qubits; col++) {
            result.xs[row].x_ref[col] = raw[row][col];
            result.xs[row].z_ref[col] = raw[row][col + num_qubits];
            result.zs[row].x_ref[col] = raw[row + num_qubits][col];
            result.zs[row].z_ref[col] = raw[row + num_qubits][col + num_qubits];
        }
    }
    result.xs.signs.randomize(num_qubits, rng);
    result.zs.signs.randomize(num_qubits, rng);
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
    result.do_transpose_quadrants();

    // Fix signs by checking for consistent round trips.
    PauliString singleton(num_qubits);
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

void Tableau::do_transpose_quadrants() {
    if (num_qubits >= 1024) {
        std::thread t1([&]() { xs.xt.do_square_transpose(); });
        std::thread t2([&]() { xs.zt.do_square_transpose(); });
        std::thread t3([&]() { zs.xt.do_square_transpose(); });
        zs.zt.do_square_transpose();
        t1.join();
        t2.join();
        t3.join();
    } else {
        xs.xt.do_square_transpose();
        xs.zt.do_square_transpose();
        zs.xt.do_square_transpose();
        zs.zt.do_square_transpose();
    }
}
