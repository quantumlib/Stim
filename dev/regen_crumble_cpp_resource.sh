#!/bin/bash
set -e

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

dev/compile_crumble_into_cpp_string_file.sh > src/stim/diagram/crumble_data.cc
