#include <iostream>
#include <immintrin.h>

int main() {
    std::cout << "Start\n";
    alignas(256) uint64_t x[] = {1, 2, 3, 4, 5, 6, 7, 8};
    alignas(256) uint64_t y[] = {2, 3, 5, 7, 11, 13, 17, 19};
    alignas(256) uint64_t z[] = {0, 0, 0, 0, 0, 0, 0, 0};
    __m256i a = _mm256_load_si256((__m256i*)x);
    __m256i b = _mm256_load_si256((__m256i*)y);
    __m256i c = _mm256_add_epi64(a, b);
    _mm256_store_si256((__m256i*)x, a);
    _mm256_store_si256((__m256i*)y, b);
    _mm256_store_si256((__m256i*)z, c);
    for (int i = 0; i < 8; i++) {
        std::cout << i << ": " << x[0] << " + " << y[i] << " = " << z[i] << "\n";
    }
    return 0;
}
