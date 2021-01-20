cmake_minimum_required(VERSION 3.12)
project(stim)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)

set(SOURCE_FILES_NO_MAIN
        src/aligned_bits256.cc
        src/arg_parse.cc
        src/bit_mat.cc
        src/bit_ptr.cc
        src/circuit.cc
        src/pauli_string.cc
        src/program_frame.cc
        src/perf.cc
        src/probability_util.cc
        src/sim_tableau.cc
        src/sim_bulk_pauli_frame.cc
        src/sim_vector.cc
        src/simd_util.cc
        src/tableau.cc
        )
set(TEST_FILES
        test/aligned_bits256.test.cc
        test/arg_parse.test.cc
        test/bit_mat.test.cc
        test/bit_ptr.test.cc
        test/circuit.test.cc
        test/pauli_string.test.cc
        test/program_frame.test.cc
        test/probability_util.test.cc
        test/sim_tableau.test.cc
        test/sim_bulk_pauli_frame.test.cc
        test/sim_vector.test.cc
        test/simd_util.test.cc
        test/tableau.test.cc
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
