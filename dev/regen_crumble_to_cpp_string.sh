#!/bin/bash
set -e

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

dev/regen_crumble_to_cpp_string_write_to_stdout.sh > src/stim/diagram/crumble_data.cc
