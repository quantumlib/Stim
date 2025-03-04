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

#include "stim/gates/gates.h"

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
    add_gate_data_pauli_product(failed);
    for (size_t k = 1; k < NUM_DEFINED_GATES; k++) {
        if (items[k].name.empty()) {
            std::cerr << "Uninitialized gate id: " << k << ".\n";
            failed = true;
        }
    }
    if (failed) {
        throw std::out_of_range("Failed to initialize gate data.");
    }
}

GateType Gate::hadamard_conjugated(bool ignoring_sign) const {
    switch (id) {
        case GateType::DETECTOR:
        case GateType::OBSERVABLE_INCLUDE:
        case GateType::TICK:
        case GateType::QUBIT_COORDS:
        case GateType::SHIFT_COORDS:
        case GateType::MPAD:
        case GateType::H:
        case GateType::H_NXZ:
        case GateType::DEPOLARIZE1:
        case GateType::DEPOLARIZE2:
        case GateType::Y_ERROR:
        case GateType::I:
        case GateType::II:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
        case GateType::Y:
        case GateType::SQRT_YY:
        case GateType::SQRT_YY_DAG:
        case GateType::MYY:
        case GateType::SWAP:
            return id;

        case GateType::MY:
        case GateType::MRY:
        case GateType::RY:
        case GateType::YCY:
            return ignoring_sign ? id : GateType::NOT_A_GATE;

        case GateType::ISWAP:
        case GateType::CZSWAP:
        case GateType::ISWAP_DAG:
            return GateType::NOT_A_GATE;

        case GateType::XCY:
            return ignoring_sign ? GateType::CY : GateType::NOT_A_GATE;
        case GateType::CY:
            return ignoring_sign ? GateType::XCY : GateType::NOT_A_GATE;
        case GateType::YCX:
            return ignoring_sign ? GateType::YCZ : GateType::NOT_A_GATE;
        case GateType::YCZ:
            return ignoring_sign ? GateType::YCX : GateType::NOT_A_GATE;

        case GateType::H_XY:
            return GateType::H_NYZ;
        case GateType::H_NYZ:
            return GateType::H_XY;

        case GateType::H_YZ:
            return GateType::H_NXY;
        case GateType::H_NXY:
            return GateType::H_YZ;

        case GateType::C_XYZ:
            return GateType::C_ZNYX;
        case GateType::C_ZNYX:
            return GateType::C_XYZ;

        case GateType::C_XNYZ:
            return GateType::C_ZYX;
        case GateType::C_ZYX:
            return GateType::C_XNYZ;

        case GateType::C_NXYZ:
            return GateType::C_ZYNX;
        case GateType::C_ZYNX:
            return GateType::C_NXYZ;

        case GateType::C_XYNZ:
            return GateType::C_NZYX;
        case GateType::C_NZYX:
            return GateType::C_XYNZ;

        case GateType::X:
            return GateType::Z;
        case GateType::Z:
            return GateType::X;
        case GateType::SQRT_Y:
            return GateType::SQRT_Y_DAG;
        case GateType::SQRT_Y_DAG:
            return GateType::SQRT_Y;
        case GateType::MX:
            return GateType::M;
        case GateType::M:
            return GateType::MX;
        case GateType::MRX:
            return GateType::MR;
        case GateType::MR:
            return GateType::MRX;
        case GateType::RX:
            return GateType::R;
        case GateType::R:
            return GateType::RX;
        case GateType::XCX:
            return GateType::CZ;
        case GateType::XCZ:
            return GateType::CX;
        case GateType::CX:
            return GateType::XCZ;
        case GateType::CZ:
            return GateType::XCX;
        case GateType::X_ERROR:
            return GateType::Z_ERROR;
        case GateType::Z_ERROR:
            return GateType::X_ERROR;
        case GateType::SQRT_X:
            return GateType::S;
        case GateType::SQRT_X_DAG:
            return GateType::S_DAG;
        case GateType::S:
            return GateType::SQRT_X;
        case GateType::S_DAG:
            return GateType::SQRT_X_DAG;
        case GateType::SQRT_XX:
            return GateType::SQRT_ZZ;
        case GateType::SQRT_XX_DAG:
            return GateType::SQRT_ZZ_DAG;
        case GateType::SQRT_ZZ:
            return GateType::SQRT_XX;
        case GateType::SQRT_ZZ_DAG:
            return GateType::SQRT_XX_DAG;
        case GateType::CXSWAP:
            return GateType::SWAPCX;
        case GateType::SWAPCX:
            return GateType::CXSWAP;
        case GateType::MXX:
            return GateType::MZZ;
        case GateType::MZZ:
            return GateType::MXX;
        default:
            return GateType::NOT_A_GATE;
    }
}

