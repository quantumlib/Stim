#!/bin/bash
set -e

#########################################################################
# Regenerate file_lists
#########################################################################

if [ "$#" -ne 1 ]; then
    FOLDER=file_lists
else
    FOLDER=$1
fi

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

# LC_ALL=C forces sorting to happen by byte value
find src | grep "\\.cc$" | grep -v "\\.\(test\|perf\|pybind\)\\.cc$" | grep -v "src/main\\.cc" | LC_ALL=C sort > "${FOLDER}/source_files_no_main"
find src | grep "\\.test\\.cc$" | LC_ALL=C sort > "${FOLDER}/test_files"
find src | grep "\\.perf\\.cc$" | LC_ALL=C sort > "${FOLDER}/benchmark_files"
find src | grep "\\.pybind\\.cc$" | LC_ALL=C sort > "${FOLDER}/python_api_files"
