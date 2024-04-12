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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY out)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY out)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY out)

# Convert desired SIMD_WIDTH into machine architecture flags.

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|I386|ARM64)$")
    if(NOT(SIMD_WIDTH))
        set(MACHINE_FLAG "-march=native")
    elseif(SIMD_WIDTH EQUAL 256)
        set(MACHINE_FLAG "-mavx2" "-msse2")
    elseif(SIMD_WIDTH EQUAL 128)
        set(MACHINE_FLAG "-mno-avx2" "-msse2")
    elseif(SIMD_WIDTH EQUAL 64)
        set(MACHINE_FLAG "-mno-avx2" "-mno-sse2")
    endif()
else ()
    set(MACHINE_FLAG "")
endif()

# make changes to file_lists trigger a reconfigure
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS file_lists/source_files_no_main)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS file_lists/test_files)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS file_lists/perf_files)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS file_lists/pybind_files)

file(STRINGS file_lists/source_files_no_main SOURCE_FILES_NO_MAIN)
file(STRINGS file_lists/test_files TEST_FILES)
file(STRINGS file_lists/perf_files PERF_FILES)
file(STRINGS file_lists/pybind_files PYBIND_FILES)

add_executable(stim src/main.cc ${SOURCE_FILES_NO_MAIN})
if(NOT(MSVC))
    target_compile_options(stim PRIVATE -O3 -Wall -Wpedantic -fno-strict-aliasing ${MACHINE_FLAG})
    target_link_options(stim PRIVATE -O3)
else()
    target_compile_options(stim PRIVATE ${MACHINE_FLAG})
endif()
install(TARGETS stim RUNTIME DESTINATION bin)

add_library(libstim ${SOURCE_FILES_NO_MAIN})
set_target_properties(libstim PROPERTIES PREFIX "")
target_include_directories(libstim PUBLIC src)
if(NOT(MSVC))
    target_compile_options(libstim PRIVATE -O3 -Wall -Wpedantic -fPIC -fno-strict-aliasing ${MACHINE_FLAG})
    target_link_options(libstim PRIVATE -O3)
else()
    target_compile_options(libstim PRIVATE ${MACHINE_FLAG})
endif()
install(TARGETS libstim LIBRARY DESTINATION)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/" DESTINATION "include" FILES_MATCHING PATTERN "*.h" PATTERN "*.inl")

add_executable(stim_perf ${SOURCE_FILES_NO_MAIN} ${PERF_FILES})
if(NOT(MSVC))
    target_compile_options(stim_perf PRIVATE -Wall -Wpedantic -O3 -fno-strict-aliasing ${MACHINE_FLAG})
    target_link_options(stim_perf PRIVATE)
else()
    target_compile_options(stim_perf PRIVATE ${MACHINE_FLAG})
endif()

find_package(GTest QUIET)
if(${GTest_FOUND})
    add_executable(stim_test ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    target_link_libraries(stim_test GTest::gtest GTest::gtest_main)
    target_compile_options(stim_test PRIVATE -Wall -Wpedantic -g -fno-omit-frame-pointer -fno-strict-aliasing -fsanitize=undefined -fsanitize=address ${MACHINE_FLAG})
    target_link_options(stim_test PRIVATE -g -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=address)

    add_executable(stim_test_o3 ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
    target_link_libraries(stim_test_o3 GTest::gtest GTest::gtest_main)
    target_compile_options(stim_test_o3 PRIVATE -O3 -Wall -Wpedantic -fno-strict-aliasing ${MACHINE_FLAG})
    target_link_options(stim_test_o3 PRIVATE)
else()
    message("WARNING: Skipped stim_test target. `GTest` not found. To fix, follow Standalone CMake Project install instructions at https://github.com/google/googletest/blob/master/googletest/README.md")
endif()

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)
if (${pybind11_FOUND} AND ${Python_FOUND})
  pybind11_add_module(stim_python_bindings ${PYBIND_FILES} ${SOURCE_FILES_NO_MAIN})
  set_target_properties(stim_python_bindings PROPERTIES OUTPUT_NAME stim)
  add_compile_definitions(STIM_PYBIND11_MODULE_NAME=stim)
  if(NOT(MSVC))
      target_compile_options(stim_python_bindings PRIVATE -O3 -Wall -Wpedantic -fno-strict-aliasing ${MACHINE_FLAG})
      target_link_options(stim_python_bindings PRIVATE -O3)
  else()
      target_compile_options(stim_python_bindings PRIVATE ${MACHINE_FLAG})
  endif()

else()
  message("WARNING: Skipped stim_python_bindings target. `pybind11` not found. To fix, install pybind11. On debian based distributions, the package name is `pybind11-dev`")
endif()
