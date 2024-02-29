#!/bin/bash
set -e

#########################################################################
# Generates javascript exporting a string KNOWN_GATE_NAMES_FROM_STIM.
#########################################################################

echo "const KNOWN_GATE_NAMES_FROM_STIM = \`"
python -c "import stim; stim.main(command_line_args=['help', 'gates'])" | grep "    " | sed 's/^ *//g'
echo "\`"
echo
echo "export {KNOWN_GATE_NAMES_FROM_STIM};"
