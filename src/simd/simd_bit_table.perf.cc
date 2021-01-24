#include "../benchmark_util.h"
#include "simd_bit_table.h"

BENCHMARK(simd_bit_table_inplace_square_transpose_diam10K) {
    auto n = 10 * 1000;
    simd_bit_table table(n, n);
    benchmark_go([&](){
        table.do_square_transpose();
    }).goal_millis(6).show_rate("Bits", n * n);
}
