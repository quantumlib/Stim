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

# Regenerate 'stim.h' to include all relevant headers.
{
    echo "#ifndef _STIM_H";
    echo "#define _STIM_H";
    echo "/// WARNING: THE STIM C++ API MAKES NO COMPATIBILITY GUARANTEES.";
    echo "/// It may change arbitrarily and catastrophically from minor version to minor version.";
    echo "/// If you need a stable API, use stim's Python API.";
    find src | grep "\\.h$" | grep -v "\\.\(test\|perf\|pybind\)\\.h$" | grep -v "src/stim\\.h" | grep -v "src/stim/mem/simd_word_.*" | LC_ALL=C sort | sed 's/src\/\(.*\)/#include "\1"/g';
    echo "#endif";
} > src/stim.h

# Regenerate crumble's unit test imports.
find glue/crumble | grep "\\.test.js$" | LC_ALL=C sort | sed 's/glue\/crumble\(.*\)/import "..\1"/g' > "glue/crumble/test/test_import_all.js"
