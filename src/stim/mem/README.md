# `mem` directory

This directory contains code related to efficiently packing data into memory.

For example, it exposes `simd_bits` which manages an array of booleans that are bit-packed, padded,
and aligned so that same-instruction-multiple-data constructs can be safely used on the bits.

A key behind-the-scenes type is `simd_word`, which has different implementations depending on the
instructions supported by the machine architecture. In particular, includes graceful degradation from
AVX to SSE to raw 64 bit integers.
