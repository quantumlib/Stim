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

#include "stim/stabilizers/tableau_transposed_raii.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <thread>

#include "stim/stabilizers/pauli_string.h"

using namespace stim;

TableauTransposedRaii::TableauTransposedRaii(Tableau &tableau) : tableau(tableau) {
    tableau.do_transpose_quadrants();
}

TableauTransposedRaii::~TableauTransposedRaii() {
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
template <typename FUNC>
inline void for_each_trans_obs(TableauTransposedRaii &trans, size_t q, FUNC body) {
    for (size_t k = 0; k < 2; k++) {
        TableauHalf &h = k == 0 ? trans.tableau.xs : trans.tableau.zs;
        PauliStringRef p = h[q];
        p.xs.for_each_word(p.zs, h.signs, body);
    }
}

template <typename FUNC>
inline void for_each_trans_obs(TableauTransposedRaii &trans, size_t q1, size_t q2, FUNC body) {
    for (size_t k = 0; k < 2; k++) {
        TableauHalf &h = k == 0 ? trans.tableau.xs : trans.tableau.zs;
        PauliStringRef p1 = h[q1];
        PauliStringRef p2 = h[q2];
        p1.xs.for_each_word(p1.zs, p2.xs, p2.zs, h.signs, body);
    }
}

void TableauTransposedRaii::append_ZCX(size_t control, size_t target) {
    for_each_trans_obs(
        *this, control, target, [](simd_word &cx, simd_word &cz, simd_word &tx, simd_word &tz, simd_word &s) {
            s ^= (cz ^ tx).andnot(cx & tz);
            cz ^= tz;
            tx ^= cx;
        });
}

void TableauTransposedRaii::append_ZCY(size_t control, size_t target) {
    for_each_trans_obs(
        *this, control, target, [](simd_word &cx, simd_word &cz, simd_word &tx, simd_word &tz, simd_word &s) {
            cz ^= tx;
            s ^= cx & cz & (tx ^ tz);
            cz ^= tz;
            tx ^= cx;
            tz ^= cx;
        });
}

void TableauTransposedRaii::append_ZCZ(size_t control, size_t target) {
    for_each_trans_obs(
        *this, control, target, [](simd_word &cx, simd_word &cz, simd_word &tx, simd_word &tz, simd_word &s) {
            s ^= cx & tx & (cz ^ tz);
            cz ^= tx;
            tz ^= cx;
        });
}

void TableauTransposedRaii::append_SWAP(size_t q1, size_t q2) {
    for_each_trans_obs(*this, q1, q2, [](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2, simd_word &s) {
        std::swap(x1, x2);
        std::swap(z1, z2);
    });
}

void TableauTransposedRaii::append_H_XY(size_t target) {
    for_each_trans_obs(*this, target, [](simd_word &x, simd_word &z, simd_word &s) {
        s ^= x.andnot(z);
        z ^= x;
    });
}

void TableauTransposedRaii::append_H_YZ(size_t target) {
    for_each_trans_obs(*this, target, [](simd_word &x, simd_word &z, simd_word &s) {
        s ^= z.andnot(x);
        x ^= z;
    });
}

void TableauTransposedRaii::append_S(size_t target) {
    for_each_trans_obs(*this, target, [](simd_word &x, simd_word &z, simd_word &s) {
        s ^= x & z;
        z ^= x;
    });
}

void TableauTransposedRaii::append_H_XZ(size_t q) {
    for_each_trans_obs(*this, q, [](simd_word &x, simd_word &z, simd_word &s) {
        std::swap(x, z);
        s ^= x & z;
    });
}

void TableauTransposedRaii::append_X(size_t target) {
    for_each_trans_obs(*this, target, [](simd_word &x, simd_word &z, simd_word &s) {
        s ^= z;
    });
}

PauliString TableauTransposedRaii::unsigned_x_input(size_t q) const {
    PauliString result(tableau.num_qubits);
    result.xs = tableau.zs[q].zs;
    result.zs = tableau.xs[q].zs;
    return result;
}
