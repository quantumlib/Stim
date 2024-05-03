#ifndef _STIM_H
#define _STIM_H
/// WARNING: THE STIM C++ API MAKES NO COMPATIBILITY GUARANTEES.
/// It may change arbitrarily and catastrophically from minor version to minor version.
/// If you need a stable API, use stim's Python API.
#include "stim/cmd/command_help.h"
#include "stim/gates/gates.h"
#include "stim/main_namespaced.h"
#include "stim/mem/bit_ref.h"
#include "stim/mem/bitword.h"
#include "stim/mem/bitword_128_sse.h"
#include "stim/mem/bitword_256_avx.h"
#include "stim/mem/bitword_64.h"
#include "stim/mem/fixed_cap_vector.h"
#include "stim/mem/monotonic_buffer.h"
#include "stim/mem/simd_bit_table.h"
#include "stim/mem/simd_bits.h"
#include "stim/mem/simd_bits_range_ref.h"
#include "stim/mem/simd_util.h"
#include "stim/mem/simd_word.h"
#include "stim/mem/span_ref.h"
#include "stim/mem/sparse_xor_vec.h"
#include "stim/util_bot/arg_parse.h"
#include "stim/util_bot/error_decomp.h"
#include "stim/util_bot/probability_util.h"
#include "stim/util_bot/str_util.h"
#include "stim/util_bot/twiddle.h"
#endif
