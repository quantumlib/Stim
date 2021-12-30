# This is a convenience bash script that regenerates documentation files.
# It assumes several things about the development environment which may not be true for you.

set -e

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

# Put latest stim python wheel into python environment.
bazel build :stim.so
rm -f stim.so
cp bazel-bin/stim.so stim.so

# Build latest stim command line tool.
cmake .
make stim

# Generate the docs.
python glue/python/generate_api_reference.py > doc/python_api_reference_vDev.md
out/stim help gates_markdown > doc/gates.md
out/stim help formats_markdown > doc/result_formats.md
out/stim help flags_markdown > doc/usage_command_line.md

echo Finished regenerating docs.
