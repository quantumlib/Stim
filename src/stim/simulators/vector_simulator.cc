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

#include "stim/simulators/vector_simulator.h"

#include <cassert>

#include "stim/circuit/gate_data.h"
#include "stim/mem/simd_util.h"
#include "stim/stabilizers/pauli_string.h"

using namespace stim;

VectorSimulator::VectorSimulator(size_t num_qubits) {
    state.resize(size_t{1} << num_qubits, 0.0f);
    state[0] = 1;
}

std::vector<std::complex<float>> mat_vec_mul(
    const std::vector<std::vector<std::complex<float>>> &matrix, const std::vector<std::complex<float>> &vec) {
    std::vector<std::complex<float>> result;
    for (size_t row = 0; row < vec.size(); row++) {
        std::complex<float> v = 0;
        for (size_t col = 0; col < vec.size(); col++) {
            v += matrix[row][col] * vec[col];
        }
        result.push_back(v);
    }
    return result;
}

void VectorSimulator::apply(
    const std::vector<std::vector<std::complex<float>>> &matrix, const std::vector<size_t> &qubits) {
    size_t n = size_t{1} << qubits.size();
    assert(matrix.size() == n);
    std::vector<size_t> masks;
    for (size_t k = 0; k < n; k++) {
        size_t m = 0;
        for (size_t q = 0; q < qubits.size(); q++) {
            if ((k >> q) & 1) {
                m |= size_t{1} << qubits[q];
            }
        }
        masks.push_back(m);
    }
    assert(masks.back() < state.size());
    for (size_t base = 0; base < state.size(); base++) {
        if (base & masks.back()) {
            continue;
        }
        std::vector<std::complex<float>> in;
        in.reserve(masks.size());
        for (auto m : masks) {
            in.push_back(state[base | m]);
        }
        auto out = mat_vec_mul(matrix, in);
        for (size_t k = 0; k < masks.size(); k++) {
            state[base | masks[k]] = out[k];
        }
    }
}

void VectorSimulator::apply(const std::string &gate, size_t qubit) {
    try {
        apply(GATE_DATA.at(gate).unitary(), {qubit});
    } catch (const std::out_of_range &) {
        throw std::out_of_range("Single qubit gate isn't supported by VectorSimulator: " + gate);
    }
}

void VectorSimulator::apply(const std::string &gate, size_t qubit1, size_t qubit2) {
    try {
        apply(GATE_DATA.at(gate).unitary(), {qubit1, qubit2});
    } catch (const std::out_of_range &) {
        throw std::out_of_range("Two qubit gate isn't supported by VectorSimulator: " + gate);
    }
}

void VectorSimulator::apply(const PauliStringRef &gate, size_t qubit_offset) {
    if (gate.sign) {
        for (auto &e : state) {
            e *= -1;
        }
    }
    for (size_t k = 0; k < gate.num_qubits; k++) {
        bool x = gate.xs[k];
        bool z = gate.zs[k];
        size_t q = qubit_offset + k;
        if (x && z) {
            apply("Y", q);
        } else if (x) {
            apply("X", q);
        } else if (z) {
            apply("Z", q);
        }
    }
}

VectorSimulator VectorSimulator::from_stabilizers(const std::vector<PauliStringRef> stabilizers, std::mt19937_64 &rng) {
    size_t num_qubits = stabilizers.empty() ? 0 : stabilizers[0].num_qubits;
    VectorSimulator result(num_qubits);

    // Create an initial state $|A\rangle^{\otimes n}$ which overlaps with all possible stabilizers.
    std::uniform_real_distribution<float> dist(-1.0, +1.0);
    for (size_t k = 0; k < result.state.size(); k++) {
        result.state[k] = {dist(rng), dist(rng)};
    }

    // Project out the non-overlapping parts.
    for (const auto &p : stabilizers) {
        result.project(p);
    }
    if (stabilizers.empty()) {
        result.project(PauliString(0));
    }

    return result;
}

float VectorSimulator::project(const PauliStringRef &observable) {
    assert(1ULL << observable.num_qubits == state.size());
    auto basis_change = [&]() {
        for (size_t k = 0; k < observable.num_qubits; k++) {
            if (observable.xs[k]) {
                if (observable.zs[k]) {
                    apply("H_YZ", k);
                } else {
                    apply("H_XZ", k);
                }
            }
        }
    };

    uint64_t mask = 0;
    for (size_t k = 0; k < observable.num_qubits; k++) {
        if (observable.xs[k] | observable.zs[k]) {
            mask |= 1ULL << k;
        }
    }

    basis_change();
    float mag2 = 0;
    for (size_t i = 0; i < state.size(); i++) {
        bool reject = observable.sign;
        reject ^= (popcnt64(i & mask) & 1) != 0;
        if (reject) {
            state[i] = 0;
        } else {
            mag2 += state[i].real() * state[i].real() + state[i].imag() * state[i].imag();
        }
    }
    assert(mag2 > 1e-8);
    auto w = sqrtf(mag2);
    for (size_t i = 0; i < state.size(); i++) {
        state[i] /= w;
    }
    basis_change();
    return mag2;
}

bool VectorSimulator::approximate_equals(const VectorSimulator &other, bool up_to_global_phase) const {
    if (state.size() != other.state.size()) {
        return false;
    }
    std::complex<float> dot = 0;
    float mag1 = 0;
    float mag2 = 0;
    for (size_t k = 0; k < state.size(); k++) {
        auto c = state[k];
        auto c2 = other.state[k];
        dot += c * std::conj(c2);
        mag1 += c.real() * c.real() + c.imag() * c.imag();
        mag2 += c2.real() * c2.real() + c2.imag() * c2.imag();
    }
    assert(1 - 1e-4 <= mag1 && mag1 <= 1 + 1e-4);
    assert(1 - 1e-4 <= mag2 && mag2 <= 1 + 1e-4);
    float f;
    if (up_to_global_phase) {
        f = dot.real() * dot.real() + dot.imag() * dot.imag();
    } else {
        f = dot.real();
    }
    return 1 - 1e-4 <= f && f <= 1 + 1e-4;
}

std::string VectorSimulator::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream &stim::operator<<(std::ostream &out, const VectorSimulator &sim) {
    out << "VectorSimulator {\n";
    for (size_t k = 0; k < sim.state.size(); k++) {
        out << "    " << k << ": " << sim.state[k] << "\n";
    }
    out << "}";
    return out;
}
