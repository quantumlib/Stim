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

using namespace stim;

void Tableau::prepend_X(size_t q) {
    zs[q].sign ^= 1;
}

void Tableau::prepend_Y(size_t q) {
    xs[q].sign ^= 1;
    zs[q].sign ^= 1;
}

void Tableau::prepend_Z(size_t q) {
    xs[q].sign ^= 1;
}

void Tableau::prepend(const PauliStringRef &op) {
    assert(op.num_qubits == num_qubits);
    zs.signs ^= op.xs;
    xs.signs ^= op.zs;
}

struct IgnoreAntiCommute {
    PauliStringRef rhs;
    IgnoreAntiCommute(PauliStringRef rhs) : rhs(rhs) {
    }
};
void operator*=(PauliStringRef lhs, const IgnoreAntiCommute &rhs) {
    lhs.sign ^= 2 & lhs.inplace_right_mul_returning_log_i_scalar(rhs.rhs);
}

void Tableau::prepend_H_XZ(const size_t q) {
    xs[q].swap_with(zs[q]);
}

void Tableau::prepend_H_YZ(const size_t q) {
    zs[q] *= IgnoreAntiCommute(xs[q]);
    prepend_Z(q);
}

void Tableau::prepend_H_XY(const size_t q) {
    xs[q] *= IgnoreAntiCommute(zs[q]);
    prepend_Y(q);
}

void Tableau::prepend_C_XYZ(const size_t q) {
    PauliStringRef x = xs[q];
    PauliStringRef z = zs[q];
    z *= IgnoreAntiCommute(x);
    x.swap_with(z);
}

void Tableau::prepend_C_ZYX(const size_t q) {
    PauliStringRef x = xs[q];
    PauliStringRef z = zs[q];
    x.swap_with(z);
    z *= IgnoreAntiCommute(x);
    prepend_X(q);
}

void Tableau::prepend_SQRT_X(size_t q) {
    prepend_SQRT_X_DAG(q);
    prepend_X(q);
}

void Tableau::prepend_SQRT_X_DAG(size_t q) {
    zs[q] *= IgnoreAntiCommute(xs[q]);
}

void Tableau::prepend_SQRT_Y(size_t q) {
    PauliStringRef z = zs[q];
    z.sign ^= 1;
    xs[q].swap_with(z);
}

void Tableau::prepend_SQRT_Y_DAG(size_t q) {
    PauliStringRef z = zs[q];
    xs[q].swap_with(z);
    z.sign ^= 1;
}

void Tableau::prepend_SQRT_Z(size_t q) {
    prepend_SQRT_Z_DAG(q);
    prepend_Z(q);
}

void Tableau::prepend_SQRT_Z_DAG(size_t q) {
    xs[q] *= IgnoreAntiCommute(zs[q]);
}

void Tableau::prepend_SWAP(size_t q1, size_t q2) {
    zs[q1].swap_with(zs[q2]);
    xs[q1].swap_with(xs[q2]);
}

void Tableau::prepend_ISWAP(size_t q1, size_t q2) {
    prepend_SWAP(q1, q2);
    prepend_ZCZ(q1, q2);
    prepend_SQRT_Z(q1);
    prepend_SQRT_Z(q2);
}

void Tableau::prepend_ISWAP_DAG(size_t q1, size_t q2) {
    prepend_SWAP(q1, q2);
    prepend_ZCZ(q1, q2);
    prepend_SQRT_Z_DAG(q1);
    prepend_SQRT_Z_DAG(q2);
}

void Tableau::prepend_ZCX(size_t control, size_t target) {
    zs[target] *= zs[control];
    xs[control] *= xs[target];
}

void Tableau::prepend_ZCY(size_t control, size_t target) {
    prepend_H_YZ(target);
    prepend_ZCZ(control, target);
    prepend_H_YZ(target);
}

void Tableau::prepend_ZCZ(size_t control, size_t target) {
    xs[target] *= zs[control];
    xs[control] *= zs[target];
}

void Tableau::prepend_XCX(size_t control, size_t target) {
    zs[target] *= xs[control];
    zs[control] *= xs[target];
}

void Tableau::prepend_SQRT_XX(size_t q1, size_t q2) {
    prepend_SQRT_XX_DAG(q1, q2);
    prepend_X(q1);
    prepend_X(q2);
}

void Tableau::prepend_SQRT_XX_DAG(size_t q1, size_t q2) {
    zs[q1] *= IgnoreAntiCommute(xs[q1]);
    zs[q1] *= IgnoreAntiCommute(xs[q2]);
    zs[q2] *= IgnoreAntiCommute(xs[q1]);
    zs[q2] *= IgnoreAntiCommute(xs[q2]);
}

void Tableau::prepend_SQRT_YY(size_t q1, size_t q2) {
    prepend_SQRT_YY_DAG(q1, q2);
    prepend_Y(q1);
    prepend_Y(q2);
}

void Tableau::prepend_SQRT_YY_DAG(size_t q1, size_t q2) {
    auto z1 = zs[q1];
    auto z2 = zs[q2];
    auto x1 = xs[q1];
    auto x2 = xs[q2];

    x1 *= IgnoreAntiCommute(z1);
    z1 *= IgnoreAntiCommute(z2);
    z1 *= IgnoreAntiCommute(x2);
    x2 *= IgnoreAntiCommute(x1);
    z2 *= IgnoreAntiCommute(x1);
    x1 *= IgnoreAntiCommute(z1);
    x1.swap_with(z1);
    x2.swap_with(z2);

    prepend_Z(q2);
}

void Tableau::prepend_SQRT_ZZ(size_t q1, size_t q2) {
    prepend_SQRT_ZZ_DAG(q1, q2);
    prepend_Z(q1);
    prepend_Z(q2);
}

void Tableau::prepend_SQRT_ZZ_DAG(size_t q1, size_t q2) {
    xs[q1] *= IgnoreAntiCommute(zs[q1]);
    xs[q1] *= IgnoreAntiCommute(zs[q2]);
    xs[q2] *= IgnoreAntiCommute(zs[q1]);
    xs[q2] *= IgnoreAntiCommute(zs[q2]);
}

void Tableau::prepend_XCY(size_t control, size_t target) {
    prepend_H_XY(target);
    prepend_XCX(control, target);
    prepend_H_XY(target);
}

void Tableau::prepend_XCZ(size_t control, size_t target) {
    prepend_ZCX(target, control);
}

void Tableau::prepend_YCX(size_t control, size_t target) {
    prepend_XCY(target, control);
}

void Tableau::prepend_YCY(size_t control, size_t target) {
    prepend_H_YZ(control);
    prepend_H_YZ(target);
    prepend_ZCZ(control, target);
    prepend_H_YZ(target);
    prepend_H_YZ(control);
}

void Tableau::prepend_YCZ(size_t control, size_t target) {
    prepend_ZCY(target, control);
}
