# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.13)
project(stim)
include_directories(src)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY out)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY out)

# Convert desired SIMD_WIDTH into machine architecture flags.
if(NOT(SIMD_WIDTH))
    set(MACHINE_FLAG "-march=native")
elseif(SIMD_WIDTH EQUAL 256)
    set(MACHINE_FLAG "-mavx2" "-msse2")
elseif(SIMD_WIDTH EQUAL 128)
    set(MACHINE_FLAG "-mno-avx2" "-msse2")
elseif(SIMD_WIDTH EQUAL 64)
    set(MACHINE_FLAG "-mno-avx2" "-mno-sse2")
endif()

set(SOURCE_FILES_NO_MAIN
        src/stim/arg_parse.cc
        src/stim/circuit/circuit.cc
        src/stim/circuit/gate_data.cc
        src/stim/circuit/gate_data_annotations.cc
        src/stim/circuit/gate_data_blocks.cc
        src/stim/circuit/gate_data_collapsing.cc
        src/stim/circuit/gate_data_controlled.cc
        src/stim/circuit/gate_data_hada.cc
        src/stim/circuit/gate_data_noisy.cc
        src/stim/circuit/gate_data_pauli.cc
        src/stim/circuit/gate_data_period_3.cc
        src/stim/circuit/gate_data_period_4.cc
        src/stim/circuit/gate_data_pp.cc
        src/stim/circuit/gate_data_swaps.cc
        src/stim/circuit/gate_target.cc
        src/stim/dem/detector_error_model.cc
        src/stim/help.cc
        src/stim/gen/circuit_gen_main.cc
        src/stim/gen/circuit_gen_params.cc
        src/stim/gen/gen_color_code.cc
        src/stim/gen/gen_rep_code.cc
        src/stim/gen/gen_surface_code.cc
        src/stim/io/measure_record_batch.cc
        src/stim/io/measure_record_batch_writer.cc
        src/stim/io/measure_record.cc
        src/stim/io/measure_record_reader.cc
        src/stim/io/measure_record_writer.cc
        src/stim/io/raii_file.cc
        src/stim/io/stim_data_formats.cc
        src/stim/io/sparse_shot.cc
        src/stim/main_namespaced.cc
        src/stim/mem/bit_ref.cc
        src/stim/mem/simd_word.cc
        src/stim/mem/simd_util.cc
        src/stim/mem/sparse_xor_vec.cc
        src/stim/probability_util.cc
        src/stim/search/graphlike/algo.cc
        src/stim/search/graphlike/edge.cc
        src/stim/search/graphlike/graph.cc
        src/stim/search/graphlike/node.cc
        src/stim/search/graphlike/search_state.cc
        src/stim/search/hyper/algo.cc
        src/stim/search/hyper/edge.cc
        src/stim/search/hyper/graph.cc
        src/stim/search/hyper/node.cc
        src/stim/search/hyper/search_state.cc
        src/stim/simulators/dem_sampler.cc
        src/stim/simulators/detection_simulator.cc
        src/stim/simulators/error_analyzer.cc
        src/stim/simulators/error_matcher.cc
        src/stim/simulators/frame_simulator.cc
        src/stim/simulators/matched_error.cc
        src/stim/simulators/measurements_to_detection_events.cc
        src/stim/simulators/tableau_simulator.cc
        src/stim/simulators/vector_simulator.cc
        src/stim/stabilizers/conversions.cc
        src/stim/stabilizers/pauli_string.cc
        src/stim/stabilizers/pauli_string_ref.cc
        src/stim/stabilizers/tableau.cc
        src/stim/stabilizers/tableau_iter.cc
        src/stim/stabilizers/tableau_specialized_prepend.cc
        src/stim/stabilizers/tableau_transposed_raii.cc
        src/stim.cc
        )
