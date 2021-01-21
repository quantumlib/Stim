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

PauliStringRef TableauHalf::operator[](size_t input_qubit) {
    return PauliStringRef(num_qubits, signs[input_qubit], xt[input_qubit], zt[input_qubit]);
}

const PauliStringRef TableauHalf::operator[](size_t input_qubit) const {
    return PauliStringRef(num_qubits, signs[input_qubit], xt[input_qubit], zt[input_qubit]);
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
simd_bit_table random_stabilizer_tableau_raw(size_t n, std::mt19937 &gen) {
    auto rand_bit = [&]() { return gen() & 1; };
    auto hs_pair = sample_qmallows(n, gen);
    const auto &hada = hs_pair.first;
    const auto &perm = hs_pair.second;

    simd_bit_table symmetric(n, n);
    for (size_t col = 0; col < n; col++) {
        symmetric[col][col] = rand_bit();
        for (size_t row = col + 1; row < n; row++) {
            bool b = rand_bit();
            symmetric[row][col] = b;
            symmetric[col][row] = b;
        }
    }

    simd_bit_table symmetric_m(n, n);
    for (size_t col = 0; col < n; col++) {
        symmetric_m[col][col] = rand_bit() && hada[col];
        for (size_t row = col + 1; row < n; row++) {
            bool b = hada[row] && hada[col];
            b |= hada[row] > hada[col] && perm[row] < perm[col];
            b |= hada[row] < hada[col] && perm[row] > perm[col];
            b &= rand_bit();
            symmetric_m[row][col] = b;
            symmetric_m[col][row] = b;
        }
    }

    auto lower = simd_bit_table::identity(n);
    for (size_t col = 0; col < n; col++) {
        for (size_t row = col + 1; row < n; row++) {
            lower[row][col] = rand_bit();
        }
    }

    auto lower_m = simd_bit_table::identity(n);
    for (size_t col = 0; col < n; col++) {
        for (size_t row = col + 1; row < n; row++) {
            bool b = hada[row] < hada[col];
            b |= hada[row] && hada[col] && perm[row] > perm[col];
            b |= !hada[row] && !hada[col] && perm[row] < perm[col];
            b &= rand_bit();
            lower_m[row][col] = b;
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
        for (size_t col = 0; col < 2 * n; col++) {
            u[row][col] = fused[perm[row]][col];
            u[row + n][col] = fused[perm[row] + n][col];
        }
    }
    // Apply Hadamards.
    for (size_t row = 0; row < n; row++) {
        if (hada[row]) {
            for (size_t col = 0; col < 2*n; col++) {
                u[row][col].swap_with(u[row + n][col]);
            }
        }
    }

    return fused_m.square_mat_mul(u, 2*n);
}

Tableau Tableau::random(size_t num_qubits, std::mt19937 &rng) {
    auto raw = random_stabilizer_tableau_raw(num_qubits, rng);
    Tableau result(num_qubits);
    for (size_t row = 0; row < num_qubits; row++) {
        for (size_t col = 0; col < num_qubits; col++) {
            result.xs[row].x_ref[col] = raw[row][col];
            result.xs[row].z_ref[col] = raw[row][col + num_qubits];
            result.zs[row].x_ref[col] = raw[row + num_qubits][col];
            result.zs[row].z_ref[col] = raw[row + num_qubits][col + num_qubits];
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
    result.do_transpose_quadrants();

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
