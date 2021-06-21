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

cmake_minimum_required(VERSION 3.15)
project(stim)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY out)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY out)

# Convert desired SIMD_WIDTH into machine architecture flags.
if(NOT(SIMD_WIDTH))
    set(MACHINE_FLAG "-march=native")
elseif(SIMD_WIDTH EQUAL 256)
    set(MACHINE_FLAG "-mavx2")
elseif(SIMD_WIDTH EQUAL 128)
    set(MACHINE_FLAG "-msse2")
elseif(SIMD_WIDTH EQUAL 64)
    set(MACHINE_FLAG "")
endif()

set(SOURCE_FILES_NO_MAIN
        src/arg_parse.cc
        src/circuit/circuit.cc
        src/circuit/gate_data.cc
        src/circuit/gate_data_annotations.cc
        src/circuit/gate_data_blocks.cc
        src/circuit/gate_data_collapsing.cc
        src/circuit/gate_data_controlled.cc
        src/circuit/gate_data_hada.cc
        src/circuit/gate_data_noisy.cc
        src/circuit/gate_data_pauli.cc
        src/circuit/gate_data_period_3.cc
        src/circuit/gate_data_period_4.cc
        src/circuit/gate_data_pp.cc
        src/circuit/gate_data_swaps.cc
        src/dem/detector_error_model.cc
        src/gate_help.cc
        src/gen/circuit_gen_main.cc
        src/gen/circuit_gen_params.cc
        src/gen/gen_color_code.cc
        src/gen/gen_rep_code.cc
        src/gen/gen_surface_code.cc
        src/main_helper.cc
        src/probability_util.cc
        src/simd/bit_ref.cc
        src/simd/simd_bit_table.cc
        src/simd/simd_bits.cc
        src/simd/simd_bits_range_ref.cc
        src/simd/simd_compat.cc
        src/simd/simd_util.cc
        src/simd/sparse_xor_vec.cc
        src/simulators/detection_simulator.cc
        src/simulators/error_analyzer.cc
        src/simulators/frame_simulator.cc
        src/simulators/measure_record_batch.cc
        src/simulators/measure_record_batch_writer.cc
        src/simulators/measure_record.cc
        src/simulators/measure_record_writer.cc
        src/simulators/tableau_simulator.cc
        src/simulators/vector_simulator.cc
        src/stabilizers/pauli_string.cc
        src/stabilizers/pauli_string_ref.cc
        src/stabilizers/tableau.cc
        src/stabilizers/tableau_specialized_prepend.cc
        src/stabilizers/tableau_transposed_raii.cc
        )
set(TEST_FILES
        src/arg_parse.test.cc
        src/circuit/circuit.test.cc
        src/circuit/gate_data.test.cc
        src/dem/detector_error_model.test.cc
        src/gen/circuit_gen_main.test.cc
        src/gen/circuit_gen_params.test.cc
        src/gen/gen_color_code.test.cc
        src/gen/gen_rep_code.test.cc
        src/gen/gen_surface_code.test.cc
        src/main_helper.test.cc
        src/probability_util.test.cc
        src/simd/bit_ref.test.cc
        src/simd/monotonic_buffer.test.cc
        src/simd/simd_bit_table.test.cc
        src/simd/simd_bits.test.cc
        src/simd/simd_bits_range_ref.test.cc
        src/simd/simd_compat.test.cc
        src/simd/simd_util.test.cc
        src/simd/sparse_xor_vec.test.cc
        src/simulators/detection_simulator.test.cc
        src/simulators/error_analyzer.test.cc
        src/simulators/frame_simulator.test.cc
        src/simulators/measure_record.test.cc
        src/simulators/measure_record_batch.test.cc
        src/simulators/measure_record_batch_writer.test.cc
        src/simulators/measure_record_writer.test.cc
        src/simulators/tableau_simulator.test.cc
        src/simulators/vector_simulator.test.cc
        src/stabilizers/pauli_string.test.cc
        src/stabilizers/tableau.test.cc
        src/test_util.test.cc
        )
set(BENCHMARK_FILES
        src/benchmark_main.perf.cc
        src/benchmark_util.perf.cc
        src/circuit/circuit.perf.cc
        src/circuit/gate_data.perf.cc
        src/main.perf.cc
        src/probability_util.perf.cc
        src/simd/simd_bit_table.perf.cc
        src/simd/simd_bits.perf.cc
        src/simd/simd_compat.perf.cc
        src/simd/sparse_xor_vec.perf.cc
        src/simulators/error_analyzer.perf.cc
        src/simulators/frame_simulator.perf.cc
        src/simulators/tableau_simulator.perf.cc
        src/stabilizers/pauli_string.perf.cc
        src/stabilizers/tableau.perf.cc
        )

add_executable(stim src/main.cc ${SOURCE_FILES_NO_MAIN})
if(NOT(MSVC))
    target_compile_options(stim PRIVATE -O3 -Wall -Wpedantic ${MACHINE_FLAG})
    target_link_options(stim PRIVATE -pthread -O3)
else()
    target_compile_options(stim PRIVATE ${MACHINE_FLAG})
endif()

add_library(libstim ${SOURCE_FILES_NO_MAIN})
target_include_directories(libstim PUBLIC src/include)
set_target_properties(libstim PROPERTIES PREFIX "")
if(NOT(MSVC))
    target_compile_options(libstim PRIVATE -O3 -Wall -Wpedantic ${MACHINE_FLAG})
    target_link_options(libstim PRIVATE -pthread -O3)
else()
    target_compile_options(libstim PRIVATE ${MACHINE_FLAG})
endif()

add_executable(stim_benchmark ${SOURCE_FILES_NO_MAIN} ${BENCHMARK_FILES})
if(NOT(MSVC))
    target_compile_options(stim_benchmark PRIVATE -Wall -Wpedantic -O3 ${MACHINE_FLAG})
    target_link_options(stim_benchmark PRIVATE -pthread)
else()
    target_compile_options(stim_benchmark PRIVATE ${MACHINE_FLAG})
endif()

find_package(GTest QUIET)
if(${GTest_FOUND})
    add_executable(stim_test ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    add_test(stim_test ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    target_link_libraries(stim_test gtest gtest_main)
    target_compile_options(stim_test PRIVATE -Wall -Wpedantic -Werror -fsanitize=undefined -fsanitize=address ${MACHINE_FLAG})
    target_link_options(stim_test PRIVATE -fsanitize=undefined -fsanitize=address -pthread)

    add_executable(stim_test_o3 ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    add_test(stim_test_o3 ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    target_link_libraries(stim_test_o3 gtest gtest_main)
    target_compile_options(stim_test_o3 PRIVATE -O3 -Wall -Wpedantic -Werror ${MACHINE_FLAG})
    target_link_options(stim_test_o3 PRIVATE -pthread)
else()
    message("WARNING: Skipped stim_test target. `GTest` not found. To fix, follow Standalone CMake Project install instructions at https://github.com/google/googletest/blob/master/googletest/README.md")
endif()
