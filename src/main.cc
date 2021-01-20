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
#include "simd/simd_util.h"
#include <cstring>
#include "circuit.h"
#include "sim_bulk_pauli_frame.h"
#include "arg_parse.h"

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

void time_sim_bulk_pauli_frame(size_t distance, size_t num_samples) {
    std::cerr << "time_sim_bulk_pauli_frame(unrotated surface code distance=" << distance << ", samples=" << num_samples << ")\n";
    auto circuit = surface_code_circuit(distance);
    auto frame_program = PauliFrameProgram::from_stabilizer_circuit(circuit.operations);
    auto sim = SimBulkPauliFrames(frame_program.num_qubits, num_samples, frame_program.num_measurements);
    auto f = PerfResult::time([&]() {
        sim.clear_and_run(frame_program);
    });
    std::cerr << f << " (sample rate " << si_describe(f.rate() * num_samples) << "Hz)";
    std::cerr << "\n";
}

void time_sim_bulk_pauli_frame_h(size_t num_qubits, size_t num_samples) {
    std::cerr << "time_sim_bulk_pauli_frame_h(num_qubits=" << num_qubits
        << ",num_samples=" << num_samples << ")\n";
    auto sim = SimBulkPauliFrames(num_qubits, num_samples, 1);
    std::vector<size_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    auto f = PerfResult::time([&]() {
        sim.H_XY(targets);
    });
    std::cerr << f;
    std::cerr << "\n";
}

void time_sim_bulk_pauli_frame_cz(size_t num_qubits, size_t num_samples) {
    std::cerr << "time_sim_bulk_pauli_frame_cz(num_qubits=" << num_qubits
        << ",num_samples=" << num_samples << ")\n";
    auto sim = SimBulkPauliFrames(num_qubits, num_samples, 1);
    std::vector<size_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    auto f = PerfResult::time([&]() {
        sim.CZ(targets);
    });
    std::cerr << f;
    std::cerr << "\n";
}

void time_any_non_zero(size_t num_bits) {
    std::cerr << "any_non_zero(num_bits=" << num_bits << ")\n";
    auto d = simd_bits::random(num_bits);
    auto f = PerfResult::time([&]() {
        any_non_zero(d.u256, ceil256(d.num_bits) >> 8);
    });
    std::cerr << f;
    std::cerr << "\n";
}

void time_sim_bulk_pauli_frame_swap(size_t num_qubits, size_t num_samples) {
    std::cerr << "time_sim_bulk_pauli_frame_swap(num_qubits=" << num_qubits
        << ",num_samples=" << num_samples << ")\n";
    auto sim = SimBulkPauliFrames(num_qubits, num_samples, 1);
    std::vector<size_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    auto f = PerfResult::time([&]() {
        sim.SWAP(targets);
    });
    std::cerr << f;
    std::cerr << "\n";
}

void time_sim_bulk_pauli_frame_depolarize(size_t num_qubits, size_t num_samples, float probability) {
    std::cerr << "time_sim_bulk_pauli_frame_depolarize(num_qubits=" << num_qubits
        << ",num_samples=" << num_samples
        << ",probability=" << probability << ")\n";
    auto sim = SimBulkPauliFrames(num_qubits, num_samples, 1);
    std::vector<size_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    auto f = PerfResult::time([&]() {
        sim.DEPOLARIZE(targets, probability);
    });
    std::cerr << f << " (hit rate " << si_describe(f.rate() * num_samples * num_qubits * probability) << "Hz)";
    std::cerr << " (attempt rate " << si_describe(f.rate() * num_samples * num_qubits) << "Hz)";
    std::cerr << "\n";
}

void time_sim_bulk_pauli_frame_depolarize2(size_t num_qubits, size_t num_samples, float probability) {
    std::cerr << "time_sim_bulk_pauli_frame_depolarize2(num_qubits=" << num_qubits
        << ",num_samples=" << num_samples
        << ",probability=" << probability << ")\n";
    auto sim = SimBulkPauliFrames(num_qubits, num_samples, 1);
    std::vector<size_t> targets;
    for (size_t k = 0; k < num_qubits; k++) {
        targets.push_back(k);
    }
    auto f = PerfResult::time([&]() {
        sim.DEPOLARIZE2(targets, probability);
    });
    std::cerr << f << " (hit rate " << si_describe(f.rate() * num_samples * num_qubits * probability) << "Hz)";
    std::cerr << " (attempt rate " << si_describe(f.rate() * num_samples * num_qubits) << "Hz)";
    std::cerr << "\n";
}

