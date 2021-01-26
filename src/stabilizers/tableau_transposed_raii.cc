#include "tableau_transposed_raii.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <thread>

#include "pauli_string.h"

TableauTransposedRaii::TableauTransposedRaii(Tableau &tableau) : tableau(tableau) { tableau.do_transpose_quadrants(); }

TableauTransposedRaii::~TableauTransposedRaii() { tableau.do_transpose_quadrants(); }

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
    for_each_trans_obs(*this, control, target, [](auto &cx, auto &cz, auto &tx, auto &tz, auto &s) {
        s ^= (cz ^ tx).andnot(cx & tz);
        cz ^= tz;
        tx ^= cx;
    });
}

void TableauTransposedRaii::append_ZCY(size_t control, size_t target) {
    for_each_trans_obs(*this, control, target, [](auto &cx, auto &cz, auto &tx, auto &tz, auto &s) {
        cz ^= tx;
        s ^= cx & cz & (tx ^ tz);
        cz ^= tz;
        tx ^= cx;
        tz ^= cx;
    });
}

void TableauTransposedRaii::append_ZCZ(size_t control, size_t target) {
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
        s ^= x.andnot(z);
        z ^= x;
    });
}

void TableauTransposedRaii::append_H_YZ(size_t target) {
    for_each_trans_obs(*this, target, [](auto &x, auto &z, auto &s) {
        s ^= z.andnot(x);
        x ^= z;
    });
}

void TableauTransposedRaii::append_H_XZ(size_t q) {
    for_each_trans_obs(*this, q, [](auto &x, auto &z, auto &s) {
        std::swap(x, z);
        s ^= x & z;
    });
}

void TableauTransposedRaii::append_X(size_t target) {
    for_each_trans_obs(*this, target, [](auto &x, auto &z, auto &s) { s ^= z; });
}
