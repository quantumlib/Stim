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

#include "stim/circuit/gate_data.h"

#include <complex>

using namespace stim;

GateDataMap::GateDataMap() {
    bool failed = false;
    items[0].name = "NOT_A_GATE";

    add_gate_data_annotations(failed);
    add_gate_data_blocks(failed);
    add_gate_data_collapsing(failed);
    add_gate_data_controlled(failed);
    add_gate_data_hada(failed);
    add_gate_data_heralded(failed);
    add_gate_data_noisy(failed);
    add_gate_data_pauli(failed);
    add_gate_data_period_3(failed);
    add_gate_data_period_4(failed);
    add_gate_data_pp(failed);
    add_gate_data_swaps(failed);
    add_gate_data_pair_measure(failed);
    for (size_t k = 1; k < NUM_DEFINED_GATES; k++) {
        if (items[k].name == nullptr) {
            std::cerr << "Uninitialized gate id: " << k << ".\n";
            failed = true;
        }
    }
    if (failed) {
        throw std::out_of_range("Failed to initialize gate data.");
    }
}

std::array<float, 3> Gate::to_euler_angles() const {
    if (unitary_data.size() != 2) {
        throw std::out_of_range(std::string(name) + " doesn't have 1q unitary data.");
    }
    auto a = unitary_data[0][0];
    auto b = unitary_data[0][1];
    auto c = unitary_data[1][0];
    auto d = unitary_data[1][1];
    std::array<float, 3> xyz;
    if (a == std::complex<float>{0}) {
        xyz[0] = 3.14159265359f;
        xyz[1] = 0;
        xyz[2] = arg(-b / c);
    } else if (b == std::complex<float>{0}) {
        xyz[0] = 0;
        xyz[1] = 0;
        xyz[2] = arg(d / a);
    } else {
        xyz[0] = 3.14159265359f / 2;
        xyz[1] = arg(c / a);
        xyz[2] = arg(-b / a);
    }
    return xyz;
}

std::array<float, 4> Gate::to_axis_angle() const {
    if (unitary_data.size() != 2) {
        throw std::out_of_range(std::string(name) + " doesn't have 1q unitary data.");
    }
    auto a = unitary_data[0][0];
    auto b = unitary_data[0][1];
    auto c = unitary_data[1][0];
    auto d = unitary_data[1][1];
    auto i = std::complex<float>{0, 1};
    auto x = b + c;
    auto y = b * i + c * -i;
    auto z = a - d;
    auto s = a + d;
    s *= -i;
    std::complex<double> p = 1;
    if (s.imag() != 0) {
        p = s;
    }
    if (x.imag() != 0) {
        p = x;
    }
    if (y.imag() != 0) {
        p = y;
    }
    if (z.imag() != 0) {
        p = z;
    }
    p /= sqrt(p.imag() * p.imag() + p.real() * p.real());
    p *= 2;
    x /= p;
    y /= p;
    z /= p;
    s /= p;
    assert(x.imag() == 0);
    assert(y.imag() == 0);
    assert(z.imag() == 0);
    assert(s.imag() == 0);
    auto rx = x.real();
    auto ry = y.real();
    auto rz = z.real();
    auto rs = s.real();

    // At this point it's more of a quaternion. Normalize the axis.
    auto r = sqrt(rx * rx + ry * ry + rz * rz);
    if (r == 0) {
        rx = 1;
    } else {
        rx /= r;
        ry /= r;
        rz /= r;
    }

    return {rx, ry, rz, acosf(rs) * 2};
}

std::vector<std::vector<std::complex<float>>> Gate::unitary() const {
    if (unitary_data.size() != 2 && unitary_data.size() != 4) {
        throw std::out_of_range(std::string(name) + " doesn't have 1q or 2q unitary data.");
    }
    std::vector<std::vector<std::complex<float>>> result;
    for (size_t k = 0; k < unitary_data.size(); k++) {
        const auto &d = unitary_data[k];
        result.emplace_back();
        for (size_t j = 0; j < d.size(); j++) {
            result.back().push_back(d[j]);
        }
    }
    return result;
}

const Gate &Gate::inverse() const {
    std::string inv_name = name;
    if ((flags & GATE_IS_UNITARY) || id == GateType::TICK) {
        return GATE_DATA[best_candidate_inverse_id];
    }
    throw std::out_of_range(inv_name + " has no inverse.");
}

void GateDataMap::add_gate(bool &failed, const Gate &gate) {
    assert((size_t)gate.id < NUM_DEFINED_GATES);
    const char *c = gate.name;
    auto h = gate_name_to_hash(c);
    auto &hash_loc = hashed_name_to_gate_type_table[h];
    if (hash_loc.expected_name_len != 0) {
        std::cerr << "GATE COLLISION " << gate.name << " vs " << items[(size_t)hash_loc.id].name << "\n";
        failed = true;
        return;
    }
    items[(size_t)gate.id] = gate;
    hash_loc.id = gate.id;
    hash_loc.expected_name = gate.name;
    hash_loc.expected_name_len = strlen(gate.name);
}

void GateDataMap::add_gate_alias(bool &failed, const char *alt_name, const char *canon_name) {
    auto h_alt = gate_name_to_hash(alt_name);
    auto &hash_loc = hashed_name_to_gate_type_table[h_alt];
    if (hash_loc.expected_name_len != 0) {
        std::cerr << "GATE COLLISION " << alt_name << " vs " << items[(size_t)hash_loc.id].name << "\n";
        failed = true;
        return;
    }

    auto h_canon = gate_name_to_hash(canon_name);
    if (hashed_name_to_gate_type_table[h_canon].expected_name_len == 0) {
        std::cerr << "MISSING CANONICAL GATE " << canon_name << "\n";
        failed = true;
        return;
    }

    hash_loc.id = hashed_name_to_gate_type_table[h_canon].id;
    hash_loc.expected_name = alt_name;
    hash_loc.expected_name_len = strlen(alt_name);
}

extern const GateDataMap stim::GATE_DATA = GateDataMap();
