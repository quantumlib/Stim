#!/bin/bash
set -e

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

# Find relevant source files using naming conventions.
readarray -t stim_src_files < <( \
    find src \
    | grep "\\.cc" \
    | grep -v "\\.\(test\|perf\|pybind\)\\.cc" \
    | grep -v "main\\.cc" \
)
readarray -t glue_src_files < <( \
    find glue/javascript \
    | grep "\\.js\\.cc" \
)
readarray -t js_test_files < <( \
    find glue/javascript \
    | grep "\\.test\\.js" \
)

mkdir -p out
echo '<script src="stim.js"></script>' > out/all_stim_tests.html
echo '<script type="text/javascript">' >> out/all_stim_tests.html
cat "glue/javascript/stim.test_harness.js" "${js_test_files[@]}" >> out/all_stim_tests.html
echo '</script>' >> out/all_stim_tests.html

# Build web assembly module using emscripten.
emcc \
    -s NO_DISABLE_EXCEPTION_CATCHING \
    -s EXPORT_NAME="load_stim_module"  \
    -s MODULARIZE=1  \
    -s SINGLE_FILE=1  \
    -o out/stim.js \
    --no-entry \
    --bind \
    -I src \
    "${glue_src_files[@]}" \
    "${stim_src_files[@]}"
