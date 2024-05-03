#ifndef _STIM_H
#define _STIM_H
/// WARNING: THE STIM C++ API MAKES NO COMPATIBILITY GUARANTEES.
/// It may change arbitrarily and catastrophically from minor version to minor version.
/// If you need a stable API, use stim's Python API.
#include "stim/circuit/circuit.h"
#include "stim/circuit/circuit_instruction.h"
#include "stim/circuit/gate_decomposition.h"
#include "stim/circuit/gate_target.h"
#include "stim/cmd/command_help.h"
#include "stim/dem/dem_instruction.h"
#include "stim/dem/detector_error_model.h"
#include "stim/gates/gates.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_color_code.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/io/measure_record.h"
#include "stim/io/measure_record_batch.h"
#include "stim/io/measure_record_batch_writer.h"
#include "stim/io/measure_record_reader.h"
#include "stim/io/measure_record_writer.h"
#include "stim/io/raii_file.h"
#include "stim/io/sparse_shot.h"
#include "stim/io/stim_data_formats.h"
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
#include "stim/search/graphlike/algo.h"
#include "stim/search/graphlike/edge.h"
#include "stim/search/graphlike/graph.h"
#include "stim/search/graphlike/node.h"
#include "stim/search/graphlike/search_state.h"
#include "stim/search/hyper/algo.h"
#include "stim/search/hyper/edge.h"
#include "stim/search/hyper/graph.h"
#include "stim/search/hyper/node.h"
#include "stim/search/hyper/search_state.h"
#include "stim/search/sat/wcnf.h"
#include "stim/search/search.h"
#include "stim/stabilizers/flex_pauli_string.h"
#include "stim/stabilizers/flow.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/pauli_string_iter.h"
#include "stim/stabilizers/pauli_string_ref.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau_iter.h"
#include "stim/stabilizers/tableau_transposed_raii.h"
#include "stim/util_bot/arg_parse.h"
#include "stim/util_bot/error_decomp.h"
#include "stim/util_bot/probability_util.h"
#include "stim/util_bot/str_util.h"
#include "stim/util_bot/twiddle.h"
#endif