void time_transpose_blockwise(size_t blocks) {
    std::cerr << "transpose_bit_matrix_256x256blocks(blocks=" << blocks << "; ";
    std::cerr << (blocks * 256 * 256 / 8 / 1024 / 1024) << "MiB)\n";

    size_t num_bits = blocks << 16;
    auto data = simd_bits::random(num_bits);
    auto f = PerfResult::time([&](){
        blockwise_transpose_256x256(data.u64, num_bits);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * blocks / 1000.0 / 1000.0 << " MegaBlocks/s)";
    std::cerr << "\n";
}

void time_transpose_tableau(size_t num_qubits) {
    std::cerr << "transpose_tableau(num_qubits=" << num_qubits << "; ";

    Tableau tableau(num_qubits);
    std::cerr << (tableau.xs.xs.num_bits * 4 / 8 / 1024 / 1024) << "MiB)\n";
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
    auto f = PerfResult::time([&](){
        p1.ref().inplace_right_mul_returning_log_i_scalar(p2);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * num_qubits / 1000 / 1000 / 1000 << " GigaPaulis/s)";
    std::cerr << "\n";
}

void time_memcpy(size_t bits) {
    std::cerr << "memcpy (bits=" << bits << ")\n";

    auto d1 = simd_bits::random(bits * 2);
    auto d2 = simd_bits::random(bits * 2);
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
    auto f = PerfResult::time([&](){
        t.xs[0].inplace_right_mul_returning_log_i_scalar(t.xs[5]);
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
    auto f = PerfResult::time([&](){
        p1.ref().swap_with(p2);
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() * num_qubits / 1000 / 1000 / 1000 << " GigaPauliSwaps/s)";
    std::cerr << "\n";
}

void time_cnot(size_t num_qubits) {
    std::cerr << "cnot(n=" << num_qubits << ")\n";

    SimTableau sim(num_qubits);
    auto f = PerfResult::time([&](){
        sim.CX({0, num_qubits - 1});
    });
    std::cerr << f;
    std::cerr << " (" << f.rate() / 1000 / 1000 << " MegaCNOT/s)";
    std::cerr << "\n";
}

void bulk_sample(size_t num_samples, SampleFormat format, FILE *in, FILE *out) {
    auto circuit = Circuit::from_file(in);
    auto program = PauliFrameProgram::from_stabilizer_circuit(circuit.operations);
    program.sample_out(num_samples, out, format);
}

void profile() {
//    time_transpose_blockwise(100);
//    time_transpose_tableau(20000);
//    time_pauli_multiplication(100000);
    time_pauli_multiplication(1000000);
//    time_pauli_multiplication(10000000);
//    time_memcpy(100000);
//    time_memcpy(1000000);
//    time_memcpy(10000000);
//    time_tableau_pauli_multiplication(10000);
//    time_pauli_swap(100000);
//    time_tableau_sim(51);
//    time_sim_bulk_pauli_frame(51, 1024);
//    time_sim_bulk_pauli_frame_depolarize(10000, 1000, 0.001);
//    time_sim_bulk_pauli_frame_depolarize2(10000, 1000, 0.001);
//    time_any_non_zero(1000*1000*1000);
//    time_sim_bulk_pauli_frame_h(10000, 1000);
//    time_sim_bulk_pauli_frame_cz(10000, 1000);
//    time_sim_bulk_pauli_frame_swap(10000, 1000);
//    time_pauli_frame_sim(51);
//    time_cnot(10000);
}

std::vector<const char *> known_arguments {
        "-shots",
        "-profile",
        "-repl",
        "-format",
        "-out",
};
std::vector<const char *> format_names {
        "ascii",
        "bin_LE8",
        "RAW_UNSTABLE",
};
std::vector<SampleFormat > format_values {
        SAMPLE_FORMAT_ASCII,
        SAMPLE_FORMAT_BINLE8,
        SAMPLE_FORMAT_RAW_UNSTABLE,
};

int main(int argc, const char** argv) {
    check_for_unknown_arguments(known_arguments.size(), known_arguments.data(), argc, argv);

    SampleFormat format = format_values[find_enum_argument("-format", 0, format_names.size(), format_names.data(), argc, argv)];
    bool interactive = find_bool_argument("-repl", argc, argv);
    bool profiling = find_bool_argument("-profile", argc, argv);
    bool forced_sampling = find_argument("-shots", argc, argv) != nullptr || interactive;
    int samples = find_int_argument("-shots", 1, 0, 1 << 30, argc, argv);
    const char *out_path = find_argument("-out", argc, argv);
    FILE *out;
    if (out_path == nullptr) {
        out = stdout;
    } else {
        out = fopen(out_path, "w");
        if (out == nullptr) {
            std::cerr << "Failed to open '" << out_path << "' to write.";
            exit(EXIT_FAILURE);
        }
    }

    if (forced_sampling && profiling) {
        std::cerr << "Incompatible arguments. -profile when sampling.\n";
        exit(EXIT_FAILURE);
    }
    if (samples != 1 && interactive) {
        std::cerr << "Incompatible arguments. Multiple samples and interactive.\n";
        exit(EXIT_FAILURE);
    }
    if (interactive && format != SAMPLE_FORMAT_ASCII) {
        std::cerr << "Incompatible arguments. Binary output format and repl.\n";
        exit(EXIT_FAILURE);
    }

    if (profiling) {
        profile();
        exit(EXIT_SUCCESS);
    }

    if (samples == 1 && format == SAMPLE_FORMAT_ASCII) {
        SimTableau::simulate(stdin, out, interactive);
    } else {
        bulk_sample((size_t) samples, format, stdin, out);
    }
}
