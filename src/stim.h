#ifndef _STIM_H
#define _STIM_H
/// WARNING: THE STIM C++ API MAKES NO COMPATIBILITY GUARANTEES.
/// It may change arbitrarily and catastrophically from minor version to minor version.
/// If you need a stable API, use stim's Python API.
#include "stim/arg_parse.h"
#include "stim/circuit/circuit.h"
#include "stim/circuit/circuit_instruction.h"
#include "stim/circuit/export_qasm.h"
#include "stim/circuit/gate_data.h"
#include "stim/circuit/gate_data_table.h"
#include "stim/circuit/gate_decomposition.h"
#include "stim/circuit/gate_target.h"
#include "stim/circuit/stabilizer_flow.h"
#include "stim/cmd/command_analyze_errors.h"
#include "stim/cmd/command_convert.h"
#include "stim/cmd/command_detect.h"
#include "stim/cmd/command_diagram.h"
#include "stim/cmd/command_explain_errors.h"
#include "stim/cmd/command_gen.h"
#include "stim/cmd/command_help.h"
#include "stim/cmd/command_m2d.h"
#include "stim/cmd/command_repl.h"
#include "stim/cmd/command_sample.h"
#include "stim/cmd/command_sample_dem.h"
#include "stim/dem/dem_instruction.h"
#include "stim/dem/detector_error_model.h"
#include "stim/diagram/ascii_diagram.h"
#include "stim/diagram/base64.h"
#include "stim/diagram/basic_3d_diagram.h"
#include "stim/diagram/circuit_timeline_helper.h"
#include "stim/diagram/coord.h"
#include "stim/diagram/crumble.h"
#include "stim/diagram/crumble_data.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/diagram_util.h"
#include "stim/diagram/gate_data_3d.h"
#include "stim/diagram/gate_data_3d_texture_data.h"
#include "stim/diagram/gate_data_svg.h"
#include "stim/diagram/gltf.h"
#include "stim/diagram/graph/match_graph_3d_drawer.h"
#include "stim/diagram/graph/match_graph_svg_drawer.h"
#include "stim/diagram/json_obj.h"
#include "stim/diagram/lattice_map.h"
#include "stim/diagram/timeline/timeline_3d_drawer.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"
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
#include "stim/probability_util.h"
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
#include "stim/search/search.h"
#include "stim/simulators/count_determined_measurements.h"
#include "stim/simulators/dem_sampler.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/error_matcher.h"
#include "stim/simulators/force_streaming.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/frame_simulator_util.h"
#include "stim/simulators/graph_simulator.h"
#include "stim/simulators/matched_error.h"
#include "stim/simulators/measurements_to_detection_events.h"
#include "stim/simulators/sparse_rev_frame_tracker.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/transform_without_feedback.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/conversions.h"
#include "stim/stabilizers/flex_pauli_string.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/pauli_string_iter.h"
#include "stim/stabilizers/pauli_string_ref.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau_iter.h"
#include "stim/stabilizers/tableau_transposed_raii.h"
#include "stim/str_util.h"
#endif
