// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <random>

#include "stim/gates/gates.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

template <size_t W>
void Tableau<W>::expand(size_t new_num_qubits, double resize_pad_factor) {
    // If the new qubits fit inside the padding, just extend into it.
    assert(new_num_qubits >= num_qubits);
    assert(resize_pad_factor >= 1);
    if (new_num_qubits <= xs.xt.num_major_bits_padded()) {
        size_t old_num_qubits = num_qubits;
        num_qubits = new_num_qubits;
        xs.num_qubits = new_num_qubits;
        zs.num_qubits = new_num_qubits;
        // Initialize identity elements along the diagonal.
        for (size_t k = old_num_qubits; k < new_num_qubits; k++) {
            xs[k].xs[k] = true;
            zs[k].zs[k] = true;
        }
        return;
    }

    // Move state to temporary storage then re-allocate to make room for additional qubits.
    size_t old_num_simd_words = xs.xt.num_simd_words_major;
    size_t old_num_qubits = num_qubits;
    Tableau<W> old_state = std::move(*this);
    *this = Tableau<W>((size_t)(new_num_qubits * resize_pad_factor));
    this->num_qubits = new_num_qubits;
    this->xs.num_qubits = new_num_qubits;
    this->zs.num_qubits = new_num_qubits;

    // Copy stored state back into new larger space.
    auto partial_copy = [=](simd_bits_range_ref<W> dst, simd_bits_range_ref<W> src) {
        dst.word_range_ref(0, old_num_simd_words) = src;
    };
    partial_copy(xs.signs, old_state.xs.signs);
    partial_copy(zs.signs, old_state.zs.signs);
    for (size_t k = 0; k < old_num_qubits; k++) {
        partial_copy(xs[k].xs, old_state.xs[k].xs);
        partial_copy(xs[k].zs, old_state.xs[k].zs);
        partial_copy(zs[k].xs, old_state.zs[k].xs);
        partial_copy(zs[k].zs, old_state.zs[k].zs);
    }
}

template <size_t W>
PauliStringRef<W> TableauHalf<W>::operator[](size_t input_qubit) {
    size_t nw = (num_qubits + W - 1) / W;
    return PauliStringRef<W>(
        num_qubits, signs[input_qubit], xt[input_qubit].word_range_ref(0, nw), zt[input_qubit].word_range_ref(0, nw));
}

template <size_t W>
const PauliStringRef<W> TableauHalf<W>::operator[](size_t input_qubit) const {
    size_t nw = (num_qubits + W - 1) / W;
    return PauliStringRef<W>(
        num_qubits, signs[input_qubit], xt[input_qubit].word_range_ref(0, nw), zt[input_qubit].word_range_ref(0, nw));
}

template <size_t W>
PauliString<W> Tableau<W>::eval_y_obs(size_t qubit) const {
    PauliString<W> result(xs[qubit]);
    uint8_t log_i = result.ref().inplace_right_mul_returning_log_i_scalar(zs[qubit]);
    log_i++;
    assert((log_i & 1) == 0);
    if (log_i & 2) {
        result.sign ^= true;
    }
    return result;
}

template <size_t W>
Tableau<W>::Tableau(size_t num_qubits) : num_qubits(num_qubits), xs(num_qubits), zs(num_qubits) {
    for (size_t q = 0; q < num_qubits; q++) {
        xs.xt[q][q] = true;
        zs.zt[q][q] = true;
    }
}

template <size_t W>
TableauHalf<W>::TableauHalf(size_t num_qubits)
    : num_qubits(num_qubits), xt(num_qubits, num_qubits), zt(num_qubits, num_qubits), signs(num_qubits) {
}

template <size_t W>
Tableau<W> Tableau<W>::identity(size_t num_qubits) {
    return Tableau<W>(num_qubits);
}

template <size_t W>
Tableau<W> Tableau<W>::from_pauli_string(const PauliString<W> &pauli_string) {
    Tableau<W> tableau = identity(pauli_string.num_qubits);
    tableau.xs.signs = pauli_string.zs;
    tableau.zs.signs = pauli_string.xs;
    return tableau;
}

