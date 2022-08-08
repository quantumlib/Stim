#!/bin/bash
set -e

#########################################################################
# Deletes files created by cmake, python setup.py, and other build steps.
#########################################################################

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

rm CMakeFiles -rf
rm CMakeCache.txt -f
rm Makefile -f
rm dist -rf
rm cmake_install.cmake -f
rm cmake-build-debug-coverage -rf
rm coverage -rf
rm stim.egg-info -rf
rm bazel-bin -rf
rm bazel-out -rf
rm bazel-stim -rf
rm bazel-testlogs -rf
rm Testing -rf
rm out -rf
rm cmake-build-debug -rf
