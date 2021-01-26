cmake_minimum_required(VERSION 3.12)
project(stim)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)
if(NOT(SIMD_WIDTH))
    message("Defaulted SIMD_WIDTH to 256. Pass -DSIMD_WIDTH=128 or -DSIMD_WIDTH=64 into cmake for other avx widths.")
    set(SIMD_WIDTH 256)
endif()

set(SOURCE_FILES_NO_MAIN
        src/arg_parse.cc
        src/circuit.cc
        src/probability_util.cc
        src/simd/bit_ref.cc
        src/simd/simd_bit_table.cc
        src/simd/simd_bits.cc
        src/simd/simd_bits_range_ref.cc
        src/simd/simd_compat.cc
        src/simd/simd_util.cc
        src/simulators/gate_data.cc
        src/simulators/frame_simulator.cc
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
        src/circuit.test.cc
        src/probability_util.test.cc
        src/simd/bit_ref.test.cc
        src/simd/simd_bit_table.test.cc
        src/simd/simd_bits.test.cc
        src/simd/simd_bits_range_ref.test.cc
        src/simd/simd_compat.test.cc
        src/simd/simd_util.test.cc
        src/simulators/frame_simulator.test.cc
        src/simulators/tableau_simulator.test.cc
        src/simulators/vector_simulator.test.cc
        src/stabilizers/pauli_string.test.cc
        src/stabilizers/tableau.test.cc
        src/test_util.test.cc
        )

set(BENCHMARK_FILES
        src/benchmark_main.perf.cc
        src/benchmark_util.perf.cc
        src/simd/simd_bits.perf.cc
        src/simd/simd_bit_table.perf.cc
        src/simd/simd_compat.perf.cc
        src/simulators/frame_simulator.perf.cc
        src/simulators/tableau_simulator.perf.cc
        src/stabilizers/pauli_string.perf.cc
        src/stabilizers/tableau.perf.cc
        )

add_executable(stim src/main.cc ${SOURCE_FILES_NO_MAIN})
if(NOT(MSVC))
    target_compile_options(stim PRIVATE -march=native -O3 -Wall -Wpedantic -Werror -DSIMD_WIDTH=${SIMD_WIDTH})
    target_link_options(stim PRIVATE -pthread -O3)
else()
    target_compile_options(stim PRIVATE -DSIMD_WIDTH=${SIMD_WIDTH})
endif()

add_executable(stim_benchmark ${SOURCE_FILES_NO_MAIN} ${BENCHMARK_FILES})
if(NOT(MSVC))
    target_compile_options(stim_benchmark PRIVATE -march=native -Wall -Wpedantic -Werror -O3 -DSIMD_WIDTH=${SIMD_WIDTH})
    target_link_options(stim_benchmark PRIVATE -pthread)
else()
    target_compile_options(stim_benchmark PRIVATE -DSIMD_WIDTH=${SIMD_WIDTH})
endif()

find_package(GTest QUIET)
if(${GTest_FOUND})
    add_executable(stim_test ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    add_test(stim_test ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    target_link_libraries(stim_test gtest gtest_main)
    target_compile_options(stim_test PRIVATE -march=native -Wall -Wpedantic -Werror -DSIMD_WIDTH=${SIMD_WIDTH})
    target_link_options(stim_test PRIVATE -pthread)
else()
    message("WARNING: Skipped stim_test target. `GTest` not found. To fix, follow Standalone CMake Project install instructions at https://github.com/google/googletest")
endif()