template <size_t W>
Tableau<W> Tableau<W>::gate1(const char *x, const char *z) {
    Tableau<W> result(1);
    result.xs[0] = PauliString<W>::from_str(x);
    result.zs[0] = PauliString<W>::from_str(z);
    assert((bool)result.zs[0].sign == (z[0] == '-'));
    return result;
}

template <size_t W>
Tableau<W> Tableau<W>::gate2(const char *x1, const char *z1, const char *x2, const char *z2) {
    Tableau<W> result(2);
    result.xs[0] = PauliString<W>::from_str(x1);
    result.zs[0] = PauliString<W>::from_str(z1);
    result.xs[1] = PauliString<W>::from_str(x2);
    result.zs[1] = PauliString<W>::from_str(z2);
    return result;
}

template <size_t W>
std::ostream &operator<<(std::ostream &out, const Tableau<W> &t) {
    out << "+-";
    for (size_t k = 0; k < t.num_qubits; k++) {
        out << 'x';
        out << 'z';
        out << '-';
    }
    out << "\n|";
    for (size_t k = 0; k < t.num_qubits; k++) {
        out << ' ';
        out << "+-"[t.xs[k].sign];
        out << "+-"[t.zs[k].sign];
    }
    for (size_t q = 0; q < t.num_qubits; q++) {
        out << "\n|";
        for (size_t k = 0; k < t.num_qubits; k++) {
            out << ' ';
            auto x = t.xs[k];
            auto z = t.zs[k];
            out << "_XZY"[x.xs[q] + 2 * x.zs[q]];
            out << "_XZY"[z.xs[q] + 2 * z.zs[q]];
        }
    }
    return out;
}

template <size_t W>
std::string Tableau<W>::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

template <size_t W>
void Tableau<W>::inplace_scatter_append(const Tableau<W> &operation, const std::vector<size_t> &target_qubits) {
    assert(operation.num_qubits == target_qubits.size());
    if (&operation == this) {
        Tableau<W> independent_copy(operation);
        inplace_scatter_append(independent_copy, target_qubits);
        return;
    }
    for (size_t q = 0; q < num_qubits; q++) {
        auto x = xs[q];
        auto z = zs[q];
        operation.apply_within(x, target_qubits);
        operation.apply_within(z, target_qubits);
    }
}

template <size_t W>
bool truncated_bits_equals(size_t nw, const simd_bits_range_ref<W> &t1, const simd_bits_range_ref<W> &t2) {
    return t1.word_range_ref(0, nw) == t2.word_range_ref(0, nw);
}

template <size_t W>
bool truncated_tableau_equals(size_t n, const simd_bit_table<W> &t1, const simd_bit_table<W> &t2) {
    size_t nw = (n + W - 1) / W;
    for (size_t k = 0; k < n; k++) {
        if (!truncated_bits_equals(nw, t1[k], t2[k])) {
            return false;
        }
    }
    return true;
}

template <size_t W>
bool Tableau<W>::operator==(const Tableau<W> &other) const {
    size_t nw = (num_qubits + W - 1) / W;
    return num_qubits == other.num_qubits && truncated_tableau_equals(num_qubits, xs.xt, other.xs.xt) &&
           truncated_tableau_equals(num_qubits, xs.zt, other.xs.zt) &&
           truncated_tableau_equals(num_qubits, zs.xt, other.zs.xt) &&
           truncated_tableau_equals(num_qubits, zs.zt, other.zs.zt) &&
           xs.signs.word_range_ref(0, nw) == other.xs.signs.word_range_ref(0, nw) &&
           zs.signs.word_range_ref(0, nw) == other.zs.signs.word_range_ref(0, nw);
}

template <size_t W>
bool Tableau<W>::operator!=(const Tableau<W> &other) const {
    return !(*this == other);
}

