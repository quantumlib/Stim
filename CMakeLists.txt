cmake_minimum_required(VERSION 3.12)
project(stim)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)
add_compile_options(-march=native -O3)
add_link_options(-pthread -O3)

set(SOURCE_FILES_NO_MAIN
        bit_mat.cc
        bit_ptr.cc
        chp_sim.cc
        pauli_string.cc
        perf.cc
        simd_util.cc
        tableau.cc
        aligned_bits256.cc
        vector_sim.cc
        )
set(TEST_FILES
        bit_mat.test.cc
        bit_ptr.test.cc
        chp_sim.test.cc
        pauli_string.test.cc
        simd_util.test.cc
        tableau.test.cc
        aligned_bits256.test.cc
        vector_sim.test.cc
        )

add_executable(stim main.cc ${SOURCE_FILES_NO_MAIN})

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
