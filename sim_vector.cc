#include <iostream>
#include <map>
#include "pauli_string.h"
#include "sim_vector.h"
#include <bit>

SimVector::SimVector(size_t num_qubits) {
    state.resize(1 << num_qubits, 0.0f);
    state[0] = 1;
}

std::vector<std::complex<float>> mat_vec_mul(const std::vector<std::vector<std::complex<float>>> &matrix,
                                             const std::vector<std::complex<float>> &vec) {
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

void SimVector::apply(const std::vector<std::vector<std::complex<float>>> &matrix, const std::vector<size_t> &qubits) {
    size_t n = 1 << qubits.size();
    assert(matrix.size() == n);
    std::vector<size_t> masks;
    for (size_t k = 0; k < n; k++) {
        size_t m = 0;
        for (size_t q = 0; q < qubits.size(); q++) {
            if (k & (1 << q)) {
                m |= 1 << qubits[q];
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

void SimVector::apply(const std::string &gate, size_t qubit) {
    apply(GATE_UNITARIES.at(gate), {qubit});
}

void SimVector::apply(const std::string &gate, size_t qubit1, size_t qubit2) {
    apply(GATE_UNITARIES.at(gate), {qubit1, qubit2});
}

void SimVector::apply(const PauliStringPtr &gate, size_t qubit_offset) {
    if (gate.bit_ptr_sign.get()) {
        for (auto &e : state) {
            e *= -1;
        }
    }
    for (size_t k = 0; k < gate.size; k++) {
        bool x = gate.get_x_bit(k);
        bool z = gate.get_z_bit(k);
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

SimVector SimVector::from_stabilizers(const std::vector<PauliStringPtr> stabilizers) {
    assert(!stabilizers.empty());
    size_t num_qubits = stabilizers[0].size;
    SimVector result(num_qubits);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0, +1.0);
    for (size_t k = 0; k < result.state.size(); k++) {
        result.state[k] = dist(gen);
    }
    for (const auto &p : stabilizers) {
        result.project(p);
    }
    return result;
}

float SimVector::project(const PauliStringPtr &observable) {
    assert(1ULL << observable.size == state.size());
    auto basis_change = [&]() {
        for (size_t k = 0; k < observable.size; k++) {
            if (observable.get_x_bit(k)) {
                if (observable.get_z_bit(k)) {
                    apply("H_YZ", k);
                } else {
                    apply("H", k);
                }
            }
        }
    };

    uint64_t mask = 0;
    for (size_t k = 0; k < observable.size; k++) {
        if (observable.get_x_bit(k) | observable.get_z_bit(k)) {
            mask |= 1 << k;
        }
    }

    basis_change();
    float mag2 = 0;
    for (size_t i = 0; i < state.size(); i++) {
        bool reject = observable.bit_ptr_sign.get();
        reject ^= (std::popcount(i & mask) & 1) != 0;
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

bool SimVector::approximate_equals(const SimVector &other, bool up_to_global_phase) const {
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

constexpr std::complex<float> i = std::complex<float>(0, 1);
constexpr std::complex<float> s = 0.7071067811865475244f;
const std::unordered_map<std::string, const std::vector<std::vector<std::complex<float>>>> GATE_UNITARIES {
    {"I", {{1, 0}, {0, 1}}},
    // Pauli gates.
    {"X", {{0, 1}, {1, 0}}},
    {"Y", {{0, -i}, {i, 0}}},
    {"Z", {{1, 0}, {0, -1}}},
    // Axis exchange gates.
    {"H", {{s, s}, {s, -s}}},
    {"H_XY", {{0, s - i*s}, {s + i*s, 0}}},
    {"H_XZ", {{s, s}, {s, -s}}},
    {"H_YZ", {{s, -i*s}, {i*s, -s}}},
    // 90 degree rotation gates.
    {"SQRT_X", {{0.5f + 0.5f*i, 0.5f - 0.5f*i}, {0.5f - 0.5f*i, 0.5f + 0.5f*i}}},
    {"SQRT_X_DAG", {{0.5f - 0.5f*i, 0.5f + 0.5f*i}, {0.5f + 0.5f*i, 0.5f - 0.5f*i}}},
    {"SQRT_Y", {{0.5f + 0.5f*i, -0.5f - 0.5f*i}, {0.5f + 0.5f*i, 0.5f + 0.5f*i}}},
    {"SQRT_Y_DAG", {{0.5f - 0.5f*i, 0.5f - 0.5f*i}, {-0.5f + 0.5f*i, 0.5f - 0.5f*i}}},
    {"SQRT_Z", {{1, 0}, {0, i}}},
    {"SQRT_Z_DAG", {{1, 0}, {0, -i}}},
    {"S", {{1, 0}, {0, i}}},
    {"S_DAG", {{1, 0}, {0, -i}}},
    // Two qubit gates.
    {"CNOT", {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}}},
    {"CX", {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}}},
    {"CY", {{1, 0, 0, 0}, {0, 0, 0, -i}, {0, 0, 1, 0}, {0, i, 0, 0}}},
    {"CZ", {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, -1}}},
    {"SWAP", {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}}},
    {"ISWAP", {{1, 0, 0, 0}, {0, 0, i, 0}, {0, i, 0, 0}, {0, 0, 0, 1}}},
    {"ISWAP_DAG", {{1, 0, 0, 0}, {0, 0, -i, 0}, {0, -i, 0, 0}, {0, 0, 0, 1}}},
    // Controlled interactions in other bases.
    {"XCX", {{0.5f, 0.5f, 0.5f, -0.5f},
             {0.5f, 0.5f, -0.5f, 0.5f},
             {0.5f, -0.5f, 0.5f, 0.5f},
             {-0.5f, 0.5f, 0.5f, 0.5f}}},
    {"XCY", {{0.5f, 0.5f, -0.5f*i, 0.5f*i},
             {0.5f, 0.5f, 0.5f*i, -0.5f*i},
             {0.5f*i, -0.5f*i, 0.5f, 0.5f},
             {-0.5f*i, 0.5f*i, 0.5f, 0.5f}}},
    {"XCZ", {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}},
    {"YCX", {{0.5f, -i*0.5f, 0.5f, i*0.5f},
             {i*0.5f, 0.5f, -i*0.5f, 0.5f},
             {0.5f, i*0.5f, 0.5f, -i*0.5f},
             {-i*0.5f, 0.5f, i*0.5f, 0.5f}}},
    {"YCY", {{0.5f, -i*0.5f, -i*0.5f, 0.5f},
             {i*0.5f, 0.5f, -0.5f, -i*0.5f},
             {i*0.5f, -0.5f, 0.5f, -i*0.5f},
             {0.5f, i*0.5f, i*0.5f, 0.5f}}},
    {"YCZ", {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, -i}, {0, 0, i, 0}}},
};