template <size_t W>
void Tableau<W>::inplace_scatter_prepend(const Tableau<W> &operation, const std::vector<size_t> &target_qubits) {
    assert(operation.num_qubits == target_qubits.size());
    if (&operation == this) {
        Tableau<W> independent_copy(operation);
        inplace_scatter_prepend(independent_copy, target_qubits);
        return;
    }

    std::vector<PauliString<W>> new_x;
    std::vector<PauliString<W>> new_z;
    new_x.reserve(operation.num_qubits);
    new_z.reserve(operation.num_qubits);
    for (size_t q = 0; q < operation.num_qubits; q++) {
        new_x.push_back(scatter_eval(operation.xs[q], target_qubits));
        new_z.push_back(scatter_eval(operation.zs[q], target_qubits));
    }
    for (size_t q = 0; q < operation.num_qubits; q++) {
        xs[target_qubits[q]] = new_x[q];
        zs[target_qubits[q]] = new_z[q];
    }
}

template <size_t W>
PauliString<W> Tableau<W>::scatter_eval(
    const PauliStringRef<W> &gathered_input, const std::vector<size_t> &scattered_indices) const {
    assert(gathered_input.num_qubits == scattered_indices.size());
    auto result = PauliString<W>(num_qubits);
    result.sign = gathered_input.sign;
    for (size_t k_gathered = 0; k_gathered < gathered_input.num_qubits; k_gathered++) {
        size_t k_scattered = scattered_indices[k_gathered];
        bool x = gathered_input.xs[k_gathered];
        bool z = gathered_input.zs[k_gathered];
        if (x) {
            if (z) {
                // Multiply by Y using Y = i*X*Z.
                uint8_t log_i = 1;
                log_i += result.ref().inplace_right_mul_returning_log_i_scalar(xs[k_scattered]);
                log_i += result.ref().inplace_right_mul_returning_log_i_scalar(zs[k_scattered]);
                assert((log_i & 1) == 0);
                result.sign ^= (log_i & 2) != 0;
            } else {
                result.ref() *= xs[k_scattered];
            }
        } else if (z) {
            result.ref() *= zs[k_scattered];
        }
    }
    return result;
}

template <size_t W>
PauliString<W> Tableau<W>::operator()(const PauliStringRef<W> &p) const {
    if (p.num_qubits != num_qubits) {
        throw std::out_of_range("pauli_string.num_qubits != tableau.num_qubits");
    }
    std::vector<size_t> indices;
    for (size_t k = 0; k < p.num_qubits; k++) {
        indices.push_back(k);
    }
    return scatter_eval(p, indices);
}

