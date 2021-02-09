# `simd` directory

This directory contains code related to exposing the same-instruction-multiple-data constructs
required by other parts of the code in a cross-platform fashion.

The key behind-the-scenes type is `simd_word`, which has multiple implementations allowing graceful degradation from AVX to SSE to
raw 64 bit integers.
Other key types are `simd_bits` and `simd_bit_table`, which expose 1d and 2d bit-packed boolean data
and methods to operate on that data in a vectorized way.
