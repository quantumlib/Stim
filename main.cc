#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include "simd_util.h"
#include "pauli_string.h"
#include <chrono>
#include <random>

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis(
            std::numeric_limits<std::uint64_t>::min(),
            std::numeric_limits<std::uint64_t>::max());
    constexpr size_t w = 256 * 256;
    size_t num_bits = w * w;
    size_t num_u64 = num_bits / 64;
    auto data = (uint64_t *) _mm_malloc(num_u64 * sizeof(uint64_t), 32);
    for (size_t k = 0; k < num_u64; k++) {
        data[k] = dis(gen);
    }

    auto start = std::chrono::steady_clock::now();
    size_t n = 10;
    for (size_t i = 0; i < n; i++) {
        transpose_bit_matrix(data, w);
    }
    auto end = std::chrono::steady_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0 / 1000.0;
    std::cout << n / dt << " transposes/sec (" << w << "x" << w << ", " << (num_bits >> 23) << " MiB)\n";

    start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < n; i++) {
        transpose_bit_matrix_256x256blocks(data, w);
    }
    end = std::chrono::steady_clock::now();
    dt = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0 / 1000.0;
    std::cout << n / dt << " block transposes/sec (" << w << "x" << w << ", " << (num_bits >> 23) << " MiB)\n";
}