template <size_t W>
void Tableau<W>::apply_within(PauliStringRef<W> &target, SpanRef<const size_t> target_qubits) const {
    assert(num_qubits == target_qubits.size());
    auto inp = PauliString<W>(num_qubits);
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
inline std::pair<std::vector<bool>, std::vector<size_t>> sample_qmallows(size_t n, std::mt19937_64 &gen) {
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
template <size_t W>
simd_bit_table<W> random_stabilizer_tableau_raw(size_t n, std::mt19937_64 &rng) {
    auto hs_pair = sample_qmallows(n, rng);
    const auto &hada = hs_pair.first;
    const auto &perm = hs_pair.second;

    simd_bit_table<W> symmetric(n, n);
    for (size_t row = 0; row < n; row++) {
        symmetric[row].randomize(row + 1, rng);
        for (size_t col = 0; col < row; col++) {
            symmetric[col][row] = symmetric[row][col];
        }
    }

    simd_bit_table<W> symmetric_m(n, n);
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

    auto lower = simd_bit_table<W>::identity(n);
    for (size_t row = 0; row < n; row++) {
        lower[row].randomize(row, rng);
    }

    auto lower_m = simd_bit_table<W>::identity(n);
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

    auto fused = simd_bit_table<W>::from_quadrants(n, lower, simd_bit_table<W>(n, n), prod, inv);
    auto fused_m = simd_bit_table<W>::from_quadrants(n, lower_m, simd_bit_table<W>(n, n), prod_m, inv_m);

    simd_bit_table<W> u(2 * n, 2 * n);

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

    return fused_m.square_mat_mul(u, 2 * n);
}

template <size_t W>
Tableau<W> Tableau<W>::random(size_t num_qubits, std::mt19937_64 &rng) {
    auto raw = random_stabilizer_tableau_raw<W>(num_qubits, rng);
    Tableau result(num_qubits);
    for (size_t row = 0; row < num_qubits; row++) {
        for (size_t col = 0; col < num_qubits; col++) {
            result.xs[row].xs[col] = raw[row][col];
            result.xs[row].zs[col] = raw[row][col + num_qubits];
            result.zs[row].xs[col] = raw[row + num_qubits][col];
            result.zs[row].zs[col] = raw[row + num_qubits][col + num_qubits];
        }
    }
    result.xs.signs.randomize(num_qubits, rng);
    result.zs.signs.randomize(num_qubits, rng);
    return result;
}

template <size_t W>
bool Tableau<W>::satisfies_invariants() const {
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

template <size_t W>
bool Tableau<W>::is_pauli_product() const {
    size_t pop_count = xs.xt.data.popcnt() + xs.zt.data.popcnt() + zs.xt.data.popcnt() + zs.zt.data.popcnt();

    if (pop_count != 2 * num_qubits) {
        return false;
    }

    for (size_t q = 0; q < num_qubits; q++) {
        if (xs.xt[q][q] == false)
            return false;
    }

    for (size_t q = 0; q < num_qubits; q++) {
        if (zs.zt[q][q] == false)
            return false;
    }

    return true;
}

template <size_t W>
PauliString<W> Tableau<W>::to_pauli_string() const {
    if (!is_pauli_product()) {
        throw std::invalid_argument("The Tableau isn't equivalent to a Pauli product.");
    }

    PauliString<W> pauli_string(num_qubits);
    pauli_string.xs = zs.signs;
    pauli_string.zs = xs.signs;
    return pauli_string;
}

template <size_t W>
Tableau<W> Tableau<W>::inverse(bool skip_signs) const {
    Tableau<W> result(xs.xt.num_major_bits_padded());
    result.num_qubits = num_qubits;
    result.xs.num_qubits = num_qubits;
    result.zs.num_qubits = num_qubits;

    // Transpose data with xx zz swap tweak.
    result.xs.xt.data = zs.zt.data;
    result.xs.zt.data = xs.zt.data;
    result.zs.xt.data = zs.xt.data;
    result.zs.zt.data = xs.xt.data;
    result.do_transpose_quadrants();

    // Fix signs by checking for consistent round trips.
    if (!skip_signs) {
        PauliString<W> singleton(num_qubits);
        for (size_t k = 0; k < num_qubits; k++) {
            singleton.xs[k] = true;
            bool x_round_trip_sign = (*this)(result(singleton)).sign;
            singleton.xs[k] = false;
            singleton.zs[k] = true;
            bool z_round_trip_sign = (*this)(result(singleton)).sign;
            singleton.zs[k] = false;

            result.xs[k].sign ^= x_round_trip_sign;
            result.zs[k].sign ^= z_round_trip_sign;
        }
    }

    return result;
}

template <size_t W>
void Tableau<W>::do_transpose_quadrants() {
    xs.xt.do_square_transpose();
    xs.zt.do_square_transpose();
    zs.xt.do_square_transpose();
    zs.zt.do_square_transpose();
}

template <size_t W>
Tableau<W> Tableau<W>::then(const Tableau<W> &second) const {
    assert(num_qubits == second.num_qubits);
    Tableau<W> result(num_qubits);
    for (size_t q = 0; q < num_qubits; q++) {
        result.xs[q] = second(xs[q]);
        result.zs[q] = second(zs[q]);
    }
    return result;
}

template <size_t W>
Tableau<W> Tableau<W>::raised_to(int64_t exponent) const {
    Tableau<W> result(num_qubits);
    if (exponent) {
        Tableau<W> square = *this;

        if (exponent < 0) {
            square = square.inverse();
            exponent *= -1;
        }

        while (true) {
            if (exponent & 1) {
                result = result.then(square);
            }
            exponent >>= 1;
            if (exponent == 0) {
                break;
            }
            square = square.then(square);
        }
    }
    return result;
}

template <size_t W>
Tableau<W> Tableau<W>::operator+(const Tableau<W> &second) const {
    Tableau<W> copy = *this;
    copy += second;
    return copy;
}

template <size_t W>
Tableau<W> &Tableau<W>::operator+=(const Tableau<W> &second) {
    size_t n = num_qubits;
    expand(n + second.num_qubits, 1.1);
    for (size_t i = 0; i < second.num_qubits; i++) {
        xs.signs[n + i] = second.xs.signs[i];
        zs.signs[n + i] = second.zs.signs[i];
        for (size_t j = 0; j < second.num_qubits; j++) {
            xs.xt[n + i][n + j] = second.xs.xt[i][j];
            xs.zt[n + i][n + j] = second.xs.zt[i][j];
            zs.xt[n + i][n + j] = second.zs.xt[i][j];
            zs.zt[n + i][n + j] = second.zs.zt[i][j];
        }
    }
    return *this;
}

template <size_t W>
uint8_t Tableau<W>::x_output_pauli_xyz(size_t input_index, size_t output_index) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    if (output_index >= num_qubits) {
        throw std::invalid_argument("output_index >= len(tableau)");
    }
    PauliStringRef<W> x = xs[input_index];
    return pauli_xz_to_xyz(x.xs[output_index], x.zs[output_index]);
}

template <size_t W>
uint8_t Tableau<W>::y_output_pauli_xyz(size_t input_index, size_t output_index) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    if (output_index >= num_qubits) {
        throw std::invalid_argument("output_index >= len(tableau)");
    }
    PauliStringRef<W> x = xs[input_index];
    PauliStringRef<W> z = zs[input_index];
    return pauli_xz_to_xyz(x.xs[output_index] ^ z.xs[output_index], x.zs[output_index] ^ z.zs[output_index]);
}

template <size_t W>
uint8_t Tableau<W>::z_output_pauli_xyz(size_t input_index, size_t output_index) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    if (output_index >= num_qubits) {
        throw std::invalid_argument("output_index >= len(tableau)");
    }
    PauliStringRef<W> z = zs[input_index];
    return pauli_xz_to_xyz(z.xs[output_index], z.zs[output_index]);
}

