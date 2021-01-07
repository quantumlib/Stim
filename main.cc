#include <cassert>
#include <chrono>
#include <complex>
#include <immintrin.h>
#include <iostream>
#include <new>
#include <sstream>
#include "chp_sim.h"
#include "pauli_string.h"
#include "perf.h"
#include "simd_util.h"

void run_surface_code_sim(size_t distance, bool progress = false) {
    size_t diam = distance * 2 - 1;
    auto qubit = [&](std::complex<float> c){
        return (size_t)(c.real() * diam + c.imag());
    };
    auto in_range = [=](std::complex<float> c) {
        return c.real() >= 0 && c.real() < (float)diam
            && c.imag() >= 0 && c.imag() < (float)diam;
    };
    std::vector<std::complex<float>> data;
    std::vector<std::complex<float>> xs;
    std::vector<std::complex<float>> zs;
    auto sim = ChpSim(diam*diam);
    for (size_t x = 0; x < diam; x++) {
        for (size_t y = 0; y < diam; y++) {
            if (x % 2 == 1 && y % 2 == 0) {
                xs.emplace_back((float)x, (float)y);
            } else if (x % 2 == 0 && y % 2 == 1) {
                zs.emplace_back((float)x, (float)y);
            } else {
                data.emplace_back((float)x, (float)y);
            }
        }
    }
    std::vector<size_t> zxqs;
    std::vector<size_t> data_qs;
    for (const auto &e : data) {
        data_qs.push_back(qubit(e));
    }
    for (const auto &z : zs) {
        zxqs.push_back(qubit(z));
    }
    for (const auto &x : xs) {
        zxqs.push_back(qubit(x));
    }
    std::vector<std::complex<float>> dirs {
            {1, 0},
            {0, 1},
            {0, -1},
            {-1, 0},
    };
    for (size_t round = 0; round < distance; round++) {
        if (progress) {
            std::cerr << "round " << round << "\n";
        }
        for (const auto &x : xs) {
            sim.H(qubit(x));
        }
        for (const auto &d : dirs) {
            for (const auto &z : zs) {
                auto p = z + d;
                if (in_range(p)) {
                    sim.CX(qubit(p), qubit(z));
                }
            }
            for (const auto &x : xs) {
                auto p = x + d;
                if (in_range(p)) {
                    sim.CX(qubit(x), qubit(p));
                }
            }
        }
        for (const auto &x : xs) {
            sim.H(qubit(x));
        }
        for (const auto &z : zs) {
            assert(sim.is_deterministic(qubit(z)));
        }
        for (const auto &x : xs) {
            assert(sim.is_deterministic(qubit(x)) == round > 0);
        }
        sim.measure_many(zxqs);
    }
    sim.measure_many(data_qs);
}

void time_clifford_sim(size_t distance, bool progress = false) {
    std::cerr << "run_surface_code_sim(distance=" << distance << ")\n";
    auto f = PerfResult::time([&]() { run_surface_code_sim(distance, progress); });
    std::cerr << f;
    std::cerr << "\n";
}

void time_transpose(size_t block_diameter) {
    size_t w = 256 * block_diameter;
    size_t num_bits = w * w;
    auto data = aligned_bits256::random(num_bits);
    std::cerr << "transpose_bit_matrix(block_diameter=" << block_diameter << "; ";
    std::cerr << (block_diameter * block_diameter * 256 * 256 / 8 / 1024 / 1024) << "MiB)\n";
    std::cerr << PerfResult::time([&](){
        transpose_bit_matrix(data.u64, w);
    });
    std::cerr << "\n";
}

void time_mike_transpose(size_t block_diameter) {
    std::cerr << "mike_transpose_bit_matrix(block_diameter=" << block_diameter << "; ";
    std::cerr << (block_diameter * block_diameter * 256 * 256 / 8 / 1024 / 1024) << "MiB)\n";

    size_t w = 256 * block_diameter;
    size_t num_bits = w * w;
    auto data = aligned_bits256::random(num_bits);
    auto out = aligned_bits256(num_bits);
    std::cerr << PerfResult::time([&](){
        mike_transpose_bit_matrix(data.u64, out.u64, w);
    });
    std::cerr << "\n";
}

void time_transpose_blockwise(size_t block_diameter) {
    std::cerr << "transpose_bit_matrix_256x256blocks(block_diameter=" << block_diameter << "; ";
    std::cerr << (block_diameter * block_diameter * 256 * 256 / 8 / 1024 / 1024) << "MiB)\n";

    size_t w = 256 * block_diameter;
    size_t num_bits = w * w;
    auto data = aligned_bits256::random(num_bits);
    auto f = PerfResult::time([&](){
        transpose_bit_matrix_256x256blocks(data.u64, w);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * block_diameter * block_diameter / 1000.0 / 1000.0 << " MegaBlocks/s)";
    std::cerr << "\n";
}

void time_pauli_multiplication(size_t num_qubits) {
    std::cerr << "pauli multiplication(n=" << num_qubits << ")\n";

    auto p1 = PauliStringVal::from_pattern(
            false,
            num_qubits,
            [](size_t i) { return "_XYZX"[i % 5]; });
    auto p2 = PauliStringVal::from_pattern(
            true,
            num_qubits,
            [](size_t i) { return "_XZYZZX"[i % 7]; });
    PauliStringPtr p1_ptr = p1;
    PauliStringPtr p2_ptr = p2;
    auto f = PerfResult::time([&](){
        p1_ptr.inplace_right_mul_with_scalar_output(p2_ptr);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * num_qubits / 1000 / 1000 / 1000 << " GigaPauliMuls/s)";
    std::cerr << "\n";
}

void time_pauli_swap(size_t num_qubits) {
    std::cerr << "pauli swaps(n=" << num_qubits << ")\n";

    auto p1 = PauliStringVal::from_pattern(
            false,
            num_qubits,
            [](size_t i) { return "_XYZX"[i % 5]; });
    auto p2 = PauliStringVal::from_pattern(
            true,
            num_qubits,
            [](size_t i) { return "_XZYZZX"[i % 7]; });
    PauliStringPtr p1_ptr = p1;
    PauliStringPtr p2_ptr = p2;
    auto f = PerfResult::time([&](){
        p1_ptr.swap_with(p2_ptr);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * num_qubits / 1000 / 1000 / 1000 << " GigaPauliSwaps/s)";
    std::cerr << "\n";
}

int main() {
//    size_t block_diam = 100;
//    time_transpose_blockwise(block_diam);
//    time_mike_transpose(block_diam);
//    time_transpose(block_diam);
//
    time_pauli_multiplication(100000);
//    time_clifford_sim(51);
//    time_pauli_swap(100000);
}
