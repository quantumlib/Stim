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

# Build web assembly module using emscripten.
emcc \
    -s NO_DISABLE_EXCEPTION_CATCHING \
    -s EXPORT_NAME="load_stim_module"  \
    -s MODULARIZE=1  \
    -o out/stim.wasm \
    -o out/stim.js \
    --no-entry \
    --bind \
    "${glue_src_files[@]}" \
    "${stim_src_files[@]}"