template <size_t W>
uint8_t Tableau<W>::inverse_x_output_pauli_xyz(size_t input_index, size_t output_index) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    if (output_index >= num_qubits) {
        throw std::invalid_argument("output_index >= len(tableau)");
    }
    return pauli_xz_to_xyz(zs[output_index].zs[input_index], xs[output_index].zs[input_index]);
}

template <size_t W>
uint8_t Tableau<W>::inverse_y_output_pauli_xyz(size_t input_index, size_t output_index) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    if (output_index >= num_qubits) {
        throw std::invalid_argument("output_index >= len(tableau)");
    }
    PauliStringRef<W> x = xs[output_index];
    PauliStringRef<W> z = zs[output_index];
    return pauli_xz_to_xyz(z.zs[input_index] ^ z.xs[input_index], x.zs[input_index] ^ x.xs[input_index]);
}

template <size_t W>
uint8_t Tableau<W>::inverse_z_output_pauli_xyz(size_t input_index, size_t output_index) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    if (output_index >= num_qubits) {
        throw std::invalid_argument("output_index >= len(tableau)");
    }
    return pauli_xz_to_xyz(zs[output_index].xs[input_index], xs[output_index].xs[input_index]);
}

template <size_t W>
PauliString<W> Tableau<W>::inverse_x_output(size_t input_index, bool skip_sign) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    PauliString<W> result(num_qubits);
    for (size_t k = 0; k < num_qubits; k++) {
        result.xs[k] = zs[k].zs[input_index];
        result.zs[k] = xs[k].zs[input_index];
    }
    if (!skip_sign) {
        result.sign = (*this)(result).sign;
    }
    return result;
}

template <size_t W>
PauliString<W> Tableau<W>::inverse_y_output(size_t input_index, bool skip_sign) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    PauliString<W> result(num_qubits);
    for (size_t k = 0; k < num_qubits; k++) {
        result.xs[k] = zs[k].zs[input_index] ^ zs[k].xs[input_index];
        result.zs[k] = xs[k].zs[input_index] ^ xs[k].xs[input_index];
    }
    if (!skip_sign) {
        result.sign = (*this)(result).sign;
    }
    return result;
}

