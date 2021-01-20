cmake_minimum_required(VERSION 3.12)
project(stim)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)

set(SOURCE_FILES_NO_MAIN
        src/arg_parse.cc
        src/bit_mat.cc
        src/circuit.cc
        src/pauli_string.cc
        src/program_frame.cc
        src/perf.cc
        src/probability_util.cc
        src/sim_bulk_pauli_frame.cc
        src/simd/bit_ptr.cc
        src/simd/simd_bits.cc
        src/simd/simd_range.cc
        src/simd/simd_util.cc
        src/sim_tableau.cc
        src/sim_vector.cc
        src/tableau.cc
        )
set(TEST_FILES
        src/arg_parse.test.cc
        src/bit_mat.test.cc
        src/circuit.test.cc
        src/pauli_string.test.cc
        src/program_frame.test.cc
        src/probability_util.test.cc
        src/sim_bulk_pauli_frame.test.cc
        src/simd/bit_ptr.test.cc
        src/simd/simd_bits.test.cc
        src/simd/simd_range.test.cc
        src/simd/simd_util.test.cc
        src/sim_tableau.test.cc
        src/sim_vector.test.cc
        src/tableau.test.cc
        )

add_executable(stim src/main.cc ${SOURCE_FILES_NO_MAIN})
target_compile_options(stim PRIVATE -march=native -O3 -Wall -Wpedantic -Werror)
target_link_options(stim PRIVATE -pthread -O3)

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)
add_executable(stim_tests ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})

target_link_libraries(stim_tests gtest_main)
add_test(stim_tests stim_tests)
target_compile_options(stim_tests PRIVATE -march=native -Wall -Wpedantic -Werror)
target_link_options(stim_tests PRIVATE -pthread)
