#include <cassert>

#include "tableau.h"

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

void Tableau::prepend_H_XZ(const size_t q) {
    xs[q].swap_with(zs[q]);
}

void Tableau::prepend_H_YZ(const size_t q) {
    PauliStringRef x = xs[q];
    PauliStringRef z = zs[q];
    uint8_t m = z.inplace_right_mul_returning_log_i_scalar(x);
    x.sign ^= 1;
    z.sign ^= m & 2;
}

void Tableau::prepend_H_XY(const size_t q) {
    PauliStringRef x = xs[q];
    PauliStringRef z = zs[q];
    uint8_t m = x.inplace_right_mul_returning_log_i_scalar(z);
    z.sign ^= 1;
    x.sign ^= !(m & 2);
}

void Tableau::prepend_SQRT_X(size_t q) {
    PauliStringRef z = zs[q];
    uint8_t m = z.inplace_right_mul_returning_log_i_scalar(xs[q]);
    z.sign ^= !(m & 2);
}

void Tableau::prepend_SQRT_X_DAG(size_t q) {
    PauliStringRef z = zs[q];
    uint8_t m = z.inplace_right_mul_returning_log_i_scalar(xs[q]);
    z.sign ^= m & 2;
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
    PauliStringRef x = xs[q];
    uint8_t m = x.inplace_right_mul_returning_log_i_scalar(zs[q]);
    x.sign ^= !(m & 2);
}

void Tableau::prepend_SQRT_Z_DAG(size_t q) {
    PauliStringRef x = xs[q];
    uint8_t m = x.inplace_right_mul_returning_log_i_scalar(zs[q]);
    x.sign ^= m & 2;
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
