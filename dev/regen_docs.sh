#!/bin/bash
set -e

#########################################################################
# Regenerates doc files using the installed version of stim.
#########################################################################

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

python glue/python/generate_api_reference.py -dev > doc/python_api_reference_vDev.md
python glue/python/generate_stub_file.py -dev > glue/python/src/stim/__init__.pyi
python glue/python/generate_stub_file.py -dev > doc/stim.pyi
python -c "import stim; stim.main(command_line_args=['help', 'gates_markdown'])" > doc/gates.md
python -c "import stim; stim.main(command_line_args=['help', 'formats_markdown'])" > doc/result_formats.md
python -c "import stim; stim.main(command_line_args=['help', 'commands_markdown'])" > doc/usage_command_line.md