template <size_t W>
PauliString<W> Tableau<W>::inverse_z_output(size_t input_index, bool skip_sign) const {
    if (input_index >= num_qubits) {
        throw std::invalid_argument("input_index >= len(tableau)");
    }
    PauliString<W> result(num_qubits);
    for (size_t k = 0; k < num_qubits; k++) {
        result.xs[k] = zs[k].xs[input_index];
        result.zs[k] = xs[k].xs[input_index];
    }
    if (!skip_sign) {
        result.sign = (*this)(result).sign;
    }
    return result;
}

template <size_t W>
std::vector<std::complex<float>> Tableau<W>::to_flat_unitary_matrix(bool little_endian) const {
    std::vector<PauliString<W>> pauli_strings;
    size_t nw = xs[0].xs.num_simd_words;

    // Add X transformation stabilizers.
    for (size_t k = 0; k < num_qubits; k++) {
        PauliString<W> p(num_qubits * 2);
        p.xs.word_range_ref(0, nw) = xs[k].xs;
        p.zs.word_range_ref(0, nw) = xs[k].zs;
        p.sign = xs[k].sign;
        p.xs[num_qubits + k] ^= true;
        pauli_strings.push_back(p);
    }

    // Add Z transformation stabilizers.
    for (size_t k = 0; k < num_qubits; k++) {
        PauliString<W> p(num_qubits * 2);
        p.xs.word_range_ref(0, nw) = zs[k].xs;
        p.zs.word_range_ref(0, nw) = zs[k].zs;
        p.sign = zs[k].sign;
        p.zs[num_qubits + k] ^= true;
        pauli_strings.push_back(p);
    }

    for (auto &p : pauli_strings) {
        for (size_t q = 0; q < num_qubits - q - 1; q++) {
            size_t q2 = num_qubits - q - 1;
            if (!little_endian) {
                p.xs[q].swap_with(p.xs[q2]);
                p.zs[q].swap_with(p.zs[q2]);
                p.xs[q + num_qubits].swap_with(p.xs[q2 + num_qubits]);
                p.zs[q + num_qubits].swap_with(p.zs[q2 + num_qubits]);
            }
        }
        for (size_t q = 0; q < num_qubits; q++) {
            p.xs[q].swap_with(p.xs[q + num_qubits]);
            p.zs[q].swap_with(p.zs[q + num_qubits]);
        }
    }

    // Turn it into a vector.
    std::vector<PauliStringRef<W>> refs;
    for (const auto &e : pauli_strings) {
        refs.push_back(e.ref());
    }

    return VectorSimulator::state_vector_from_stabilizers<W>(refs, 1 << num_qubits);
}

template <size_t W>
PauliString<W> Tableau<W>::y_output(size_t input_index) const {
    uint8_t log_i = 1;
    PauliString<W> result = xs[input_index];
    log_i += result.ref().inplace_right_mul_returning_log_i_scalar(zs[input_index]);
    assert((log_i & 1) == 0);
    result.sign ^= (log_i & 2) != 0;
    return result;
}

template <size_t W>
std::vector<PauliString<W>> Tableau<W>::stabilizers(bool canonical) const {
    std::vector<PauliString<W>> stabilizers;
    for (size_t k = 0; k < num_qubits; k++) {
        stabilizers.push_back(zs[k]);
    }

    if (canonical) {
        size_t min_pivot = 0;
        for (size_t q = 0; q < num_qubits; q++) {
            for (size_t b = 0; b < 2; b++) {
                size_t pivot = min_pivot;
                while (pivot < num_qubits && !(b ? stabilizers[pivot].zs : stabilizers[pivot].xs)[q]) {
                    pivot++;
                }
                if (pivot == num_qubits) {
                    continue;
                }
                for (size_t s = 0; s < num_qubits; s++) {
                    if (s != pivot && (b ? stabilizers[s].zs : stabilizers[s].xs)[q]) {
                        stabilizers[s].ref() *= stabilizers[pivot];
                    }
                }
                if (min_pivot != pivot) {
                    std::swap(stabilizers[min_pivot], stabilizers[pivot]);
                }
                min_pivot += 1;
            }
        }
    }

    return stabilizers;
}

}  // namespace stim
