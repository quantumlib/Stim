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

find src | grep \.cc | grep -Ev "test|pybind|perf|^src/main\.cc" > $FOLDER/source_files_no_main
find src | grep \.cc | grep test > $FOLDER/test_files
find src | grep \.cc | grep perf > $FOLDER/benchmark_files
find src | grep \.cc | grep pybind > $FOLDER/python_api_files
