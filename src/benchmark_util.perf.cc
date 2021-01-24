#include "benchmark_util.h"
#include <cassert>
#include <complex>

Circuit unrotated_surface_code_circuit(size_t distance) {
    CircuitReader reader;
    size_t diam = distance * 2 - 1;
    size_t num_qubits = diam * diam;
    auto qubit = [&](std::complex<float> c){
        size_t q = (size_t)(c.real() * diam + c.imag());
        assert(q < num_qubits);
        return q;
    };
    auto in_range = [=](std::complex<float> c) {
        return c.real() >= 0 && c.real() < (float)diam
            && c.imag() >= 0 && c.imag() < (float)diam;
    };
    std::vector<std::complex<float>> c_data {};
    std::vector<std::complex<float>> c_xs {};
    std::vector<std::complex<float>> c_zs {};
    for (size_t x = 0; x < diam; x++) {
        for (size_t y = 0; y < diam; y++) {
            if (x % 2 == 1 && y % 2 == 0) {
                c_xs.emplace_back((float)x, (float)y);
            } else if (x % 2 == 0 && y % 2 == 1) {
                c_zs.emplace_back((float)x, (float)y);
            } else {
                c_data.emplace_back((float)x, (float)y);
            }
        }
    }
    std::vector<size_t> q_xs;
    std::vector<size_t> q_zxs;
    std::vector<size_t> q_datas;
    for (const auto &e : c_data) {
        q_datas.push_back(qubit(e));
    }
    for (const auto &z : c_zs) {
        q_zxs.push_back(qubit(z));
    }
    for (const auto &x : c_xs) {
        q_zxs.push_back(qubit(x));
        q_xs.push_back(qubit(x));
    }
    std::vector<std::complex<float>> dirs {
            {1, 0},
            {0, 1},
            {0, -1},
            {-1, 0},
    };
    for (size_t round = 0; round < distance; round++) {
        reader.read_operation({"H", q_xs});
        for (const auto &d : dirs) {
            for (const auto &z : c_zs) {
                auto p = z + d;
                if (in_range(p)) {
                    reader.read_operation({"CX", {qubit(p), qubit(z)}});
                }
            }
            for (const auto &x : c_xs) {
                auto p = x + d;
                if (in_range(p)) {
                    reader.read_operation({"CX", {qubit(x), qubit(p)}});
                }
            }
        }
        reader.read_operation({"H", q_xs});
        reader.read_operation({"M", q_zxs});
    }
    reader.read_operation({"M", q_datas});
    return Circuit(reader.ops);
}
