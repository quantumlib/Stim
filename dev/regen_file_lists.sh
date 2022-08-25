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

echo "generating file lists in $FOLDER"

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

# LC_ALL=C forces sorting to happen by byte value
find src | grep \.cc | grep -Ev "test|pybind|perf|^src/main\.cc" | LC_ALL=C sort > $FOLDER/source_files_no_main
find src | grep \.cc | grep test | LC_ALL=C sort > $FOLDER/test_files
find src | grep \.cc | grep perf | LC_ALL=C sort > $FOLDER/benchmark_files
find src | grep \.cc | grep pybind | LC_ALL=C sort > $FOLDER/python_api_files