bool Gate::is_symmetric() const {
    if (flags & GATE_IS_SINGLE_QUBIT_GATE) {
        return true;
    }

    if (flags & GATE_TARGETS_PAIRS) {
        switch (id) {
            case GateType::II:
            case GateType::II_ERROR:
            case GateType::XCX:
            case GateType::YCY:
            case GateType::CZ:
            case GateType::DEPOLARIZE2:
            case GateType::SWAP:
            case GateType::ISWAP:
            case GateType::CZSWAP:
            case GateType::ISWAP_DAG:
            case GateType::MXX:
            case GateType::MYY:
            case GateType::MZZ:
            case GateType::SQRT_XX:
            case GateType::SQRT_YY:
            case GateType::SQRT_ZZ:
            case GateType::SQRT_XX_DAG:
            case GateType::SQRT_YY_DAG:
            case GateType::SQRT_ZZ_DAG:
                return true;
            default:
                return false;
        }
    }

    return false;
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
    if ((rx < 0) + (ry < 0) + (rz < 0) > 1) {
        rx = -rx;
        ry = -ry;
        rz = -rz;
        rs = -rs;
    }

    return {rx, ry, rz, acosf(rs) * 2};
}

bool Gate::has_known_unitary_matrix() const {
    return (flags & GateFlags::GATE_IS_UNITARY) &&
           (flags & (GateFlags::GATE_IS_SINGLE_QUBIT_GATE | GateFlags::GATE_TARGETS_PAIRS));
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
    if ((flags & GATE_IS_UNITARY) || id == GateType::TICK) {
        return GATE_DATA[best_candidate_inverse_id];
    }
    throw std::out_of_range(std::string(name) + " has no inverse.");
}

void GateDataMap::add_gate(bool &failed, const Gate &gate) {
    assert((size_t)gate.id < NUM_DEFINED_GATES);
    auto h = gate_name_to_hash(gate.name);
    auto &hash_loc = hashed_name_to_gate_type_table[h];
    if (!hash_loc.expected_name.empty()) {
        std::cerr << "GATE COLLISION " << gate.name << " vs " << items[(size_t)hash_loc.id].name << "\n";
        failed = true;
        return;
    }
    items[(size_t)gate.id] = gate;
    hash_loc.id = gate.id;
    hash_loc.expected_name = gate.name;
}

void GateDataMap::add_gate_alias(bool &failed, const char *alt_name, const char *canon_name) {
    auto h_alt = gate_name_to_hash(alt_name);
    auto &hash_loc = hashed_name_to_gate_type_table[h_alt];
    if (!hash_loc.expected_name.empty()) {
        std::cerr << "GATE COLLISION " << alt_name << " vs " << items[(size_t)hash_loc.id].name << "\n";
        failed = true;
        return;
    }

    auto h_canon = gate_name_to_hash(canon_name);
    if (hashed_name_to_gate_type_table[h_canon].expected_name.empty()) {
        std::cerr << "MISSING CANONICAL GATE " << canon_name << "\n";
        failed = true;
        return;
    }

    hash_loc.id = hashed_name_to_gate_type_table[h_canon].id;
    hash_loc.expected_name = alt_name;
}

extern const GateDataMap stim::GATE_DATA = GateDataMap();