set(TEST_FILES
        src/stim/arg_parse.test.cc
        src/stim/circuit/circuit.test.cc
        src/stim/circuit/gate_data.test.cc
        src/stim/circuit/gate_target.test.cc
        src/stim/dem/detector_error_model.test.cc
        src/stim/gen/circuit_gen_main.test.cc
        src/stim/gen/circuit_gen_params.test.cc
        src/stim/gen/gen_color_code.test.cc
        src/stim/gen/gen_rep_code.test.cc
        src/stim/gen/gen_surface_code.test.cc
        src/stim/io/measure_record.test.cc
        src/stim/io/measure_record_batch.test.cc
        src/stim/io/measure_record_batch_writer.test.cc
        src/stim/io/measure_record_reader.test.cc
        src/stim/io/measure_record_writer.test.cc
        src/stim/io/sparse_shot.test.cc
        src/stim/main_namespaced.test.cc
        src/stim/mem/bit_ref.test.cc
        src/stim/mem/fixed_cap_vector.test.cc
        src/stim/mem/monotonic_buffer.test.cc
        src/stim/mem/simd_bit_table.test.cc
        src/stim/mem/simd_bits.test.cc
        src/stim/mem/simd_bits_range_ref.test.cc
        src/stim/mem/simd_word.test.cc
        src/stim/mem/simd_util.test.cc
        src/stim/mem/sparse_xor_vec.test.cc
        src/stim/probability_util.test.cc
        src/stim/search/graphlike/algo.test.cc
        src/stim/search/graphlike/edge.test.cc
        src/stim/search/graphlike/graph.test.cc
        src/stim/search/graphlike/node.test.cc
        src/stim/search/graphlike/search_state.test.cc
        src/stim/search/hyper/algo.test.cc
        src/stim/search/hyper/edge.test.cc
        src/stim/search/hyper/graph.test.cc
        src/stim/search/hyper/node.test.cc
        src/stim/search/hyper/search_state.test.cc
        src/stim/simulators/dem_sampler.test.cc
        src/stim/simulators/detection_simulator.test.cc
        src/stim/simulators/error_analyzer.test.cc
        src/stim/simulators/error_matcher.test.cc
        src/stim/simulators/frame_simulator.test.cc
        src/stim/simulators/matched_error.test.cc
        src/stim/simulators/measurements_to_detection_events.test.cc
        src/stim/simulators/tableau_simulator.test.cc
        src/stim/simulators/vector_simulator.test.cc
        src/stim/stabilizers/conversions.test.cc
        src/stim/stabilizers/pauli_string.test.cc
        src/stim/stabilizers/tableau.test.cc
        src/stim/stabilizers/tableau_iter.test.cc
        src/stim.test.cc
        src/stim_included_twice.test.cc
        src/stim/str_util.test.cc
        src/stim/test_util.test.cc
        )
set(BENCHMARK_FILES
        src/stim/benchmark_main.perf.cc
        src/stim/benchmark_util.perf.cc
        src/stim/circuit/circuit.perf.cc
        src/stim/circuit/gate_data.perf.cc
        src/stim/io/measure_record_reader.perf.cc
        src/stim/main_namespaced.perf.cc
        src/stim/mem/simd_bit_table.perf.cc
        src/stim/mem/simd_bits.perf.cc
        src/stim/mem/simd_word.perf.cc
        src/stim/mem/sparse_xor_vec.perf.cc
        src/stim/probability_util.perf.cc
        src/stim/search/graphlike/algo.perf.cc
        src/stim/simulators/dem_sampler.perf.cc
        src/stim/simulators/error_analyzer.perf.cc
        src/stim/simulators/frame_simulator.perf.cc
        src/stim/simulators/tableau_simulator.perf.cc
        src/stim/stabilizers/pauli_string.perf.cc
        src/stim/stabilizers/tableau.perf.cc
        src/stim/stabilizers/tableau_iter.perf.cc
        )
