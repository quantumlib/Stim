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

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS file_lists/source_files_no_main)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS file_lists/test_files)

file(STRINGS file_lists/source_files_no_main SOURCE_FILES_NO_MAIN)
file(STRINGS file_lists/test_files TEST_FILES)

find_package(GTest QUIET)
add_executable(stim_test ${SOURCE_FILES_NO_MAIN} ${TEST_FILES})
target_link_libraries(stim_test GTest::gtest GTest::gtest_main)
target_compile_options(stim_test PRIVATE -Wall -Wpedantic -g -fno-omit-frame-pointer -fno-strict-aliasing -fsanitize=undefined -fsanitize=address ${MACHINE_FLAG})
target_link_options(stim_test PRIVATE -g -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=address)
