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

#include "stim/stabilizers/tableau.h"

namespace stim {

template <size_t W>
void Tableau<W>::prepend_X(size_t q) {
    zs[q].sign ^= 1;
}

template <size_t W>
void Tableau<W>::prepend_Y(size_t q) {
    xs[q].sign ^= 1;
    zs[q].sign ^= 1;
}

template <size_t W>
void Tableau<W>::prepend_Z(size_t q) {
    xs[q].sign ^= 1;
}

template <size_t W>
void Tableau<W>::prepend_pauli_product(const PauliStringRef<W> &op) {
    assert(op.num_qubits == num_qubits);
    zs.signs ^= op.xs;
    xs.signs ^= op.zs;
}

template <size_t W>
struct IgnoreAntiCommute {
    PauliStringRef<W> rhs;
    IgnoreAntiCommute(PauliStringRef<W> rhs) : rhs(rhs) {
    }
};

template <size_t W>
void operator*=(PauliStringRef<W> lhs, const IgnoreAntiCommute<W> &rhs) {
    lhs.sign ^= 2 & lhs.inplace_right_mul_returning_log_i_scalar(rhs.rhs);
}

template <size_t W>
void Tableau<W>::prepend_H_XZ(const size_t q) {
    xs[q].swap_with(zs[q]);
}

template <size_t W>
void Tableau<W>::prepend_H_YZ(const size_t q) {
    zs[q] *= IgnoreAntiCommute<W>(xs[q]);
    prepend_Z(q);
}

template <size_t W>
void Tableau<W>::prepend_H_XY(const size_t q) {
    xs[q] *= IgnoreAntiCommute<W>(zs[q]);
    prepend_Y(q);
}

template <size_t W>
void Tableau<W>::prepend_H_NXY(const size_t q) {
    xs[q] *= IgnoreAntiCommute<W>(zs[q]);
    prepend_X(q);
}

template <size_t W>
void Tableau<W>::prepend_H_NXZ(const size_t q) {
    xs[q].swap_with(zs[q]);
    prepend_Y(q);
}

template <size_t W>
void Tableau<W>::prepend_H_NYZ(const size_t q) {
    zs[q] *= IgnoreAntiCommute<W>(xs[q]);
    prepend_Y(q);
}

template <size_t W>
void Tableau<W>::prepend_C_XYZ(const size_t q) {
    PauliStringRef<W> x = xs[q];
    PauliStringRef<W> z = zs[q];
    z *= IgnoreAntiCommute<W>(x);
    x.swap_with(z);
}

template <size_t W>
void Tableau<W>::prepend_C_NXYZ(const size_t q) {
    PauliStringRef<W> x = xs[q];
    PauliStringRef<W> z = zs[q];
    z *= IgnoreAntiCommute<W>(x);
    x.swap_with(z);
    prepend_Y(q);
}

template <size_t W>
void Tableau<W>::prepend_C_XNYZ(const size_t q) {
    PauliStringRef<W> x = xs[q];
    PauliStringRef<W> z = zs[q];
    z *= IgnoreAntiCommute<W>(x);
    x.swap_with(z);
    prepend_Z(q);
}

template <size_t W>
void Tableau<W>::prepend_C_XYNZ(const size_t q) {
    PauliStringRef<W> x = xs[q];
    PauliStringRef<W> z = zs[q];
    z *= IgnoreAntiCommute<W>(x);
    x.swap_with(z);
    prepend_X(q);
}

template <size_t W>
void Tableau<W>::prepend_C_ZYX(const size_t q) {
    PauliStringRef<W> x = xs[q];
    PauliStringRef<W> z = zs[q];
    x.swap_with(z);
    z *= IgnoreAntiCommute<W>(x);
    prepend_X(q);
}

template <size_t W>
void Tableau<W>::prepend_C_ZYNX(const size_t q) {
    PauliStringRef<W> x = xs[q];
    PauliStringRef<W> z = zs[q];
    x.swap_with(z);
    z *= IgnoreAntiCommute<W>(x);
    prepend_Y(q);
}

template <size_t W>
void Tableau<W>::prepend_C_ZNYX(const size_t q) {
    PauliStringRef<W> x = xs[q];
    PauliStringRef<W> z = zs[q];
    x.swap_with(z);
    z *= IgnoreAntiCommute<W>(x);
}

template <size_t W>
void Tableau<W>::prepend_C_NZYX(const size_t q) {
    PauliStringRef<W> x = xs[q];
    PauliStringRef<W> z = zs[q];
    x.swap_with(z);
    z *= IgnoreAntiCommute<W>(x);
    prepend_Z(q);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_X(size_t q) {
    prepend_SQRT_X_DAG(q);
    prepend_X(q);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_X_DAG(size_t q) {
    zs[q] *= IgnoreAntiCommute<W>(xs[q]);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_Y(size_t q) {
    PauliStringRef<W> z = zs[q];
    z.sign ^= 1;
    xs[q].swap_with(z);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_Y_DAG(size_t q) {
    PauliStringRef<W> z = zs[q];
    xs[q].swap_with(z);
    z.sign ^= 1;
}

template <size_t W>
void Tableau<W>::prepend_SQRT_Z(size_t q) {
    prepend_SQRT_Z_DAG(q);
    prepend_Z(q);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_Z_DAG(size_t q) {
    xs[q] *= IgnoreAntiCommute<W>(zs[q]);
}

template <size_t W>
void Tableau<W>::prepend_SWAP(size_t q1, size_t q2) {
    zs[q1].swap_with(zs[q2]);
    xs[q1].swap_with(xs[q2]);
}

template <size_t W>
void Tableau<W>::prepend_ISWAP(size_t q1, size_t q2) {
    prepend_SWAP(q1, q2);
    prepend_ZCZ(q1, q2);
    prepend_SQRT_Z(q1);
    prepend_SQRT_Z(q2);
}

template <size_t W>
void Tableau<W>::prepend_ISWAP_DAG(size_t q1, size_t q2) {
    prepend_SWAP(q1, q2);
    prepend_ZCZ(q1, q2);
    prepend_SQRT_Z_DAG(q1);
    prepend_SQRT_Z_DAG(q2);
}

template <size_t W>
void Tableau<W>::prepend_ZCX(size_t control, size_t target) {
    zs[target] *= zs[control];
    xs[control] *= xs[target];
}

template <size_t W>
void Tableau<W>::prepend_ZCY(size_t control, size_t target) {
    prepend_H_YZ(target);
    prepend_ZCZ(control, target);
    prepend_H_YZ(target);
}

template <size_t W>
void Tableau<W>::prepend_ZCZ(size_t control, size_t target) {
    xs[target] *= zs[control];
    xs[control] *= zs[target];
}

template <size_t W>
void Tableau<W>::prepend_XCX(size_t control, size_t target) {
    zs[target] *= xs[control];
    zs[control] *= xs[target];
}

template <size_t W>
void Tableau<W>::prepend_SQRT_XX(size_t q1, size_t q2) {
    prepend_SQRT_XX_DAG(q1, q2);
    prepend_X(q1);
    prepend_X(q2);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_XX_DAG(size_t q1, size_t q2) {
    zs[q1] *= IgnoreAntiCommute<W>(xs[q1]);
    zs[q1] *= IgnoreAntiCommute<W>(xs[q2]);
    zs[q2] *= IgnoreAntiCommute<W>(xs[q1]);
    zs[q2] *= IgnoreAntiCommute<W>(xs[q2]);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_YY(size_t q1, size_t q2) {
    prepend_SQRT_YY_DAG(q1, q2);
    prepend_Y(q1);
    prepend_Y(q2);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_YY_DAG(size_t q1, size_t q2) {
    auto z1 = zs[q1];
    auto z2 = zs[q2];
    auto x1 = xs[q1];
    auto x2 = xs[q2];

    x1 *= IgnoreAntiCommute<W>(z1);
    z1 *= IgnoreAntiCommute<W>(z2);
    z1 *= IgnoreAntiCommute<W>(x2);
    x2 *= IgnoreAntiCommute<W>(x1);
    z2 *= IgnoreAntiCommute<W>(x1);
    x1 *= IgnoreAntiCommute<W>(z1);
    x1.swap_with(z1);
    x2.swap_with(z2);

    prepend_Z(q2);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_ZZ(size_t q1, size_t q2) {
    prepend_SQRT_ZZ_DAG(q1, q2);
    prepend_Z(q1);
    prepend_Z(q2);
}

template <size_t W>
void Tableau<W>::prepend_SQRT_ZZ_DAG(size_t q1, size_t q2) {
    xs[q1] *= IgnoreAntiCommute<W>(zs[q1]);
    xs[q1] *= IgnoreAntiCommute<W>(zs[q2]);
    xs[q2] *= IgnoreAntiCommute<W>(zs[q1]);
    xs[q2] *= IgnoreAntiCommute<W>(zs[q2]);
}

template <size_t W>
void Tableau<W>::prepend_XCY(size_t control, size_t target) {
    prepend_H_XY(target);
    prepend_XCX(control, target);
    prepend_H_XY(target);
}

template <size_t W>
void Tableau<W>::prepend_XCZ(size_t control, size_t target) {
    prepend_ZCX(target, control);
}

template <size_t W>
void Tableau<W>::prepend_YCX(size_t control, size_t target) {
    prepend_XCY(target, control);
}

template <size_t W>
void Tableau<W>::prepend_YCY(size_t control, size_t target) {
    prepend_H_YZ(control);
    prepend_H_YZ(target);
    prepend_ZCZ(control, target);
    prepend_H_YZ(target);
    prepend_H_YZ(control);
}

template <size_t W>
void Tableau<W>::prepend_YCZ(size_t control, size_t target) {
    prepend_ZCY(target, control);
}

}  // namespace stim