set(PYTHON_API_FILES
        src/stim/stabilizers/tableau_iter.pybind.cc
        src/stim/stabilizers/tableau.pybind.cc
        src/stim/stabilizers/pauli_string.pybind.cc
        src/stim/simulators/dem_sampler.pybind.cc
        src/stim/simulators/tableau_simulator.pybind.cc
        src/stim/simulators/matched_error.pybind.cc
        src/stim/simulators/measurements_to_detection_events.pybind.cc
        src/stim/circuit/circuit.pybind.cc
        src/stim/circuit/circuit_gate_target.pybind.cc
        src/stim/circuit/circuit_instruction.pybind.cc
        src/stim/circuit/circuit_repeat_block.pybind.cc
        src/stim/py/stim.pybind.cc
        src/stim/py/compiled_measurement_sampler.pybind.cc
        src/stim/py/base.pybind.cc
        src/stim/py/march.pybind.cc
        src/stim/py/compiled_detector_sampler.pybind.cc
        src/stim/dem/detector_error_model_target.pybind.cc
        src/stim/dem/detector_error_model_instruction.pybind.cc
        src/stim/dem/detector_error_model_repeat_block.pybind.cc
        src/stim/dem/detector_error_model.pybind.cc
        src/stim/io/read_write.pybind.cc
        )

add_executable(stim src/main.cc ${SOURCE_FILES_NO_MAIN})
if(NOT(MSVC))
    target_compile_options(stim PRIVATE -O3 -Wall -Wpedantic -fno-strict-aliasing ${MACHINE_FLAG})
    target_link_options(stim PRIVATE -pthread -O3)
else()
    target_compile_options(stim PRIVATE ${MACHINE_FLAG})
endif()

add_library(libstim ${SOURCE_FILES_NO_MAIN})
set_target_properties(libstim PROPERTIES PREFIX "")
target_include_directories(libstim PUBLIC src)
if(NOT(MSVC))
    target_compile_options(libstim PRIVATE -O3 -Wall -Wpedantic -fno-strict-aliasing ${MACHINE_FLAG})
    target_link_options(libstim PRIVATE -pthread -O3)
else()
    target_compile_options(libstim PRIVATE ${MACHINE_FLAG})
endif()

add_executable(stim_benchmark ${SOURCE_FILES_NO_MAIN} ${BENCHMARK_FILES})
if(NOT(MSVC))
    target_compile_options(stim_benchmark PRIVATE -Wall -Wpedantic -O3 -fno-strict-aliasing ${MACHINE_FLAG})
    target_link_options(stim_benchmark PRIVATE -pthread)
else()
    target_compile_options(stim_benchmark PRIVATE ${MACHINE_FLAG})
endif()

find_package(GTest QUIET)
if(${GTest_FOUND})
    add_executable(stim_test ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    target_link_libraries(stim_test GTest::gtest GTest::gtest_main)
    target_compile_options(stim_test PRIVATE --coverage -Wall -Wpedantic -Werror -fno-strict-aliasing -fsanitize=undefined -fsanitize=address ${MACHINE_FLAG})
    target_link_options(stim_test PRIVATE --coverage -fsanitize=undefined -fsanitize=address -pthread)

    add_executable(stim_test_o3 ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    target_link_libraries(stim_test_o3 GTest::gtest GTest::gtest_main)
    target_compile_options(stim_test_o3 PRIVATE -O3 -Wall -Wpedantic -Werror -fno-strict-aliasing ${MACHINE_FLAG})
    target_link_options(stim_test_o3 PRIVATE -pthread)
else()
    message("WARNING: Skipped stim_test target. `GTest` not found. To fix, follow Standalone CMake Project install instructions at https://github.com/google/googletest/blob/master/googletest/README.md")
endif()

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)
if (${pybind11_FOUND} AND ${Python_FOUND})
  pybind11_add_module(stim_python_bindings ${PYTHON_API_FILES} ${SOURCE_FILES_NO_MAIN})
  set_target_properties(stim_python_bindings PROPERTIES OUTPUT_NAME stim)
  add_compile_definitions(STIM_PYBIND11_MODULE_NAME=stim)
else()
  message("WARNING: Skipped stim_python_bindings target. `pybind11` not found")
endif()
