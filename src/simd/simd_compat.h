#ifndef SIMD_COMPAT_H
#define SIMD_COMPAT_H

#include <cstdint>
#include <immintrin.h>
#include <iostream>
#include "simd_util.h"

#ifndef SIMD_WIDTH
#error SIMD_WIDTH must be defined. For example, pass -DSIMD_WIDTH=256 as a compiler flag.
#endif

#if SIMD_WIDTH == 256
#include "simd_compat_256.h"
#elif SIMD_WIDTH == 128
#include "simd_compat_128.h"
#elif SIMD_WIDTH == 64
#include "simd_compat_64.h"
#else
#error Unsupported SIMD_WIDTH value. Supported values are 256 (default), 128, 64.
#endif

#endif
