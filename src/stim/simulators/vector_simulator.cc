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
#include "stim/probability_util.h"
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

std::vector<std::complex<float>> VectorSimulator::state_vector_from_stabilizers(
    const std::vector<PauliStringRef> &stabilizers, float norm2) {
    size_t num_qubits = stabilizers.empty() ? 0 : stabilizers[0].num_qubits;
    VectorSimulator sim(num_qubits);

    // Create an initial state $|A\rangle^{\otimes n}$ which overlaps with all possible stabilizers.
    std::uniform_real_distribution<float> dist(-1.0, +1.0);

    auto rng = externally_seeded_rng();
    for (auto &s : sim.state) {
        s = {dist(rng), dist(rng)};
    }

    // Project out the non-overlapping parts.
    for (const auto &p : stabilizers) {
        sim.project(p);
    }
    if (stabilizers.empty()) {
        sim.project(PauliString(0));
    }

    sim.canonicalize_assuming_stabilizer_state(norm2);

    return sim.state;
}

VectorSimulator VectorSimulator::from_stabilizers(const std::vector<PauliStringRef> &stabilizers) {
    VectorSimulator result(0);
    result.state = state_vector_from_stabilizers(stabilizers, 1);
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
        if (observable.xs[k] || observable.zs[k]) {
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

void VectorSimulator::smooth_stabilizer_state(std::complex<float> base_value) {
    std::vector<std::complex<float>> ratio_values{
        {0, 0},
        {1, 0},
        {-1, 0},
        {0, 1},
        {0, -1},
    };
    for (size_t k = 0; k < state.size(); k++) {
        auto ratio = state[k] / base_value;
        bool solved = false;
        for (const auto &r : ratio_values) {
            if (std::norm(ratio - r) < 0.125) {
                state[k] = r;
                solved = true;
            }
        }
        if (!solved) {
            throw std::invalid_argument("The state vector wasn't a stabilizer state.");
        }
    }
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

void VectorSimulator::canonicalize_assuming_stabilizer_state(double norm2) {
    // Find a solid non-zero entry.
    size_t nz = 0;
    for (size_t k = 1; k < state.size(); k++) {
        if (abs(state[k]) > abs(state[nz]) * 2) {
            nz = k;
        }
    }

    // Rescale so that the non-zero entries are 1, -1, 1j, or -1j.
    size_t num_non_zero = 0;
    std::complex<float> big_v = state[nz];
    for (auto &v : state) {
        v /= big_v;
        if (abs(v) < 0.1) {
            v = 0;
            continue;
        }
        num_non_zero++;
        if (abs(v - std::complex<float>{1, 0}) < 0.1) {
            v = std::complex<float>{1, 0};
        } else if (abs(v - std::complex<float>{0, 1}) < 0.1) {
            v = std::complex<float>{0, 1};
        } else if (abs(v - std::complex<float>{-1, 0}) < 0.1) {
            v = std::complex<float>{-1, 0};
        } else if (abs(v - std::complex<float>{0, -1}) < 0.1) {
            v = std::complex<float>{0, -1};
        } else {
            throw std::invalid_argument("State vector extraction failed. This shouldn't occur.");
        }
    }

    // Normalize the entries so the result is a unit vector.
    std::complex<float> scale = (float)(sqrt(norm2 / (double)num_non_zero));
    for (auto &v : state) {
        v *= scale;
    }
}
void VectorSimulator::do_unitary_circuit(const Circuit &circuit) {
    std::vector<size_t> targets1{1};
    std::vector<size_t> targets2{1, 2};
    circuit.for_each_operation([&](const Operation &op) {
        if (!(op.gate->flags & GATE_IS_UNITARY)) {
            std::stringstream ss;
            ss << "Not a unitary gate: " << op.gate->name;
            throw std::invalid_argument(ss.str());
        }
        auto unitary = op.gate->unitary();
        for (auto t : op.target_data.targets) {
            if (!t.is_qubit_target() || (size_t{1} << t.data) >= state.size()) {
                std::stringstream ss;
                ss << "Targets out of range: " << op;
                throw std::invalid_argument(ss.str());
            }
        }
        if (op.gate->flags & stim::GATE_TARGETS_PAIRS) {
            for (size_t k = 0; k < op.target_data.targets.size(); k += 2) {
                targets2[0] = (size_t)op.target_data.targets[k].data;
                targets2[1] = (size_t)op.target_data.targets[k + 1].data;
                apply(unitary, targets2);
            }
        } else {
            for (auto t : op.target_data.targets) {
                targets1[0] = (size_t)t.data;
                apply(unitary, targets1);
            }
        }
    });
}
