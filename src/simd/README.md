# `simd` directory

This directory contains code related to exposing the same-instruction-multiple-data constructs
required by other parts of the code in a potentially cross-platform fashion.

The key types as `simd_bits` and `simd_bit_table`, which expose 1d and 2d bit-packed booleans
that can be operated on by SSE or AVX instructions.
Another important type is `simd_word`, which supports graceful degradation from AVX to SSE to
raw 64 bit integers via the `SIMD_WIDTH` definition.
