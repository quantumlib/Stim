cmake_minimum_required(VERSION 3.12)
project(stim)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)

set(SOURCE_FILES_NO_MAIN
        aligned_bits256.cc
        bit_mat.cc
        bit_ptr.cc
        circuit.cc
        pauli_string.cc
        perf.cc
        sim_tableau.cc
        sim_frame.cc
        sim_vector.cc
        simd_util.cc
        tableau.cc
        )
set(TEST_FILES
        aligned_bits256.test.cc
        bit_mat.test.cc
        bit_ptr.test.cc
        circuit.test.cc
        pauli_string.test.cc
        sim_tableau.test.cc
        sim_frame.test.cc
        sim_vector.test.cc
        simd_util.test.cc
        tableau.test.cc
        )

add_executable(stim main.cc ${SOURCE_FILES_NO_MAIN})
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
