#include <cassert>
#include <chrono>
#include <complex>
#include <immintrin.h>
#include <iostream>
#include <new>
#include <sstream>
#include "sim_tableau.h"
#include "pauli_string.h"
#include "perf.h"
#include "simd_util.h"
#include <cstring>
#include "circuit.h"
#include "sim_frame.h"

Circuit surface_code_circuit(size_t distance) {
    std::stringstream ss;
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
    std::vector<std::complex<float>> data;
    std::vector<std::complex<float>> xs;
    std::vector<std::complex<float>> zs;
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
        for (const auto &x : xs) {
            ss << "H " << qubit(x) << "\n";
        }
        for (const auto &d : dirs) {
            for (const auto &z : zs) {
                auto p = z + d;
                if (in_range(p)) {
                    ss << "CX " << qubit(p) << " " << qubit(z) << "\n";
                }
            }
            for (const auto &x : xs) {
                auto p = x + d;
                if (in_range(p)) {
                    ss << "CX " << qubit(x) << " " << qubit(p) << "\n";
                }
            }
        }
        for (const auto &x : xs) {
            ss << "H " << qubit(x) << "\n";
        }
        for (const auto &q : zxqs) {
            ss << "M " << q << "\n";
        }
    }
    for (const auto &q : data_qs) {
        ss << "M " << q << "\n";
    }
    return Circuit::from_text(ss.str());
}

void time_tableau_sim(size_t distance) {
    std::cerr << "tableau_sim(unrotated surface code distance=" << distance << ")\n";
    auto circuit = surface_code_circuit(distance);
    auto f = PerfResult::time([&]() { SimTableau::simulate(circuit); });
    std::cerr << f;
    std::cerr << "\n";
}

void time_pauli_frame_sim(size_t distance) {
    std::cerr << "frame_sim(unrotated surface code distance=" << distance << ")\n";
    auto circuit = surface_code_circuit(distance);
    auto sim = SimFrame::recorded_from_tableau_sim(circuit.operations);
    std::mt19937 rng((std::random_device {})());
    auto out = aligned_bits256(sim.num_measurements);
    auto f = PerfResult::time([&]() {
        sim.sample(out, rng);
    });
    std::cerr << f;
    std::cerr << "\n";
}

void time_pauli_frame_sim2(size_t distance, size_t num_samples) {
    assert(!(num_samples & 0xFF));
    std::cerr << "frame_sim(unrotated surface code distance=" << distance << ", samples=" << num_samples << ")\n";
    auto circuit = surface_code_circuit(distance);
    auto sim = SimFrame::recorded_from_tableau_sim(circuit.operations);
    auto sim2 = SimFrame2(sim.num_qubits, num_samples >> 8, sim.num_measurements);
    auto f = PerfResult::time([&]() {
        for (const auto &cycle : sim.cycles) {
            for (const auto &op : cycle.step1_unitary) {
                if (op.name == "H") {
                    sim2.H_XZ(op.targets);
                } else if (op.name == "CX") {
                    sim2.CX(op.targets);
                } else {
                    assert(false);
                }
            }
            for (const auto &collapse : cycle.step2_collapse) {
                sim2.RANDOM_INTO_FRAME(collapse.destabilizer);
            }
            sim2.RECORD(cycle.step3_measure);
            sim2.R(cycle.step4_reset);
        }
    });

    std::cerr << f << " (sample rate " << si_describe(f.rate() * num_samples) << "Hz)";
    std::cerr << "\n";
}

void time_transpose_blockwise(size_t blocks) {
    std::cerr << "transpose_bit_matrix_256x256blocks(blocks=" << blocks << "; ";
    std::cerr << (blocks * 256 * 256 / 8 / 1024 / 1024) << "MiB)\n";

    size_t num_bits = blocks << 16;
    auto data = aligned_bits256::random(num_bits);
    auto f = PerfResult::time([&](){
        transpose_bit_matrix_256x256blocks(data.u64, num_bits);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * blocks / 1000.0 / 1000.0 << " MegaBlocks/s)";
    std::cerr << "\n";
}

void time_transpose_tableau(size_t num_qubits) {
    std::cerr << "transpose_tableau(num_qubits=" << num_qubits << "; ";

    Tableau tableau(num_qubits);
    std::cerr << (tableau.data_x2x.num_bits * 4 / 8 / 1024 / 1024) << "MiB)\n";
    auto f = PerfResult::time([&](){
        TempTransposedTableauRaii temp(tableau);
    });
    std::cerr << f;
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
        p1_ptr.inplace_right_mul_returning_log_i_scalar(p2_ptr);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * num_qubits / 1000 / 1000 / 1000 << " GigaPaulis/s)";
    std::cerr << "\n";
}

void time_memcpy(size_t bits) {
    std::cerr << "memcpy (bits=" << bits << ")\n";

    auto d1 = aligned_bits256::random(bits * 2);
    auto d2 = aligned_bits256::random(bits * 2);
    auto f = PerfResult::time([&](){
        memcpy(d1.u64, d2.u64, d1.num_bits >> 3);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * bits / 1000 / 1000 / 1000 << " GigaPaulis/s)";
    std::cerr << "\n";
}

void time_tableau_pauli_multiplication(size_t num_qubits) {
    std::cerr << "tableau pauli multiplication(n=" << num_qubits << ")\n";

    Tableau t(num_qubits);
    PauliStringPtr p1_ptr = t.x_obs_ptr(0);
    PauliStringPtr p2_ptr = t.x_obs_ptr(5);
    auto f = PerfResult::time([&](){
        p1_ptr.inplace_right_mul_returning_log_i_scalar(p2_ptr);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * num_qubits / 1000 / 1000 / 1000 << " GigaPaulis/s)";
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

void time_cnot(size_t num_qubits) {
    std::cerr << "cnot(n=" << num_qubits << ")\n";

    SimTableau sim(num_qubits);
    auto f = PerfResult::time([&](){
        sim.CX(0, num_qubits - 1);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() / 1000 / 1000 << " MegaCNOT/s)";
    std::cerr << "\n";
}

int main() {
//    time_transpose_blockwise(100);
//    time_transpose_tableau(20000);
//    time_pauli_multiplication(100000);
//    time_pauli_multiplication(1000000);
//    time_pauli_multiplication(10000000);
//    time_memcpy(100000);
//    time_memcpy(1000000);
//    time_memcpy(10000000);
//    time_tableau_pauli_multiplication(10000);
//    time_pauli_swap(100000);
//    time_tableau_sim(51);
    time_pauli_frame_sim2(51, 256 * 4);
//    time_pauli_frame_sim(51);
//    time_cnot(10000);
//    SimTableau::simulate(stdin, stdout);
}
