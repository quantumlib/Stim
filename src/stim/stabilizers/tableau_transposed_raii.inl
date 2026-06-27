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

#include <cstring>
#include <map>

#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau_transposed_raii.h"

namespace stim {

template <size_t W>
TableauTransposedRaii<W>::TableauTransposedRaii(Tableau<W> &tableau) : tableau(tableau) {
    tableau.do_transpose_quadrants();
}

template <size_t W>
TableauTransposedRaii<W>::~TableauTransposedRaii() {
    tableau.do_transpose_quadrants();
}

/// Iterates over the Paulis in a row of the tableau.
///
/// Args:
///     trans: The transposed tableau (where rows are contiguous in memory and so operations can be done efficiently).
///     q: The row to iterate over.
///     body: A function taking X, Z, and SIGN words.
///         The X and Z words are chunks of xz-encoded Paulis from the row.
///         The SIGN word is the corresponding chunk of sign bits from the sign row.
template <size_t W, typename FUNC>
inline void for_each_trans_obs(TableauTransposedRaii<W> &trans, size_t q, FUNC body) {
    for (size_t k = 0; k < 2; k++) {
        TableauHalf<W> &h = k == 0 ? trans.tableau.xs : trans.tableau.zs;
        PauliStringRef<W> p = h[q];
        p.xs.for_each_word(p.zs, h.signs, body);
    }
}

template <size_t W, typename FUNC>
inline void for_each_trans_obs(TableauTransposedRaii<W> &trans, size_t q1, size_t q2, FUNC body) {
    for (size_t k = 0; k < 2; k++) {
        TableauHalf<W> &h = k == 0 ? trans.tableau.xs : trans.tableau.zs;
        PauliStringRef<W> p1 = h[q1];
        PauliStringRef<W> p2 = h[q2];
        p1.xs.for_each_word(p1.zs, p2.xs, p2.zs, h.signs, body);
    }
}

template <size_t W>
void TableauTransposedRaii<W>::append_ZCX(size_t control, size_t target) {
    for_each_trans_obs<W>(
        *this,
        control,
        target,
        [](simd_word<W> &cx, simd_word<W> &cz, simd_word<W> &tx, simd_word<W> &tz, simd_word<W> &s) {
            s ^= (cz ^ tx).andnot(cx & tz);
            cz ^= tz;
            tx ^= cx;
        });
}

template <size_t W>
void TableauTransposedRaii<W>::append_ZCY(size_t control, size_t target) {
    for_each_trans_obs<W>(
        *this,
        control,
        target,
        [](simd_word<W> &cx, simd_word<W> &cz, simd_word<W> &tx, simd_word<W> &tz, simd_word<W> &s) {
            cz ^= tx;
            s ^= cx & cz & (tx ^ tz);
            cz ^= tz;
            tx ^= cx;
            tz ^= cx;
        });
}

template <size_t W>
void TableauTransposedRaii<W>::append_ZCZ(size_t control, size_t target) {
    for_each_trans_obs<W>(
        *this,
        control,
        target,
        [](simd_word<W> &cx, simd_word<W> &cz, simd_word<W> &tx, simd_word<W> &tz, simd_word<W> &s) {
            s ^= cx & tx & (cz ^ tz);
            cz ^= tx;
            tz ^= cx;
        });
}

template <size_t W>
void TableauTransposedRaii<W>::append_SWAP(size_t q1, size_t q2) {
    for_each_trans_obs<W>(
        *this, q1, q2, [](simd_word<W> &x1, simd_word<W> &z1, simd_word<W> &x2, simd_word<W> &z2, simd_word<W> &s) {
            std::swap(x1, x2);
            std::swap(z1, z2);
        });
}

template <size_t W>
void TableauTransposedRaii<W>::append_H_XY(size_t target) {
    for_each_trans_obs<W>(*this, target, [](simd_word<W> &x, simd_word<W> &z, simd_word<W> &s) {
        s ^= x.andnot(z);
        z ^= x;
    });
}

template <size_t W>
void TableauTransposedRaii<W>::append_H_YZ(size_t target) {
    for_each_trans_obs<W>(*this, target, [](simd_word<W> &x, simd_word<W> &z, simd_word<W> &s) {
        s ^= z.andnot(x);
        x ^= z;
    });
}

template <size_t W>
void TableauTransposedRaii<W>::append_S(size_t target) {
    for_each_trans_obs<W>(*this, target, [](simd_word<W> &x, simd_word<W> &z, simd_word<W> &s) {
        s ^= x & z;
        z ^= x;
    });
}

template <size_t W>
void TableauTransposedRaii<W>::append_H_XZ(size_t q) {
    for_each_trans_obs<W>(*this, q, [](simd_word<W> &x, simd_word<W> &z, simd_word<W> &s) {
        std::swap(x, z);
        s ^= x & z;
    });
}

template <size_t W>
void TableauTransposedRaii<W>::append_X(size_t target) {
    for_each_trans_obs<W>(*this, target, [](simd_word<W> &x, simd_word<W> &z, simd_word<W> &s) {
        s ^= z;
    });
}

template <size_t W>
PauliString<W> TableauTransposedRaii<W>::unsigned_x_input(size_t q) const {
    PauliString<W> result(tableau.num_qubits);
    result.xs = tableau.zs[q].zs;
    result.zs = tableau.xs[q].zs;
    return result;
}

}  // namespace stim
