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
        {x.x_ref.u256, x.z_ref.u256, tableau.xs.signs.u256},
        {z.x_ref.u256, z.z_ref.u256, tableau.zs.signs.u256}
    }};
}

TableauTransposedRaii::TableauTransposedRaii(Tableau &tableau) : tableau(tableau) {
    tableau.do_transpose_quadrants();
}

TableauTransposedRaii::~TableauTransposedRaii() {
    tableau.do_transpose_quadrants();
}

void TableauTransposedRaii::append_CX(size_t control, size_t target) {
    auto pcs = transposed_xz_ptr(control);
    auto pts = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.xz[k];
        auto pt = pts.xz[k];
        auto s = pc.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= _mm256_andnot_si256(*pc.z ^ *pt.x, *pc.x & *pt.z);
            *pc.z ^= *pt.z;
            *pt.x ^= *pc.x;
            pc.x++;
            pc.z++;
            pt.x++;
            pt.z++;
            s++;
        }
    }
}

void TableauTransposedRaii::append_CY(size_t control, size_t target) {
    auto pcs = transposed_xz_ptr(control);
    auto pts = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.xz[k];
        auto pt = pts.xz[k];
        auto s = pc.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= *pc.x & (*pc.z ^ *pt.x) & (*pt.z ^ *pt.x);
            *pc.z ^= *pt.x;
            *pc.z ^= *pt.z;
            *pt.x ^= *pc.x;
            *pt.z ^= *pc.x;
            pc.x++;
            pc.z++;
            pt.x++;
            pt.z++;
            s++;
        }
    }
}

void TableauTransposedRaii::append_CZ(size_t control, size_t target) {
    auto pcs = transposed_xz_ptr(control);
    auto pts = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto pc = pcs.xz[k];
        auto pt = pts.xz[k];
        auto s = pc.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= *pc.x & *pt.x & (*pc.z ^ *pt.z);
            *pc.z ^= *pt.x;
            *pt.z ^= *pc.x;
            pc.x++;
            pc.z++;
            pt.x++;
            pt.z++;
            s++;
        }
    }
}

void TableauTransposedRaii::append_SWAP(size_t q1, size_t q2) {
    auto p1s = transposed_xz_ptr(q1);
    auto p2s = transposed_xz_ptr(q2);
    for (size_t k = 0; k < 2; k++) {
        auto p1 = p1s.xz[k];
        auto p2 = p2s.xz[k];
        auto end = p1.x + (ceil256(tableau.num_qubits) >> 8);
        while (p1.x != end) {
            std::swap(*p1.x, *p2.x);
            std::swap(*p1.z, *p2.z);
            p1.x++;
            p1.z++;
            p2.x++;
            p2.z++;
        }
    }
}

void TableauTransposedRaii::append_H_XY(size_t target) {
    auto ps = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.xz[k];
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= _mm256_andnot_si256(*p.x, *p.z);
            *p.z ^= *p.x;
            p.x++;
            p.z++;
            s++;
        }
    }
}

void TableauTransposedRaii::append_H_YZ(size_t target) {
    auto ps = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.xz[k];
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= _mm256_andnot_si256(*p.z, *p.x);
            *p.x ^= *p.z;
            p.x++;
            p.z++;
            s++;
        }
    }
}

void TableauTransposedRaii::append_H(size_t target) {
    auto ps = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.xz[k];
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            std::swap(*p.x, *p.z);
            *s ^= *p.x & *p.z;
            p.x++;
            p.z++;
            s++;
        }
    }
}

void TableauTransposedRaii::append_X(size_t target) {
    auto ps = transposed_xz_ptr(target);
    for (size_t k = 0; k < 2; k++) {
        auto p = ps.xz[k];
        auto s = p.s;
        auto end = s + (ceil256(tableau.num_qubits) >> 8);
        while (s != end) {
            *s ^= *p.z;
            p.z++;
            s++;
        }
    }
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
