cmake_minimum_required(VERSION 3.15)
project(stim)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)

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
        src/circuit/common_circuits.cc
        src/circuit/gate_data.cc
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
        src/simulators/error_fuser.cc
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
        src/circuit/circuit.test.cc
        src/circuit/common_circuits.test.cc
        src/circuit/gate_data.test.cc
        src/main_helper.test.cc
        src/probability_util.test.cc
        src/simd/bit_ref.test.cc
        src/simd/simd_bit_table.test.cc
        src/simd/simd_bits.test.cc
        src/simd/simd_bits_range_ref.test.cc
        src/simd/simd_compat.test.cc
        src/simd/simd_util.test.cc
        src/simd/sparse_xor_vec.test.cc
        src/simulators/detection_simulator.test.cc
        src/simulators/error_fuser.test.cc
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
        src/circuit/circuit.perf.cc
        src/circuit/gate_data.perf.cc
        src/simd/simd_bit_table.perf.cc
        src/simd/simd_bits.perf.cc
        src/simd/simd_compat.perf.cc
        src/simd/sparse_xor_vec.perf.cc
        src/simulators/error_fuser.perf.cc
        src/simulators/frame_simulator.perf.cc
        src/simulators/tableau_simulator.perf.cc
        src/stabilizers/pauli_string.perf.cc
        src/stabilizers/tableau.perf.cc
        )

add_executable(stim src/main.cc ${SOURCE_FILES_NO_MAIN})
if(NOT(MSVC))
    target_compile_options(stim PRIVATE -O3 -Wall -Wpedantic -Werror ${MACHINE_FLAG})
    target_link_options(stim PRIVATE -pthread -O3)
else()
    target_compile_options(stim PRIVATE -DSIMD_WIDTH=${SIMD_WIDTH})
endif()

add_executable(stim_benchmark ${SOURCE_FILES_NO_MAIN} ${BENCHMARK_FILES})
if(NOT(MSVC))
    target_compile_options(stim_benchmark PRIVATE -Wall -Wpedantic -Werror -O3 ${MACHINE_FLAG})
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
