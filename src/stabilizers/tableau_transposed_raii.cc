#include <iostream>
#include <map>
#include <random>
#include <cmath>
#include <cstring>
#include <thread>
#include "pauli_string.h"
#include "tableau_transposed_raii.h"

TransposedTableauXZ TableauTransposedRaii::transposed_xz_ptr(size_t qubit) const {
    PauliStringRef x(tableau.xs[qubit]);
    PauliStringRef z(tableau.zs[qubit]);
    return {{
        {x.x_ref, x.z_ref, tableau.xs.signs},
        {z.x_ref, z.z_ref, tableau.zs.signs}
    }};
}

TableauTransposedRaii::TableauTransposedRaii(Tableau &tableau) : tableau(tableau) {
    tableau.do_transpose_quadrants();
}

TableauTransposedRaii::~TableauTransposedRaii() {
    tableau.do_transpose_quadrants();
}

template <typename BODY>
inline void for_each_trans_obs(
        TableauTransposedRaii &trans,
        size_t q,
        BODY body) {
    auto ps = trans.transposed_xz_ptr(q);
    for (size_t k = 0; k < 2; k++) {
        auto x = ps.xz[k].x.ptr_simd;
        auto z = ps.xz[k].z.ptr_simd;
        auto s = ps.xz[k].s.ptr_simd;
        auto x_end = x + ps.xz[k].x.num_simd_words;
        while (x != x_end) {
            body(*x, *z, *s);
            x++;
            z++;
            s++;
        }
    }
}

template <typename BODY>
inline void for_each_trans_obs(
        TableauTransposedRaii &trans,
        size_t q1,
        size_t q2,
        BODY body) {
    auto p1s = trans.transposed_xz_ptr(q1);
    auto p2s = trans.transposed_xz_ptr(q2);
    for (size_t k = 0; k < 2; k++) {
        auto x1 = p1s.xz[k].x.ptr_simd;
        auto z1 = p1s.xz[k].z.ptr_simd;
        auto x2 = p2s.xz[k].x.ptr_simd;
        auto z2 = p2s.xz[k].z.ptr_simd;
        auto s = p1s.xz[k].s.ptr_simd;
        auto x1_end = x1 + p1s.xz[k].x.num_simd_words;
        while (x1 != x1_end) {
            body(*x1, *z1, *x2, *z2, *s);
            x1++;
            z1++;
            x2++;
            z2++;
            s++;
        }
    }
}

void TableauTransposedRaii::append_CX(size_t control, size_t target) {
    for_each_trans_obs(*this, control, target, [](auto &cx, auto &cz, auto &tx, auto &tz, auto &s) {
        s ^= _mm256_andnot_si256(cz ^ tx, cx & tz);
        cz ^= tz;
        tx ^= cx;
    });
}

void TableauTransposedRaii::append_CY(size_t control, size_t target) {
    for_each_trans_obs(*this, control, target, [](auto &cx, auto &cz, auto &tx, auto &tz, auto &s) {
        cz ^= tx;
        s ^= cx & cz & (tx ^ tz);
        cz ^= tz;
        tx ^= cx;
        tz ^= cx;
    });
}

void TableauTransposedRaii::append_CZ(size_t control, size_t target) {
    for_each_trans_obs(*this, control, target, [](auto &cx, auto &cz, auto &tx, auto &tz, auto &s) {
        s ^= cx & tx & (cz ^ tz);
        cz ^= tx;
        tz ^= cx;
    });
}

void TableauTransposedRaii::append_SWAP(size_t q1, size_t q2) {
    for_each_trans_obs(*this, q1, q2, [](auto &x1, auto &z1, auto &x2, auto &z2, auto &s) {
        std::swap(x1, x2);
        std::swap(z1, z2);
    });
}

void TableauTransposedRaii::append_H_XY(size_t target) {
    for_each_trans_obs(*this, target, [](auto &x, auto &z, auto &s) {
        s ^= _mm256_andnot_si256(x, z);
        z ^= x;
    });
}

void TableauTransposedRaii::append_H_YZ(size_t target) {
    for_each_trans_obs(*this, target, [](auto &x, auto &z, auto &s) {
        s ^= _mm256_andnot_si256(z, x);
        x ^= z;
    });
}

void TableauTransposedRaii::append_H(size_t target) {
    for_each_trans_obs(*this, target, [](auto &x, auto &z, auto &s) {
        std::swap(x, z);
        s ^= x & z;
    });
}

void TableauTransposedRaii::append_X(size_t target) {
    for_each_trans_obs(*this, target, [](auto &x, auto &z, auto &s) {
        s ^= z;
    });
}

bool TableauTransposedRaii::z_sign(size_t a) const {
    return tableau.z_sign(a);
}

bool TableauTransposedRaii::z_obs_x_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.zs.xt[output_qubit][input_qubit];
}

bool TableauTransposedRaii::x_obs_z_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.xs.zt[output_qubit][input_qubit];
}

bool TableauTransposedRaii::z_obs_z_bit(size_t input_qubit, size_t output_qubit) const {
    return tableau.zs.zt[output_qubit][input_qubit];
}
